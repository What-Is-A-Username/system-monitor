#ifndef PARSE_CPU_STATS_H
#define PARSE_CPU_STATS_H

#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <utmp.h>
#include <inttypes.h>
#include <sys/resource.h>

#include "stringUtils.h"

#ifndef FD_WRITE
#define FD_WRITE 1
#endif

#ifndef FD_READ
#define FD_READ 0
#endif

/**
 * Start flag for cpu stats to be calculated
 */
#define CPU_START_FLAG 2

/**
 * Flag to identify output data as CPU related
*/
#define CPU_DATA_ID 2

/**
 * Max length of string readable from /proc/cpuinfo
 */
#define CPUINFO_LINE_LENGTH 256
/**
 * Max number of processors to check for
 */
#define MAX_PROCESSORS 256
/**
 * Size of a single gigabyte in bytes (1024 ^ 3)
 */
#define GIGABYTE_BYTE_SIZE 1073741824
/**
 * Max length of output string dedicated for displaying the bars in the graphical representation of CPU usage
 */
#define GRAPHICS_MAX_CPU_BAR_COUNT 100
/**
 * Max length of output string dedicated for displaying the numbers in the graphical representation of CPU usage
 */
#define GRAPHICS_MAX_CPU_NUM_COUNT 32

/**
 * Handle processing and printing of CPU utilization stats
 * @param numSamples The number of iterations specified by command line arguments
 * @param showGraphics Command line argument for whether to show memory use graphics
 * @param writeToChildFds Pipes used to read input data from main
 * @param readFromChildFds Pipes used to write input data to main
 * @param incomingDataPipe Pipe used to notify parent of data ready in readFromChildFds
 */
extern void displayCpu(int numSamples, bool showGraphics, int writeToChildFds[2], int readFromChildFds[2], int incomingDataPipe[2]);

#endif
