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
#define MAXBUF      50
#define todigit(c)  ((c) - '0')

static char line[MAXLINE];
static char word[MAXWORD];
static const wchar_t bars[] = { 0x0020, 0x2581, 0x2582, 0x2583, 0x2584, 0x2585, 0x2586, 0x2587, 0x2588 };
static int coreCount;
static double passedIdleTime;

struct {
	int totalCores;
	// every MAXBUF cores, it'll realloc more memory for idleTimes and copy the values over
	int *idleTimes;
} coreStats;

void showTotalCPUTime();
void getNewCoreIdleTime(void);
char *getLine(FILE *fp);
char *getWord(char *string, int wordIdx);


int main(int argc, char *argv[])
{
	// while the first word starts with 'cpu[num]', get each line of /proc/stat and record the 5th word
	// for each subsequent entry of the recorded idle times, do the same thing but don't multiply by 100, multiply by 8
	// get the relevant CPU bar height and print it
	// exit
	coreCount = sysconf(_SC_NPROCESSORS_ONLN);
	setlocale(LC_ALL, "");

	showTotalCPUTime();



	return 0;
}


void showTotalCPUTime()
{
	FILE *cache = fopen("/tmp/cpubarcache", "r+");
	FILE *uptimeFile = fopen("/proc/uptime", "r");
	double idleTimeDif;
	double totalTimeDif;

	if (cache == (FILE *)NULL) {
		cache = fopen("/tmp/cpubarcache", "w");
		fprintf(cache, "%s\n", getLine(uptimeFile));
		fclose(cache);
		fclose(uptimeFile);
		return;
	}

	getLine(uptimeFile);
	totalTimeDif = atof(getWord(line, 0));
	idleTimeDif = atof(getWord(line, 1));

	getLine(cache);
	rewind(cache);
	fprintf(cache, "%f %f\n", totalTimeDif, idleTimeDif);
	totalTimeDif -= atof(getWord(line, 0));
	idleTimeDif -= atof(getWord(line, 1));

	fclose(uptimeFile);
	fclose(cache);

	printf("CPU: %.2f%%\n", 100.0 - 100.0 * (idleTimeDif / (totalTimeDif * (double)coreCount)));
}


void getNewCoreIdleTime()
{
	
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
