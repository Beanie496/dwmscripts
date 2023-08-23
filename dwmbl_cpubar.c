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

static void cacheTimes(void);
static inline void getCacheInfo(void);
static inline void getCoreInfo(void);
// this reads the two numbers from /proc/uptime, so they're lumped into one
// function for efficiency
static inline void getUptimeAndIdleTime(void);
static inline void calcTimeDiffs(void);
static inline void skipOverLine(FILE *fp);
static inline void showTotalCPUTime(void);
static inline void showVisualCore(int core);
static inline void showVisualCores(void);

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
	int cores;
	int elapsedTime;
	int totalIdleTime;
	int *coreIdleTimes;
} coreStats;

static struct {
	int time;
	int idle;
	int *coreIdle;
} cacheInfo;

int main(int argc, char *argv[])
{
	// this allows the wide chars to be used
	setlocale(LC_ALL, "");

	getUptimeAndIdleTime();
	getCoreInfo();
	getCacheInfo();
	cacheTimes();
	calcTimeDiffs();
	showTotalCPUTime();
	showVisualCores();

	return 0;
}

void getUptimeAndIdleTime()
{
	double uptime, idle;
	int ticksPerSec = sysconf(_SC_CLK_TCK);
	FILE *uptimeFile = fopen("/proc/uptime", "r");
	if (uptimeFile == NULL) {
		fprintf(stderr, "Error reading from /proc/uptime\n");
		exit(ERR_FILE_CANNOT_BE_OPENED);
	}

	fscanf(uptimeFile, "%lf %lf", &uptime, &idle);
	fclose(uptimeFile);

	coreStats.elapsedTime = uptime * ticksPerSec;
	coreStats.totalIdleTime = idle * ticksPerSec;
}

void getCoreInfo()
{
	coreStats.cores = sysconf(_SC_NPROCESSORS_ONLN);

	FILE *stats = fopen("/proc/stat", "r");
	if (stats == NULL) {
		fprintf(stderr, "Error reading from /proc/stat\n");
		exit(ERR_FILE_CANNOT_BE_OPENED);
	}

	coreStats.coreIdleTimes = malloc(sizeof(int) * coreStats.cores);
	// ignore the first line, as that stores the collective core info
	skipOverLine(stats);
	// get the idle time of each core (the 5th column)
	for (int i = 0; i < coreStats.cores; i++)
		fscanf(stats, "%*s %*d %*d %*d %d %*d %*d %*d %*d %*d %*d",
				&coreStats.coreIdleTimes[i]);
	fclose(stats);
}

void getCacheInfo()
{
	FILE *cache = fopen("/tmp/cpubarcache", "r");
	if (cache == NULL) {
		fprintf(stderr, "Error reading from cache\n");
		cacheTimes();
		exit(SUCCESS);
	}
	cacheInfo.coreIdle = malloc(sizeof(int) * coreStats.cores);

	fscanf(cache, "%d %d", &cacheInfo.time, &cacheInfo.idle);
	for (int i = 0; i < coreStats.cores; i++)
		fscanf(cache, "%d", &cacheInfo.coreIdle[i]);
	fclose(cache);
}

void cacheTimes()
{
	FILE *cache = fopen("/tmp/cpubarcache", "w");
	if (cache == NULL) {
		fprintf(stderr, "Error writing to cache\n");
		exit(ERR_CACHE_CANNOT_BE_OPENED);
	}

	fprintf(cache, "%d %d", coreStats.elapsedTime, coreStats.totalIdleTime);
	for (int i = 0; i < coreStats.cores; i++)
		fprintf(cache, " %d", coreStats.coreIdleTimes[i]);
	fprintf(cache, "\n");
	fclose(cache);
}

void calcTimeDiffs()
{
	coreStats.elapsedTime -= cacheInfo.time;
	coreStats.totalIdleTime -= cacheInfo.idle;
	for (int i = 0; i < coreStats.cores; i++)
		coreStats.coreIdleTimes[i] -= cacheInfo.coreIdle[i];
	free(cacheInfo.coreIdle);
}

void showTotalCPUTime()
{
	double fraction;
	if (coreStats.elapsedTime == 0)
	       fraction = 0;
	else
	       fraction = (double)(100 * coreStats.totalIdleTime) /
		       (double)(coreStats.elapsedTime * coreStats.cores);
	// I don't know why, but somwtimes the idle time diff is higher than
	// the elapsed time diff
	if (fraction > 100.0)
		fraction = 100.0;
	printf("CPU: %.2f%%", 100.0 - fraction);
}

void showVisualCores()
{
	printf(" ");
	for (int i = 0; i < coreStats.cores; i++)
		showVisualCore(i);
	free(coreStats.coreIdleTimes);
	printf("\n");
}

void showVisualCore(int core)
{
	int fraction;
	if (coreStats.elapsedTime == 0)
	        fraction = 0;
	else
	        // gets a number from 0 to 8, where 8 is max CPU use and 0 is
	        // none.
		fraction = LENGTH(bars) - 1 -
			(int)(8 * coreStats.coreIdleTimes[core] / coreStats.elapsedTime);
	// The idle time diff is sometimes higher than the total elapsed time
	// diff
	if (fraction < 0)
		fraction = 0;
	printf("%lc", bars[fraction]);
}

void skipOverLine(FILE *fp)
{
	while (fgetc(fp) != '\n')
		;
}
