#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>

#include "printUsers.h"
#include "printSystem.h"
#include "stringUtils.h"
#include "parseArguments.h"
#include "parseCpuStats.h"
#include "parseMemoryStats.h"

#define FD_READ 0
#define FD_WRITE 1

#define IN_DEBUG_MODE false

/**
 * Signal to indicate that user wants to terminate the program.
*/
#define CALLED_TERMINATE SIGUSR1

/**
 * Signal to indicate that user wants to continue the program.
*/
#define CALLED_CONTINUE SIGUSR2

/**
 * Index of file descriptors used for communication with memory utilization process.
*/
#define MEM_FDS 0

/**
 * Index of file descriptors used for communication with user process.
*/
#define USER_FDS 1

/**
 * Index of file descriptors used for communication with CPU utilization process.
*/
#define CPU_FDS 2

/**
 * Begin process of terminating children and parent processes.
 * @param writeToChildFds File descriptors of pipes used to communicate to children
 * @param readFromChildFds File descriptors of pipes used to communicate from children
 * @param incomingDataPipe Additional file descriptors of pipes used to communicate from children
*/
void terminateChildProcesses(int writeToChildFds[3][2], int readFromChildFds[3][2], int incomingDataPipe[2])
{
    // TODO: Clean up and free memory if termination

    // tell children to exit
    int temp = -1;
    for (int i = 0; i < 3; i++)
    {
        write(writeToChildFds[i][FD_WRITE], &temp, sizeof(int));
    }

    // wait on children to exit
    for (int i = 0; i < 3; i++)
    {
        int status;
        pid_t w = wait(&status);
        if (IN_DEBUG_MODE) {
            // write(STDOUT_FILENO, chldMsg, sizeof(char) * (strnlen(chldMsg, 256) + 1));
            if (WIFEXITED(status))
                printf("Child %d terminated with status %d\n", w, WEXITSTATUS(status));
            else if (WIFSIGNALED(status))
                printf("Child %d terminated with signal %d\n", w, WTERMSIG(status));
        }
    }

    // close all fds
    for (int i = 0; i < 3; i++)
    {
        close(writeToChildFds[i][FD_READ]);
        close(writeToChildFds[i][FD_WRITE]);
        close(readFromChildFds[i][FD_READ]);
        close(readFromChildFds[i][FD_WRITE]);
    }
    close(incomingDataPipe[FD_WRITE]);
    close(incomingDataPipe[FD_READ]);
}

/**
 * Configure signal mask on child processes made from forking.
*/
void configureChildSignals()
{
    sigset_t blocker;
    sigemptyset(&blocker);
    // Make child ignore the following signals
    sigaddset(&blocker, SIGINT);
    sigaddset(&blocker, SIGTSTP);
    sigaddset(&blocker, CALLED_TERMINATE);
    sigaddset(&blocker, CALLED_CONTINUE);
    sigprocmask(SIG_BLOCK, &blocker, NULL);
}

/**
 * Signal handler for Ctrl-C signal (SIGINT) on the parent process.
 */
void askQuitMainProcess(int signum, siginfo_t *info, void *context)
{
    char *msg = "\nAre you sure you want to exit the program? (y/n)\n";
    write(STDOUT_FILENO, msg, sizeof(char) * (strnlen(msg, 256) + 1));
    char response;
    while (read(STDIN_FILENO, &response, sizeof(char)))
    {
        if (response == 'y' || response == 'Y')
        {
            // terminate the program
            char *terminationStartMsg = "Terminating processes ...\n";
            write(STDOUT_FILENO, terminationStartMsg, sizeof(char) * (strnlen(terminationStartMsg, 256) + 1));
            kill(getpid(), CALLED_TERMINATE); // send user signal to terminate execution
            break;
        }
        else if (response == 'n' || response == 'N')
        {
            // continue the program
            char *continueMsg = "Continuing process ...\n";
            write(STDOUT_FILENO, continueMsg, sizeof(char) * (strnlen(continueMsg, 256) + 1));
            kill(getpid(), CALLED_CONTINUE); // send user signal to continue execution
            break;
        }
        else
        {
            // unknown input from user
            char *promptAgain = "Response not recognized. Please try again.\n\n";
            write(STDOUT_FILENO, promptAgain, sizeof(char) * (strnlen(promptAgain, 256) + 1));
            write(STDOUT_FILENO, msg, sizeof(char) * (strnlen(msg, 256) + 1));
        }
    }
}

/**
 * Sleep for the time delay between samples
 * @param sampleDelay The number of seconds to wait for
 * @param writeToChildFds File descriptors of pipes used to communicate to children
 * @param readFromChildFds File descriptors of pipes used to communicate from children
 * @param incomingDataPipe Additional file descriptors of pipes used to communicate from children
 * @return Returns CALLED_CONTINUE if execution is to continue as usual, and will not return otherwise.
*/
int sleepForSampleDelay(int sampleDelay, int writeToChildFds[3][2], int readFromChildFds[3][2], int incomingDataPipe[2])
{
    struct timespec req, rem;
    req.tv_sec = sampleDelay;
    req.tv_nsec = 0;
    rem.tv_nsec = 0;
    rem.tv_sec = 0;
    while (req.tv_nsec > 0 || req.tv_sec > 0) {
        if (nanosleep(&req, &rem) == -1)
        {
            // handle when sleep interrupted by signal or error
            if (IN_DEBUG_MODE)
                perror("nanosleep");
            // check if CALLED_TERMINATE or CALLED_CONTINUE signals pending
            sigset_t blocked;
            sigpending(&blocked);
            if (sigismember(&blocked, CALLED_TERMINATE))
            {
                if (IN_DEBUG_MODE)
                    printf("Detected interrupt CALLED_TERMINATE\n");
                terminateChildProcesses(writeToChildFds, readFromChildFds, incomingDataPipe);
                exit(EXIT_SUCCESS);
            }
            else if (sigismember(&blocked, CALLED_CONTINUE))
            {
                if (IN_DEBUG_MODE)
                    printf("Detected interrupt CALLED_CONTINUE %ld, %ld\n", rem.tv_nsec, rem.tv_sec);
                req.tv_nsec = rem.tv_nsec;
                req.tv_sec = rem.tv_sec;
            } 
            else {
                printf("Unknown input given. Terminating program.\n");
                return CALLED_CONTINUE;
            }
        } 
        else {
            req.tv_nsec = 0;
            req.tv_sec = 0;
        }
        printf("Waiting another %lds, %ldns\n", req.tv_sec, req.tv_nsec);
    }
    return CALLED_CONTINUE;
}

int main(int argc, char **argv)
{
    struct sigaction ignoreSig;
    ignoreSig.sa_flags = 0;
    ignoreSig.sa_handler = SIG_IGN;
    sigemptyset(&ignoreSig.sa_mask);
    if (sigaction(SIGTSTP, &ignoreSig, NULL) == -1)
    {
        perror("Sigaction: SIGTSTP");
        exit(EXIT_FAILURE);
    }

    struct sigaction sig;
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = &askQuitMainProcess;
    sigemptyset(&sig.sa_mask);
    if (sigaction(SIGINT, &sig, NULL) == -1)
    {
        perror("Sigaction: SIGINT");
        exit(EXIT_FAILURE);
    }

    sigset_t alwaysIgnored;
    sigemptyset(&alwaysIgnored);
    sigaddset(&alwaysIgnored, SIGTSTP);
    sigaddset(&alwaysIgnored, CALLED_TERMINATE);
    sigaddset(&alwaysIgnored, CALLED_CONTINUE);
    sigprocmask(SIG_BLOCK, &alwaysIgnored, NULL);

    sigset_t criticalCodeBlocker;
    sigemptyset(&criticalCodeBlocker);
    sigaddset(&criticalCodeBlocker, SIGINT);

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

    /**
     * Fds of pipes used to read data from children.
     */
    int readFromChildFds[3][2];

    /**
     * Pipe for notifying parent of incoming data.
     */
    int incomingDataPipe[2];

    /**
     * Fds of pipes used to write data to children
     */
    int writeToChildFds[3][2];

    // parse command line arguments
    if (parseArguments(argc, argv, &showSystem, &showUser, &showGraphics, &showSequential, &numSamples, &sampleDelay) != 0)
    {
        return 1;
    }
    printf("\033[2J\033[3J");

    printf("\033[2J\033[H\n");

    // printf("Parsed arguments: --system %d --user %d --graphics %d --sequential %d numSamples %ld samplesDelay %ld\n",
    //        showSystem, showUser, showGraphics, showSequential, numSamples, sampleDelay);

    // store previously calculated output
    char *memoryOutput[numSamples + 1];
    char *cpuOutput[numSamples + 1];
    for (int i = 0; i <= numSamples; i++)
    {
        memoryOutput[i] = NULL;
        cpuOutput[i] = NULL;
    }
    char *userInfo[MAX_USERS];
    int numUsers = 0;
    for (int i = 0; i < MAX_USERS; i++)
    {
        userInfo[i] = NULL;
    }

    int processorCount;
    int coreCount;
    char *averageCpuUsage = NULL;

    pipe(incomingDataPipe);

    // create child processes
    if (showSystem || !showUser)
    {                                    // Show memory utilization
        pipe(writeToChildFds[MEM_FDS]);  // create pipe for parent -> child
        pipe(readFromChildFds[MEM_FDS]); // pipe for child -> parent
        pid_t memoryPid = fork();        // memory usage process
        if (memoryPid == 0)
        { // child process
            configureChildSignals();
            close(writeToChildFds[MEM_FDS][FD_WRITE]);
            close(readFromChildFds[MEM_FDS][FD_READ]); // prevent child from reading data meant for parent
            close(incomingDataPipe[FD_READ]);
            displayMemory(numSamples, showGraphics, writeToChildFds[MEM_FDS], readFromChildFds[MEM_FDS], incomingDataPipe);
            exit(0);
        }
        else
        {
            close(writeToChildFds[MEM_FDS][FD_READ]);
            close(readFromChildFds[MEM_FDS][FD_WRITE]);
        }

        pipe(writeToChildFds[CPU_FDS]);  // create pipe for parent -> child
        pipe(readFromChildFds[CPU_FDS]); // pipe for child -> parent
        pid_t cpuPid = fork();
        if (cpuPid == 0)
        {
            configureChildSignals();
            close(writeToChildFds[CPU_FDS][FD_WRITE]);
            close(readFromChildFds[CPU_FDS][FD_READ]);
            close(incomingDataPipe[FD_READ]);
            displayCpu(numSamples, showGraphics, writeToChildFds[CPU_FDS], readFromChildFds[CPU_FDS], incomingDataPipe);
            exit(0);
        }
        else
        {
            close(writeToChildFds[CPU_FDS][FD_READ]);
            close(readFromChildFds[CPU_FDS][FD_WRITE]);
        }
    }

    if (!showSystem || showUser)
    {
        pipe(writeToChildFds[USER_FDS]);  // create pipe for parent -> child
        pipe(readFromChildFds[USER_FDS]); // pipe for child -> parent
        pid_t userPid = fork();
        if (userPid == 0)
        {
            configureChildSignals();
            close(writeToChildFds[USER_FDS][FD_WRITE]);
            close(readFromChildFds[USER_FDS][FD_READ]);
            close(incomingDataPipe[FD_READ]);
            printUsers(writeToChildFds[USER_FDS], readFromChildFds[USER_FDS], incomingDataPipe);
            exit(0);
        }
        else
        {
            close(writeToChildFds[USER_FDS][FD_READ]);
            close(readFromChildFds[USER_FDS][FD_WRITE]);
        }
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        perror("Signal SIGPIPE");
    }

    close(incomingDataPipe[FD_WRITE]);

    for (int thisSample = 0; thisSample <= numSamples; thisSample++)
    {
        sigprocmask(SIG_BLOCK, &criticalCodeBlocker, NULL);

        // ensure this iteration's info is empty
        memoryOutput[thisSample] = NULL;
        cpuOutput[thisSample] = NULL;
        for (int i = 0; i < numUsers; i++)
        {
            if (userInfo[i] != NULL)
            {
                free(userInfo[i]);
                userInfo[i] = NULL;
            }
        }
        numUsers = 0;

        if (averageCpuUsage != NULL)
        {
            free(averageCpuUsage);
            averageCpuUsage = NULL;
        }

        // PASS DATA TO PROCESSES

        if (showSystem || !showUser)
        {

            // MEMORY USAGE (system information)
            int temp = MEM_START_FLAG;
            write(writeToChildFds[MEM_FDS][FD_WRITE], &temp, sizeof(int));
            write(writeToChildFds[MEM_FDS][FD_WRITE], &thisSample, sizeof(int));

            // CPU USAGE
            temp = CPU_START_FLAG;
            write(writeToChildFds[CPU_FDS][FD_WRITE], &temp, sizeof(int));
            write(writeToChildFds[CPU_FDS][FD_WRITE], &thisSample, sizeof(int));
        }

        if (showUser || !showSystem)
        {
            // USERS
            int temp = USER_START_FLAG;
            write(writeToChildFds[USER_FDS][FD_WRITE], &temp, sizeof(int));
            write(writeToChildFds[USER_FDS][FD_WRITE], &thisSample, sizeof(int));
        }

        if (IN_DEBUG_MODE)
            printf("Passed data\n");

        if (thisSample == 0) // skip printing on first iteration
        { 
            // temporarily unblock SIGINT to allow interrupt during sleep
            sigprocmask(SIG_UNBLOCK, &criticalCodeBlocker, NULL);
            // sleep
            sleepForSampleDelay(sampleDelay, writeToChildFds, readFromChildFds, incomingDataPipe);
            continue;
        }

        bool errored = false;
        // read output from children
        while (!errored)
        {
            int processFunction = 0, strLen = 0;
            int readInp = read(incomingDataPipe[FD_READ], &processFunction, sizeof(int));
            if (readInp == 0)
                continue;
            if (readInp == -1)
            {
                perror("read: incomingDataPipe");
                return EXIT_FAILURE;
            }
            if (IN_DEBUG_MODE)
                printf("Received info of type %d\n", processFunction);
            switch (processFunction)
            {
            case MEM_DATA_ID:
                read(readFromChildFds[MEM_FDS][FD_READ], &strLen, sizeof(int));
                memoryOutput[thisSample] = (char *)malloc(sizeof(char) * (strLen + 1));
                read(readFromChildFds[MEM_FDS][FD_READ], memoryOutput[thisSample], sizeof(char) * (strLen + 1));
                break;

            case CPU_DATA_ID:
                read(readFromChildFds[CPU_FDS][FD_READ], &processorCount, sizeof(int));
                read(readFromChildFds[CPU_FDS][FD_READ], &coreCount, sizeof(int));

                // read average CPU usage line
                read(readFromChildFds[CPU_FDS][FD_READ], &strLen, sizeof(int));
                averageCpuUsage = (char *)malloc(sizeof(char) * (strLen + 1));
                read(readFromChildFds[CPU_FDS][FD_READ], averageCpuUsage, sizeof(char) * (strLen + 1));

                // read timepoint CPU utilization
                read(readFromChildFds[CPU_FDS][FD_READ], &strLen, sizeof(int));
                cpuOutput[thisSample] = (char *)malloc(sizeof(char) * (strLen + 1));
                read(readFromChildFds[CPU_FDS][FD_READ], cpuOutput[thisSample], sizeof(char) * (strLen + 1));
                break;

            case USER_DATA_ID:
                while (read(readFromChildFds[USER_FDS][FD_READ], &strLen, sizeof(int)) > 0)
                {
                    if (strLen == 0)
                        break;
                    if (numUsers < MAX_USERS)
                    {
                        userInfo[numUsers] = (char *)malloc(sizeof(char) * (strLen + 1));
                        read(readFromChildFds[USER_FDS][FD_READ], userInfo[numUsers], sizeof(char) * (strLen + 1));
                        numUsers++;
                    }
                    else
                    {
                        char *overflowBin = (char *)malloc(sizeof(char) * (strLen + 1));
                        read(readFromChildFds[USER_FDS][FD_READ], overflowBin, sizeof(char) * (strLen + 1));
                        free(overflowBin);
                    }
                }
                break;

            default:
                errored = true;
                break;
            }

            // check if information is complete
            if (showSystem || !showUser)
            {
                if (memoryOutput[thisSample] == NULL || cpuOutput[thisSample] == NULL || userInfo == NULL)
                {
                    continue;
                }
            }
            if (showUser || !showSystem)
            {
                if (userInfo == NULL)
                {
                    continue;
                }
            }
            break;
        }

        if (IN_DEBUG_MODE)
            printf("Read data\n");

        if (!showSequential)
        {
            // use escape characters to make it appear that screen is refreshing
            // \033[3J erases saved lines
            // \033[H repositions cursor to start
            // \033[2J erases entire screen
            printf("\033[2J\033[3J\033[2J\033[H\n");
        }

        printf("\n||| Sample #%d |||\n", thisSample);
        printDivider();
        printf("Nbr of samples: %ld -- every %ld secs\n", numSamples, sampleDelay);

        struct rusage rUsageData;
        getrusage(RUSAGE_SELF, &rUsageData);
        printf("Memory usage: %ld kilobytes\n", rUsageData.ru_maxrss);

        printDivider();

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
            for (int i = 1; i <= numSamples; i++)
            {
                if (memoryOutput[i] == NULL)
                    printf("\n");
                else
                    printf("%s", memoryOutput[i]);
            }
            printDivider();
        }

        // USER CONNECTIONS (user information)
        if (showUser || !showSystem)
        {
            printf("### Sessions/users ###\n");
            for (int i = 0; i < numUsers; i++)
            {
                printf("%s", userInfo[i]);
            }
            printDivider();
        }

        if (showSystem || !showUser)
        {
            printf("Number of processors: %d\n", processorCount);
            printf("Total number of cores: %d\n", coreCount);
            // Print the average CPU utilization from beginning to current sample
            printf("%s", averageCpuUsage);

            printDivider();

            if (showGraphics)
            {
                printf("CPU Utilization (%% Use, Relative Abs. Change, %% Use Graphic)\n");
            }
            else
            {
                printf("CPU Utilization (%% Use, Relative Abs. Change)\n");
            }

            for (int i = 1; i <= numSamples; i++)
            {
                if (cpuOutput[i] == NULL)
                    printf("\n");
                else
                    printf("%s", cpuOutput[i]);
            }

            printDivider();
        }

        printf("||| End of Sample #%d |||\n", thisSample);

        // temporarily unblock SIGINT to allow interrupt during sleep
        sigprocmask(SIG_UNBLOCK, &criticalCodeBlocker, NULL);
        if (thisSample != numSamples)
            sleepForSampleDelay(sampleDelay, writeToChildFds, readFromChildFds, incomingDataPipe);

        printf("\n\n");
    }

    printDivider();
    if (printSystemInfo() != 0)
    {
        return 1;
    }
    printDivider();

    if (averageCpuUsage != NULL)
    {
        free(averageCpuUsage);
    }

    for (int thisSample = 0; thisSample < numSamples; thisSample++)
    {
        if (memoryOutput[thisSample] != NULL)
            free(memoryOutput[thisSample]);
        if (cpuOutput[thisSample] != NULL)
            free(cpuOutput[thisSample]);
    }
    for (int i = 0; i < numUsers; i++)
    {
        if (userInfo[i] != NULL)
            free(userInfo[i]);
    }

    terminateChildProcesses(writeToChildFds, readFromChildFds, incomingDataPipe);

    return EXIT_SUCCESS;
}
