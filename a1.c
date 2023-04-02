#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/resource.h>

#include "printUsers.h"
#include "printSystem.h"
#include "stringUtils.h"
#include "parseArguments.h"
#include "parseCpuStats.h"
#include "parseMemoryStats.h"

int main(int argc, char **argv)
{
    /**
     * Show only the system usage? (--system)
     */
    bool showSystem = false;
    /**
     * Show only the user's usage? (--user)
     */
    bool showUser = false;
    /**
     * Show graphical output for memory and CPU utilization? (--graphics)
     */
    bool showGraphics = false;
    /**
     * Output information sequentially without refreshing screen? (--sequential)
     */
    bool showSequential = false;
    /**
     * The number of times that the usage statistics will be sampled. (--samples). Default = 10;
     */
    long numSamples = 10;
    /**
     *  The time between consecutive samples of the usage statistics, in seconds (--tdelay). Default = 1
     */
    long sampleDelay = 1;

    // parse command line arguments
    if (parseArguments(argc, argv, &showSystem, &showUser, &showGraphics, &showSequential, &numSamples, &sampleDelay) != 0)
    {
        return 1;
    }
    printf("\033[2J\033[3J");

    printf("\033[2J\033[H\n");

    // printf("Parsed arguments: --system %d --user %d --graphics %d --sequential %d numSamples %ld samplesDelay %ld\n",
    //        showSystem, showUser, showGraphics, showSequential, numSamples, sampleDelay);

    // store retrieved and computed memory values of each sample
    struct memorySample memoryData[numSamples];

    // store past computed cpu utilization for each sample + initial data point
    struct cpuDataSample cpuData[numSamples + 1];
    float cpuHistory[numSamples];
    for (int i = 0; i < numSamples; i++)
    {
        cpuHistory[i] = 0.0;
    }

    // record initial CPU stats
    if (recordCpuStats(&cpuData[0]) != 0)
    {
        return 1;
    }
    sleep(sampleDelay);

    for (int thisSample = 0; thisSample < numSamples; thisSample++)
    {
        if (!showSequential)
        {
            // use escape characters to make it appear that screen is refreshing
            // \033[3J erases saved lines
            // \033[H repositions cursor to start
            // \033[2J erases entire screen
            printf("\033[2J\033[3J\033[2J\033[H\n");
        }

        printf("\n||| Sample #%d |||\n", thisSample + 1);
        printDivider();
        printf("Nbr of samples: %ld -- every %ld secs\n", numSamples, sampleDelay);

        struct rusage rUsageData;
        getrusage(RUSAGE_SELF, &rUsageData);
        printf("Memory usage: %ld kilobytes\n", rUsageData.ru_maxrss);

        printDivider();

        // MEMORY USAGE (system information)
        if (showSystem || !showUser)
        {
            if (showGraphics)
            {
                printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot, Memory Graphic)\n");
            }
            else
            {
                printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
            }
            // Retrieve memory information from sysinfo
            // DOCS: https://man7.org/linux/man-pages/man2/sysinfo.2.html
            if (computeMemory(memoryData + thisSample) != 0)
            {
                return 1;
            }

            for (int i = 0; i < numSamples; i++)
            {
                if (i <= thisSample)
                {
                    if (showGraphics)
                    {
                        // print graphical representations
                        char *memoryGraphics = NULL;
                        if (i == 0)
                        {
                            // if first sample, exclude previous data
                            memoryGraphics = calculateDelta(NULL, memoryData + i);
                        }
                        else
                        {
                            // for all other samples, pass two data points
                            memoryGraphics = calculateDelta(memoryData + i - 1, memoryData + i);
                        }
                        if (memoryGraphics == NULL)
                        {
                            fprintf(stderr, "Errored while showing memory graphics.");
                            return 1;
                        }
                        printf("%s \t%s\n", memoryData[i].memoryOutput, memoryGraphics);
                        free(memoryGraphics);
                    }
                    else
                    {
                        printf("%s\n", memoryData[i].memoryOutput);
                    }
                }
                else
                {
                    printf("\n");
                }
            }

            printDivider();
        }

        // USER CONNECTIONS (user information)
        if (showUser || !showSystem)
        {
            printf("### Sessions/users ###\n");
            printUsers();
            printDivider();
        }

        if (showSystem || !showUser)
        {
            // Total number of processors on the machine
            int processorCount = 0;
            // Total number of cores across all processors on the machine
            int coreCount = 0;
            if (getCpuCounts(&processorCount, &coreCount) != 0)
            {
                return 1;
            }

            // sample the cpu utilization
            if (recordCpuStats(&cpuData[thisSample + 1]) != 0)
            {
                return 1;
            }

            printf("Number of processors: %d\n", processorCount);
            printf("Total number of cores: %d\n", coreCount);
            // Print the average CPU utilization from beginning to current sample
            printf("\tAverage Usage = %.4f%%\n", calculateCpuUsage(&cpuData[0], &cpuData[thisSample + 1]));

            printDivider();

            if (showGraphics)
            {
                printf("CPU Utilization (%% Use, Relative Abs. Change, %% Use Graphic)\n");
            }
            else
            {
                printf("CPU Utilization (%% Use, Relative Abs. Change)\n");
            }

            // calculate the cpu utilization for the current sample
            // calculate the unused cpu time
            cpuHistory[thisSample] = calculateCpuUsage(&cpuData[thisSample], &cpuData[thisSample + 1]);

            // print all cpu utilization of all samples
            for (long pastSample = 0; pastSample < numSamples; pastSample++)
            {
                if (pastSample <= thisSample)
                {
                    if (pastSample == 0)
                    {
                        if (showGraphics)
                        {
                            char *cpuGraphics = renderCPUUsage(cpuHistory[pastSample]);
                            // print only the % usage if this is the first sample
                            printf("%.2f%% (0.00) \t%s\n", cpuHistory[pastSample], cpuGraphics);
                            free(cpuGraphics);
                        }
                        else
                        {
                            printf("%.2f%% (0.00)\n", cpuHistory[pastSample]);
                        }
                    }
                    else if (pastSample > 0)
                    {
                        // print the change in cpu % usage from the previous sample
                        float absChange = cpuHistory[pastSample] - cpuHistory[pastSample - 1];
                        if (showGraphics)
                        {
                            char *cpuGraphics = renderCPUUsage(cpuHistory[pastSample]);
                            printf("%.2f%% (%.2f) \t%s\n", cpuHistory[pastSample], absChange, cpuGraphics);
                            free(cpuGraphics);
                        }
                        else
                        {
                            printf("%.2f%% (%.2f)\n", cpuHistory[pastSample], absChange);
                        }
                    }
                }
                else
                {
                    printf("\n");
                }
            }

            printDivider();
        }

        printf("||| End of Sample #%d |||\n\n\n", thisSample + 1);

        if (thisSample < numSamples - 1)
        {
            sleep(sampleDelay);
        }
    }

    printDivider();
    if (printSystemInfo() != 0) {
        return 1;
    }
    printDivider();
    
    return 0;
}
