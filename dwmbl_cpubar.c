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

#define MAXLINE     100
#define MAXWORD     20
#define todigit(c)  ((c) - '0')

static char line[MAXLINE];
static const wchar_t bars[] = { 0x0020, 0x2581, 0x2582, 0x2583, 0x2584, 0x2585, 0x2586, 0x2587, 0x2588 };

struct {
	int totalCores;
	int *idleTimes;
} coreStats;

// returns the total idle time in clock ticks
int showTotalCPUTime(void);
void showVisualCores(int elapsedTime);
void getCoreInfo(void);
char *getLine(FILE *fp);


int main(int argc, char *argv[])
{
	// while the first word starts with 'cpu[num]', get each line of /proc/stat and record the 5th word
	// for each subsequent entry of the recorded idle times, do the same thing but don't multiply by 100, multiply by 8
	// get the relevant CPU bar height and print it
	// exit
	setlocale(LC_ALL, "");
	getCoreInfo();
	int elapsedTime = showTotalCPUTime();
	if (elapsedTime == 0)
		return 0;
	showVisualCores(elapsedTime);


	return 0;
}


int showTotalCPUTime()
{
	FILE *cache = fopen("/tmp/cpubarcache", "r+");
	FILE *uptimeFile = fopen("/proc/uptime", "r");
	double oldTimeTotal, oldIdleTotal;
	double currTimeTotal, currIdleTotal;
	int tmp[coreStats.totalCores];

	if (cache == (FILE *)NULL) {
		cache = fopen("/tmp/cpubarcache", "w");
		fprintf(cache, "%s", getLine(uptimeFile));
		for (int i = 0; i < coreStats.totalCores; i++)
			fprintf(cache, " %d", coreStats.idleTimes[i]);
		fclose(cache);
		fclose(uptimeFile);
		return 0;
	}

	fscanf(uptimeFile, "%lf %lf", &currTimeTotal, &currIdleTotal);
	fclose(uptimeFile);

	fscanf(cache, "%lf %lf\n", &oldTimeTotal, &oldIdleTotal);
	for (int i = 0; i < coreStats.totalCores; i++)
		fscanf(cache, "%d", &tmp[i]);
	rewind(cache);
	fprintf(cache, "%f %f", currTimeTotal, currIdleTotal);
	for (int i = 0; i < coreStats.totalCores; i++)
		fprintf(cache, " %d", coreStats.idleTimes[i]);
	fclose(cache);
	for (int i = 0; i < coreStats.totalCores; i++)
		coreStats.idleTimes[i] -= tmp[i];

	printf("CPU: %.2f%% ", 100.0 - 100.0 * 
			((currIdleTotal - oldIdleTotal) /
			 ((currTimeTotal - oldTimeTotal) * (double)coreStats.totalCores))
			);

	return (int)((currTimeTotal - oldTimeTotal) * sysconf(_SC_CLK_TCK));
}


void showVisualCores(int elapsedTime)
{
	int fraction;

	printf(" ");
	for (int i = 0; i < coreStats.totalCores; i++) {
		fraction = (sizeof(bars) / sizeof(bars[0])) - 1 - (int)(coreStats.idleTimes[i] * 8 / elapsedTime);
		if (fraction < 0)
			fraction = 0;
		printf("%lc", bars[fraction]);
	}
	printf("\n");
}


void getCoreInfo()
{
	FILE *stats = fopen("/proc/stat", "r");
	coreStats.totalCores = sysconf(_SC_NPROCESSORS_ONLN);
	coreStats.idleTimes = malloc(sizeof(int *) * coreStats.totalCores);

	// ignore the first line, as that stores the collective core info
	fscanf(stats, "%*s %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d");
	
	for (int i = 0; i < coreStats.totalCores; i++)
		fscanf(stats, "%*s %*d %*d %*d %d %*d %*d %*d %*d %*d %*d", &coreStats.idleTimes[i]);

	fclose(stats);
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
