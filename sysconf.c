#include <unistd.h>

long total_threads() {
	return sysconf(_SC_NPROCESSORS_ONLN);
}

long ticks_per_second() {
	return sysconf(_SC_CLK_TCK);
}
