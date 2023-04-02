#ifndef PARSE_MEMORY_H
#define PARSE_MEMORY_H

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

#define GIGABYTE_BYTE_SIZE 1073741824
#define GRAPHICS_MAX_BAR_COUNT 512
#define GRAPHICS_MAX_NUM_COUNT 32

/**
 * Representation of a single data point of memory usage, as set by recordCpuStats(), as well as a string representation to be printed according to convertMemoryToString()
*/
struct memorySample
{
    float physUsed, physTot, virtUsed, virtTot;
    char *memoryOutput;
    char *delta;
};

/**
 * Generate a human readable string representation of the memory utilization at the given sample data point and store its result in the same struct.
 * @param sample A memory utilization data point.
 * @param mem_unit The size of a memory unit, as given by sysinfo.mem_unit
*/
void convertMemoryToString(struct memorySample *sample, unsigned int mem_unit)
{
    // calculate the number of gigabytes
    sample->memoryOutput = (char *)malloc(sizeof(char) * 63);
    sprintf(sample->memoryOutput, "%.2f GB / %.2f GB -- %.2f GB / %.2f GB", sample->physUsed, sample->physTot, sample->virtUsed, sample->virtTot);
    return;
}

/**
 * Retrieve memory information to calculate current utilization and store the result and memory statistics.
 * @param sample Pointer to memorySample point to store the current memory utilization.
 * @returns 0 if operation was successful, 1 otherwise
*/
int computeMemory(struct memorySample *sample)
{
    struct sysinfo sysinfoData;
    int sysinfoStatus = sysinfo(&sysinfoData);
    if (sysinfoStatus == 0)
    {
        // total physical ram
        sample->physTot = sysinfoData.totalram / (float)GIGABYTE_BYTE_SIZE * sysinfoData.mem_unit;
        // used physical ram
        sample->physUsed = (sysinfoData.totalram - sysinfoData.freeram) / (float)GIGABYTE_BYTE_SIZE * sysinfoData.mem_unit;
        // total virtual ram
        sample->virtTot = (sysinfoData.totalswap + sysinfoData.totalram) / (float)GIGABYTE_BYTE_SIZE * sysinfoData.mem_unit;
        // used virtual ram
        sample->virtUsed = (sysinfoData.totalram - sysinfoData.freeram + sysinfoData.totalswap - sysinfoData.freeswap) / (float)GIGABYTE_BYTE_SIZE * sysinfoData.mem_unit;
        // generate string with statistics
        convertMemoryToString(sample, sysinfoData.mem_unit);
    }
    else if (sysinfoStatus == -1)
    { 
        // On error
        perror("Failed to read information from sysinfo()");
        return 1;
    }
    return 0;
}

/**
 * Return a pointer to dynamically-allocated string representing the change in memory usage using graphical bars.
*/
char *calculateDelta(struct memorySample *previous, struct memorySample *current)
{
    // allocate a string to store the result
    char *deltaChange = (char *)malloc((GRAPHICS_MAX_BAR_COUNT + GRAPHICS_MAX_NUM_COUNT) * sizeof(char));
    if (current == NULL)
    {
        // Error if current is null
        snprintf(deltaChange, GRAPHICS_MAX_BAR_COUNT, "---");
        return deltaChange;
    }
    else if (previous == NULL)
    {
        // First entry
        snprintf(deltaChange, GRAPHICS_MAX_BAR_COUNT, "|o 0.00 (%.2f)", current->virtUsed);
        return deltaChange;
    }

    // initialize the string to start with | and end with null terminator
    deltaChange[0] = '|';
    for (int i = 1; i < GRAPHICS_MAX_BAR_COUNT + GRAPHICS_MAX_NUM_COUNT - 1; i++)
    {
        deltaChange[i] = ' ';
    }
    deltaChange[GRAPHICS_MAX_BAR_COUNT + GRAPHICS_MAX_NUM_COUNT - 1] = '\0';

    // calculate the net change in gigabytes, in absolute and percentage difference
    float delta = current->virtUsed - previous->virtUsed;
    float deltaPercentage = delta / current->virtTot;

    // the maximum number of relative change bars we can display, while leaving space for starting bar (|), ending symbol (* or @) and null terminator
    int maxChangeBars = GRAPHICS_MAX_BAR_COUNT - 3;

    // if change was positive (more memory is being used)
    if (deltaPercentage > 0)
    {
        int bars = (int)(deltaPercentage * maxChangeBars);
        for (int i = 1; i <= bars; i++)
        {
            deltaChange[i] = '#';
        }
        deltaChange[bars + 1] = '*';
        deltaChange[bars + 2] = '\0';
    }
    // if change was positive (less memory being used)
    else if (deltaPercentage < 0)
    {
        int bars = (int)(-deltaPercentage * maxChangeBars);
        for (int i = 1; i <= bars; i++)
        {
            deltaChange[i] = ':';
        }
        deltaChange[bars + 1] = '@';
        deltaChange[bars + 2] = '\0';
    }
    else // change of zero 
    {
        snprintf(deltaChange, GRAPHICS_MAX_BAR_COUNT, "|o 0.00 (%.2f)", current->virtUsed);
        return deltaChange;
    }

    char deltaNum[GRAPHICS_MAX_NUM_COUNT] = "";
    // print the change in memory numerically 
    snprintf(deltaNum, GRAPHICS_MAX_NUM_COUNT, " %.2f (%.2f)", delta, current->virtUsed);

    // add the numerical stats at the end of the graphical representation
    strncat(deltaChange, deltaNum, GRAPHICS_MAX_NUM_COUNT);
    return deltaChange;
}

#endif