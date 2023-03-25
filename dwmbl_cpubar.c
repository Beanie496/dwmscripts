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
	// while the first word starts with 'cpu[num]', get each line of /proc/stat and record the 5th word
	// for each subsequent entry of the recorded idle times, do the same thing but don't multiply by 100, multiply by 8
	// get the relevant CPU bar height and print it
	// exit
	setlocale(LC_ALL, "");
	if (getCoreInfo())
		return 0;
	showTotalCPUTime();
	showVisualCores();


	return 0;
}


void showTotalCPUTime()
{
	printf("CPU: %.2f%%", 100.0 - (float)(100 * coreStats.idleTime) /
			 (float)(coreStats.elapsedTime * coreStats.totalCores)
			);
}


void showVisualCores()
{
	int fraction;

	printf(" ");
	for (int i = 0; i < coreStats.totalCores; i++) {
		fraction = LENGTH(bars) - 1 - (int)(coreStats.idleTimes[i] * 8 / coreStats.elapsedTime);
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
	int tmp[cores];

	coreStats.totalCores = cores;
	coreStats.idleTimes = malloc(sizeof(int *) * cores);
	// I get the total uptime, the total idle time and the idle time
	// of each core as close together as possible because the idle time
	// of some of the cores might increase between getting the uptime
	// and getting the idle time of that core
	fscanf(uptimeFile, "%lf %lf", &uptime, &idle);
	// ignore the first line, as that stores the collective core info
	fscanf(stats, "%*s %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d");
	for (int i = 0; i < cores; i++)
		fscanf(stats, "%*s %*d %*d %*d %d %*d %*d %*d %*d %*d %*d", &coreStats.idleTimes[i]);
	fclose(uptimeFile);
	fclose(stats);

	coreStats.elapsedTime = uptime * ticksPerSec;
	coreStats.idleTime = idle * ticksPerSec;

	if (cache == (FILE *)NULL) {
		cache = fopen("/tmp/cpubarcache", "w");
		fprintf(cache, "%d %d", coreStats.elapsedTime, coreStats.idleTime);
		for (int i = 0; i < cores; i++)
			fprintf(cache, " %d", coreStats.idleTimes[i]);
		fclose(cache);
		return 1;
	}

	fscanf(cache, "%d %d", &cacheTime, &cacheIdle);
	for (int i = 0; i < cores; i++)
		fscanf(cache, "%d", &tmp[i]);
	rewind(cache);
	fprintf(cache, "%d %d", coreStats.elapsedTime, coreStats.idleTime);
	for (int i = 0; i < cores; i++)
		fprintf(cache, " %d", coreStats.idleTimes[i]);
	fclose(cache);

	coreStats.elapsedTime -= cacheTime;
	coreStats.idleTime -= cacheIdle;
	for (int i = 0; i < cores; i++)
		coreStats.idleTimes[i] -= tmp[i];

	return 0;
}

char *getLine(FILE *fp)
{
	// this breaks on files that don't end with a newline, but that's not a problem in linux
	int i = 0;
	while ((line[i] = fgetc(fp)) != '\n' && i < MAXLINE)
		i++;
	line[i] = '\0';

	return line;
}

/*
char *getWord(char *string, int wordIdx)
{
	int currWord = 0;
	int letter = 0;
	char *retWord;

	while (string[letter] != EOF && currWord < wordIdx)
		if (isspace(string[letter++]))
			currWord++;

	if (string[letter] == EOF || currWord < wordIdx)
		return NULL;

	retWord = word;
	// MAXWORD - 1 to give space for the null char
	while (!isspace(*retWord = string[letter++]) && retWord < &word[MAXWORD - 1])
		retWord++;
	*retWord = '\0';
	
	return word;
}
*/
