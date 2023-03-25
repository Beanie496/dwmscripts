/*
 * dwmbl_cpubar - a program to display overall CPU use and/or
 * graphically show the load average for each core.
 * I wrote this program because writing a shell script became
 * too long and complicated, to the point where I suspected it
 * would starts affecting the number it was trying to measure
 * (since my average CPU use is less than 0.5% usually).
 * Also, I prefer writing C than bash scripting.
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

static char line[MAXLINE];
// this stores 9 characters: a space,
// then the 8 block elements that go from lower eighth to full block
static const wchar_t bars[] = { 0x0020, 0x2581, 0x2582, 0x2583, 0x2584, 0x2585, 0x2586, 0x2587, 0x2588 };

struct {
	int totalCores;
	int elapsedTime;
	int idleTime;
	int *idleTimes;
} coreStats;

// returns the total idle time in clock ticks
void showTotalCPUTime(void);
void showVisualCores(void);
int getCoreInfo(void);
char *getLine(FILE *fp);


int main(int argc, char *argv[])
{
	// this allows the wide chars to be used
	setlocale(LC_ALL, "");

	if (getCoreInfo())
		return 0;
	showTotalCPUTime();
	showVisualCores();

	return 0;
}


void showTotalCPUTime()
{
	double fraction = (double)(100 * coreStats.idleTime) /
		(double)(coreStats.elapsedTime * coreStats.totalCores);
	// I don't know why, but somwtimes the idle time diff is higher than
	// the elapsed time diff
	if (fraction > 100.0)
		fraction = 100.0;
	printf("CPU: %.2f%%", 100.0 - fraction);
}


void showVisualCores()
{
	int fraction;

	printf(" ");
	for (int i = 0; i < coreStats.totalCores; i++) {
		fraction = LENGTH(bars) - 1 -
			(int)(8 * coreStats.idleTimes[i] / coreStats.elapsedTime);
		// same as before. The idle time diff is sometimes higher than
		// the total elapsed time diff.
		if (fraction < 0)
			fraction = 0;
		printf("%lc", bars[fraction]);
	}
	printf("\n");
}


int getCoreInfo()
{
	FILE *cache = fopen("/tmp/cpubarcache", "r+");
	FILE *stats = fopen("/proc/stat", "r");
	FILE *uptimeFile = fopen("/proc/uptime", "r");
	double uptime, idle;
	int cacheTime, cacheIdle;
	int ticksPerSec = sysconf(_SC_CLK_TCK);
	int cores = sysconf(_SC_NPROCESSORS_ONLN);
	int cachedCoreIdleTimes[cores];

	coreStats.totalCores = cores;
	coreStats.idleTimes = malloc(sizeof(int *) * cores);
	// the total uptime, the total idle time and the idle time
	// of each core are read in as close together as possible because
	// the idle time of some of the cores might increase between getting
	// the total uptime and getting the idle time of it
	fscanf(uptimeFile, "%lf %lf", &uptime, &idle);
	// ignore the first line, as that stores the collective core info
	getLine(stats);
	// get the idle time of each core (the 5th column)
	for (int i = 0; i < cores; i++)
		fscanf(stats, "%*s %*d %*d %*d %d %*d %*d %*d %*d %*d %*d", &coreStats.idleTimes[i]);
	fclose(uptimeFile);
	fclose(stats);

	// this doesn't truncate the decimal by the way
	coreStats.elapsedTime = uptime * ticksPerSec;
	coreStats.idleTime = idle * ticksPerSec;

	// if the cache doesn't exist, store the elapsed time, idle time,
	// and idle time of each core (all in clock ticks)
	if (cache == (FILE *)NULL) {
		cache = fopen("/tmp/cpubarcache", "w");
		fprintf(cache, "%d %d", coreStats.elapsedTime, coreStats.idleTime);
		for (int i = 0; i < cores; i++)
			fprintf(cache, " %d", coreStats.idleTimes[i]);
		fclose(cache);
		return 1;
	}

	// get all the info from the cache before writing in the current info
	fscanf(cache, "%d %d", &cacheTime, &cacheIdle);
	for (int i = 0; i < cores; i++)
		fscanf(cache, "%d", &cachedCoreIdleTimes[i]);
	rewind(cache);
	fprintf(cache, "%d %d", coreStats.elapsedTime, coreStats.idleTime);
	for (int i = 0; i < cores; i++)
		fprintf(cache, " %d", coreStats.idleTimes[i]);
	fclose(cache);

	// get the difference
	coreStats.elapsedTime -= cacheTime;
	coreStats.idleTime -= cacheIdle;
	for (int i = 0; i < cores; i++)
		coreStats.idleTimes[i] -= cachedCoreIdleTimes[i];

	return 0;
}


char *getLine(FILE *fp)
{
	// this breaks on files that don't end with a newline, but that's
	// not a problem in linux
	// (and it doesn't make sense for windows users to use this code)
	int i = 0;
	while ((line[i] = fgetc(fp)) != '\n' && i < MAXLINE)
		i++;
	line[i] = '\0';

	return line;
}
