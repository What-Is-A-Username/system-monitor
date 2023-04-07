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
 * Representation of a single data point of CPU usage, as set by recordCpuStats()
 */
typedef struct cpuDataSample
{
    long user;
    long nice;
    long system;
    long idle;
    long iowait;
    long irq;
    long softirq;
    long steal;
    long guest;
    long guest_nice;
} CpuDataSample;

/**
 * Retrieve the number of processors and cores on the machine while considering hyperthreading.
 * Statistics are calculated by reading /proc/cpuinfo, and counting the number of unique physical ids and summing their sibling counts.
 * @param processorCount Pointer to int where the number of processors will be stored
 * @param coreCount Pointer to int where the number of cores will be stored
 * @returns 0 if operation successful, 1 otherwise
 */
int getCpuCounts(int *processorCount, int *coreCount)
{
    FILE *cpuinfodata = fopen("/proc/cpuinfo", "r");
    char inp[CPUINFO_LINE_LENGTH], inpVal[CPUINFO_LINE_LENGTH];
    bool processors_seen[MAX_PROCESSORS] = {false};
    int lastPhysicalId = 0;

    if (cpuinfodata == NULL)
    {
        fprintf(stderr, "Encountered error finding /proc/cpuinfo");
        return 1;
    }
    else
    {
        while (fgets(inp, CPUINFO_LINE_LENGTH, cpuinfodata) != NULL)
        {
            // check if the line begins with "processor  :"
            if (startsWith(inp, "physical id"))
            {
                // retrieve physical id and store it
                strncpy(inpVal, strstr(inp, ":") + 1, CPUINFO_LINE_LENGTH);
                lastPhysicalId = atoi(inpVal);
            }
            else if (startsWith(inp, "siblings"))
            {
                // add the number of "cores" for processors we havent seen before
                // by using "siblings" so we account for hyperthreading, so the core count is similar to that of htop and nproc
                if (lastPhysicalId < MAX_PROCESSORS && !processors_seen[lastPhysicalId])
                {
                    strncpy(inpVal, strstr(inp, ":") + 1, CPUINFO_LINE_LENGTH);
                    processors_seen[lastPhysicalId] = true;
                    *coreCount = *coreCount + atoi(inpVal);
                    *processorCount = *processorCount + 1;
                }
            }
        }
    }
    if (fclose(cpuinfodata) != 0)
    {
        fprintf(stderr, "Failed to properly close /proc/cpuinfo");
        return 1;
    }
    return 0;
}

/**
 * Record a data point for CPU utilization by reading from /proc/stat, and store the data in a struct.
 * @param cpuHistoryRow A pointer to a cpuDataSample struct used to store the parsed values
 * @returns 1 if operation was successful, 0 otherwise
 */
int recordCpuStats(struct cpuDataSample *cpuHistoryRow)
{
    char totalLine[CPUINFO_LINE_LENGTH];
    FILE *statdata = fopen("/proc/stat", "r");
    if (statdata == NULL)
    {
        fprintf(stderr, "Encountered error finding /proc/cpuinfo");
        return 1;
    }
    else
    {
        // read the first line of /proc/stat which contains the total cpu time
        fgets(totalLine, CPUINFO_LINE_LENGTH, statdata);
        char *nextNum = NULL;
        // separate the entries in the row using the space as delimiter
        nextNum = strtok(totalLine, " ");
        int index = 0;
        nextNum = strtok(NULL, " ");
        // parse each integer
        while (nextNum != NULL && index < 10)
        {
            long val = atol(nextNum);
            switch (index)
            {
            case 0:
                cpuHistoryRow->user = val;
                break;
            case 1:
                cpuHistoryRow->nice = val;
                break;
            case 2:
                cpuHistoryRow->system = val;
                break;
            case 3:
                cpuHistoryRow->idle = val;
                break;
            case 4:
                cpuHistoryRow->iowait = val;
                break;
            case 5:
                cpuHistoryRow->irq = val;
                break;
            case 6:
                cpuHistoryRow->softirq = val;
                break;
            case 7:
                cpuHistoryRow->steal = val;
                break;
            case 8:
                cpuHistoryRow->guest = val;
                break;
            case 9:
                cpuHistoryRow->guest_nice = val;
                break;
            }
            index++;
            nextNum = strtok(NULL, " ");
        }
    }
    if (fclose(statdata) != 0)
    {
        perror("Failed to close /proc/stat");
        return 1;
    }
    return 0;
}

/**
 * Calculate the percentage CPU utilization (0%-100%) that occurred between two CPU usage data points parsed by recordCpuStats.
 * @param previous Pointer to data point taken first.
 * @param current Pointer to data point taken second.
 * @returns Percentage CPU utilization, with 100 representing 100%.
 */
float calculateCpuUsage(struct cpuDataSample *previous, struct cpuDataSample *current)
{
    // calculate the cpu change attributed to idle
    long deltaIdle = current->idle - previous->idle;

    // calculate the total cpu change from previous to current
    long totalDelta = (current->user - previous->user) +
                      (current->nice - previous->nice) +
                      (current->system - previous->system) +
                      deltaIdle +
                      (current->iowait - previous->iowait) +
                      (current->irq - previous->irq) +
                      (current->softirq - previous->softirq) +
                      (current->steal - previous->steal);

    float usage = (1 - (float)deltaIdle / (float)totalDelta) * 100;
    return usage;
}

/**
 * Return a pointer to dynamically-allocated string representing the specified change in CPU utilization graphically.
 * The display begins with a [ character, followed by a number of | characters proportional to the CPU utilization level
 * @param cpuUsage The value of CPU utilization to be displayed. 100 = 100%.
 * @return A string displaying the CPU usage as a graphical display.
 */
char *renderCPUUsage(float cpuUsage)
{
    char *deltaChange = (char *)malloc((GRAPHICS_MAX_CPU_BAR_COUNT + GRAPHICS_MAX_CPU_NUM_COUNT) * sizeof(char));

    deltaChange[0] = '[';
    for (int i = 1; i < GRAPHICS_MAX_CPU_BAR_COUNT + GRAPHICS_MAX_CPU_NUM_COUNT - 1; i++)
    {
        deltaChange[i] = ' ';
    }
    deltaChange[GRAPHICS_MAX_CPU_BAR_COUNT + GRAPHICS_MAX_CPU_NUM_COUNT - 1] = '\0';

    int bars = cpuUsage / 100.0 * GRAPHICS_MAX_CPU_BAR_COUNT;
    for (int i = 1; i <= (int)(bars); i++)
    {
        deltaChange[i] = '|';
    }
    deltaChange[bars + 1] = '\0';

    char deltaNum[GRAPHICS_MAX_CPU_NUM_COUNT] = "";
    snprintf(deltaNum, GRAPHICS_MAX_CPU_NUM_COUNT, " %.2f%%", cpuUsage);

    strncat(deltaChange, deltaNum, GRAPHICS_MAX_CPU_NUM_COUNT);

    return deltaChange;
}

/**
 * Handle processing and printing of CPU utilization stats
 * @param numSamples The number of iterations specified by command line arguments
 * @param showGraphics Command line argument for whether to show memory use graphics
 * @param writeToChildFds Pipes used to read input data from main
 * @param readFromChildFds Pipes used to write input data to main
 * @param incomingDataPipe Pipe used to notify parent of data ready in readFromChildFds
 */
void displayCpu(int numSamples, bool showGraphics, int writeToChildFds[2], int readFromChildFds[2], int incomingDataPipe[2])
{
    struct cpuDataSample cpuData[numSamples + 1];
    float cpuHistory[numSamples + 1];
    for (int i = 0; i <= numSamples; i++)
    {
        cpuHistory[i] = 0.0;
    }

    char averageUseOutputString[4096], outputString[4096];
    int parentInfo, thisSample;

    while (true)
    {
        // get an instruction from the parent
        read(writeToChildFds[FD_READ], &parentInfo, sizeof(int));
        if (parentInfo != CPU_START_FLAG) {
            // TODO: Remove before submitting
            printf("CPU process ended.\n");
            break;
        }

        // get the iteration number
        read(writeToChildFds[FD_READ], &thisSample, sizeof(int));

        // Total number of processors on the machine
        int processorCount = 0;
        // Total number of cores across all processors on the machine
        int coreCount = 0;
        if (getCpuCounts(&processorCount, &coreCount) != 0)
        {
            exit(1);
        }

        // sample the cpu utilization
        if (recordCpuStats(cpuData + thisSample) != 0)
        {
            exit(1);
        }

        // compute average since start
        float averageCpuUsage = calculateCpuUsage(cpuData, cpuData + thisSample);
        snprintf(averageUseOutputString, 4096, "\tAverage Usage = %.4f%%\n", averageCpuUsage);

        // calculate the cpu utilization for the current sample
        // calculate the unused cpu time
        if (thisSample > 0)
        cpuHistory[thisSample] = calculateCpuUsage(cpuData + thisSample - 1, cpuData + thisSample);

        if (thisSample == 0) {
            continue;
        }
        else if (thisSample == 1)
        {
            if (showGraphics)
            {
                char *cpuGraphics = renderCPUUsage(cpuHistory[thisSample]);
                // print only the % usage if this is the first sample
                snprintf(outputString, 4096, "%.2f%% (0.00) \t%s\n", cpuHistory[thisSample], cpuGraphics);
                free(cpuGraphics);
            }
            else
            {
                snprintf(outputString, 4096, "%.2f%% (0.00)\n", cpuHistory[thisSample - 1]);
            }
        }
        else if (thisSample > 1)
        {
            // print the change in cpu % usage from the previous sample
            float absChange = cpuHistory[thisSample] - cpuHistory[thisSample - 1];
            if (showGraphics)
            {
                char *cpuGraphics = renderCPUUsage(cpuHistory[thisSample]);
                snprintf(outputString, 4096, "%.2f%% (%.2f) \t%s\n", cpuHistory[thisSample], absChange, cpuGraphics);
                free(cpuGraphics);
            }
            else
            {
                snprintf(outputString, 4096, "%.2f%% (%.2f)\n", cpuHistory[thisSample], absChange);
            }
        }

        // send results back to parent in a pipe
        write(readFromChildFds[FD_WRITE], &processorCount, sizeof(int));
        write(readFromChildFds[FD_WRITE], &coreCount, sizeof(int));
        
        int outLen = strlen(averageUseOutputString);
        write(readFromChildFds[FD_WRITE], &outLen, sizeof(int)); 
        write(readFromChildFds[FD_WRITE], averageUseOutputString, sizeof(char) * (outLen + 1));
        outLen = strlen(outputString);
        write(readFromChildFds[FD_WRITE], &outLen, sizeof(int)); 
        write(readFromChildFds[FD_WRITE], outputString, sizeof(char) * (outLen + 1));
    
        // printf("Notifying parent cpu");
        int temp = CPU_DATA_ID; 
        write(incomingDataPipe[FD_WRITE], &temp, sizeof(int)); // notify parent that there is cpu data
    }
    exit(0);
    close(readFromChildFds[FD_READ]);
    close(readFromChildFds[FD_WRITE]);
    close(writeToChildFds[FD_READ]);
    close(writeToChildFds[FD_WRITE]);
    close(incomingDataPipe[FD_READ]);
    close(incomingDataPipe[FD_WRITE]);
}

#endif
