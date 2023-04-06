#ifndef PRINT_USERS_H
#define PRINT_USERS_H

#include <utmp.h>
#include <stdio.h>

#define USER_START_FLAG 3
#define USER_DATA_ID 3
#define MAX_USERS 512

#ifndef FD_WRITE
#define FD_WRITE 1
#endif

#ifndef FD_READ
#define FD_READ 0
#endif

/**
 * Handle processing and printing of connected user information.
 * @param writeToChildFds Pipes used to read input data from main
 * @param readFromChildFds Pipes used to write input data to main
 * @param incomingDataPipe Pipe used to notify parent of data ready in readFromChildFds
 */
void printUsers(int writeToChildFds[2], int readFromChildFds[2], int incomingDataPipe[2])
{
    int outLen = 0;
    char outputString[4096] = "";
    int parentInfo, thisSample;
    while (true) {

        // get an instruction from the parent
        read(writeToChildFds[FD_READ], &parentInfo, sizeof(int));
        if (parentInfo != USER_START_FLAG) {
            // TODO: Remove before submitting
            printf("User process ended.\n");
            break;
        }

        read(writeToChildFds[FD_READ], &thisSample, sizeof(int));
        if (thisSample == 0) {
            continue;
        }

        struct utmp *data = getutent();
        while (data != NULL)
        {
            if (data->ut_type == USER_PROCESS)
            {
                snprintf(outputString, 4096, "%s\t %s (%s)\n", data->ut_user, data->ut_line, data->ut_host);
                outLen = strlen(outputString);
                if (outLen > 0) {
                    write(readFromChildFds[FD_WRITE], &outLen, sizeof(int)); 
                    write(readFromChildFds[FD_WRITE], outputString, sizeof(char) * (outLen + 1));
                }
            }
            data = getutent();
        }

        outLen = 0;
        write(readFromChildFds[FD_WRITE], &outLen, sizeof(int)); 

        int temp = USER_DATA_ID; 
        write(incomingDataPipe[FD_WRITE], &temp, sizeof(int)); // notify parent that memory data is available

        // close the currently open utmp file
        endutent();
    }
    close(readFromChildFds[FD_READ]);
    close(readFromChildFds[FD_WRITE]);
    close(writeToChildFds[FD_READ]);
    close(writeToChildFds[FD_WRITE]);
    exit(0);
}

#endif