/*
 * dwmbl_cpubar - a program to display overall CPU use and/or
 * graphically show the load average for each core.
 * I wrote this program because writing a shell script became
 * too long and complicated, to the point where I suspected it
 * would starts affecting the number it was trying to measure
 * (since my average CPU use is less than 0.5% usually).
 * Also, I prefer writing C than shell scripting.
 */
#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE      100
#define MAXWORD      20
#define todigit(c)   ((c) - '0')
#define LENGTH(arr)  (sizeof(arr) / sizeof(arr[0]))

enum error {
	SUCCESS,
	ERR_FILE_CANNOT_BE_OPENED,
	ERR_CACHE_CANNOT_BE_OPENED,
	ERR_CACHE_DOES_NOT_EXIST,
};

static void showTotalCPUTime(void);
static void showVisualCores(void);
static void showVisualCore(int core);
static int getCoreInfo(void);
static int getIdleAndElapsedTime(void);
static int getCoreIdleTimes(void);
static int getCoreIdleTimes(void);
static int cacheTimes(void);
static int getCachedElapsedTimeAndIdleTime(int *cacheTime, int *cacheIdle,
		int cachedCoreIdleTimes[]);
static void getTimeDiffs(int cacheTime, int cacheIdle,
		int cachedCoreIdleTimes[]);
static void skipOverLine(FILE *fp);
static void coreStats_free(void);

// this stores 9 characters: a space, then the 8 block elements that go from
// lower eighth to full block
static const wchar_t bars[] = {
	0x0020,
	0x2581,
	0x2582,
	0x2583,
	0x2584,
	0x2585,
	0x2586,
	0x2587,
	0x2588,
};

static struct {
	int totalCores;
	int elapsedTime;
	int totalIdleTime;
	int *coreIdleTimes;
} coreStats;

int main(int argc, char *argv[])
{
	// this allows the wide chars to be used
	setlocale(LC_ALL, "");

	switch (getCoreInfo()) {
		case SUCCESS:
			break;
		case ERR_FILE_CANNOT_BE_OPENED:
			return 1;
			break;
		case ERR_CACHE_CANNOT_BE_OPENED:
			return 1;
			break;
		case ERR_CACHE_DOES_NOT_EXIST:
			break;
		default:
			fprintf(stderr, "Unknown error code\n");
			break;
	}
	showTotalCPUTime();
	showVisualCores();
	coreStats_free();

	return 0;
}

int getCoreInfo()
{
	int cacheTime, cacheIdle;
	int ret;

	coreStats.totalCores = sysconf(_SC_NPROCESSORS_ONLN);
	int cachedCoreIdleTimes[coreStats.totalCores];

	ret = getIdleAndElapsedTime();
	if (ret)
		return ret;

	ret = getCoreIdleTimes();
	if (ret)
		return ret;

	// TODO: ew
	ret = getCachedElapsedTimeAndIdleTime(&cacheTime, &cacheIdle,
				cachedCoreIdleTimes);
	if (ret) {
		// TODO: move this function call out of this function, as it
		// doesn't make sense for what the function does
		cacheTimes();
		return ret;
	}

	ret = cacheTimes();
	if (ret)
		return ret;

	getTimeDiffs(cacheTime, cacheIdle, cachedCoreIdleTimes);

	return SUCCESS;
}

void showTotalCPUTime()
{
	double fraction = (double)(100 * coreStats.totalIdleTime) /
		(double)(coreStats.elapsedTime * coreStats.totalCores);
	// I don't know why, but somwtimes the idle time diff is higher than
	// the elapsed time diff
	if (fraction > 100.0)
		fraction = 100.0;
	printf("CPU: %.2f%%", 100.0 - fraction);
}

void showVisualCores()
{
	printf(" ");
	for (int i = 0; i < coreStats.totalCores; i++)
		showVisualCore(i);
	printf("\n");
}

void showVisualCore(int core) {
	int fraction = LENGTH(bars) - 1 -
		(int)(8 * coreStats.coreIdleTimes[core] / coreStats.elapsedTime);
	// same as before. The idle time diff is sometimes higher than
	// the total elapsed time diff.
	if (fraction < 0)
		fraction = 0;
	printf("%lc", bars[fraction]);
}

int getIdleAndElapsedTime()
{
	double uptime, idle;
	int ticksPerSec = sysconf(_SC_CLK_TCK);
	FILE *uptimeFile = fopen("/proc/uptime", "r");
	if (uptimeFile == NULL) {
		fprintf(stderr, "Error reading from /proc/uptime\n");
		return ERR_FILE_CANNOT_BE_OPENED;
	}

	fscanf(uptimeFile, "%lf %lf", &uptime, &idle);
	fclose(uptimeFile);

	coreStats.elapsedTime = uptime * ticksPerSec;
	coreStats.totalIdleTime = idle * ticksPerSec;

	return SUCCESS;
}

int getCoreIdleTimes() {
	FILE *stats = fopen("/proc/stat", "r");
	if (stats == NULL) {
		fprintf(stderr, "Error reading from /proc/stat\n");
		return ERR_FILE_CANNOT_BE_OPENED;
	}

	coreStats.coreIdleTimes = malloc(sizeof(int) * coreStats.totalCores);
	// ignore the first line, as that stores the collective core info
	skipOverLine(stats);
	// get the idle time of each core (the 5th column)
	for (int i = 0; i < coreStats.totalCores; i++)
		fscanf(stats, "%*s %*d %*d %*d %d %*d %*d %*d %*d %*d %*d",
				&coreStats.coreIdleTimes[i]);
	fclose(stats);
	return SUCCESS;
}

int cacheTimes() {
	FILE *cache = fopen("/tmp/cpubarcache", "w");
	if (cache == NULL) {
		fprintf(stderr, "Error writing to cache\n");
		return ERR_CACHE_CANNOT_BE_OPENED;
	}

	fprintf(cache, "%d %d", coreStats.elapsedTime, coreStats.totalIdleTime);
	for (int i = 0; i < coreStats.totalCores; i++)
		fprintf(cache, " %d", coreStats.coreIdleTimes[i]);
	fprintf(cache, "\n");
	fclose(cache);

	return SUCCESS;
}

int getCachedElapsedTimeAndIdleTime(int *cacheTime, int *cacheIdle,
		int cachedCoreIdleTimes[]) {
	FILE *cache = fopen("/tmp/cpubarcache", "r");
	if (cache == NULL)
		return ERR_CACHE_DOES_NOT_EXIST;

	fscanf(cache, "%d %d", cacheTime, cacheIdle);
	for (int i = 0; i < coreStats.totalCores; i++)
		fscanf(cache, "%d", &cachedCoreIdleTimes[i]);
	fclose(cache);

	return SUCCESS;
}

void getTimeDiffs(int cacheTime, int cacheIdle, int cachedCoreIdleTimes[]) {
	coreStats.elapsedTime -= cacheTime;
	coreStats.totalIdleTime -= cacheIdle;
	for (int i = 0; i < coreStats.totalCores; i++)
		coreStats.coreIdleTimes[i] -= cachedCoreIdleTimes[i];
}

void skipOverLine(FILE *fp)
{
	while (fgetc(fp) != '\n')
		;
}

void coreStats_free()
{
	free(coreStats.coreIdleTimes);
}
