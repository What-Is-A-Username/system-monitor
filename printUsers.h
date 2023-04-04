#ifndef PRINT_USERS_H
#define PRINT_USERS_H

#include <utmp.h>
#include <stdio.h>

#define USER_START_FLAG 3
#define USER_DATA_ID 3

#ifndef FD_WRITE
#define FD_WRITE 1
#endif

#ifndef FD_READ
#define FD_READ 0
#endif

/**
 * Print users and sessions with data from getutent().
*/
void printUsers(int writeToChildFds[2], int readFromChildFds[2], int incomingDataPipe[2])
{
    char outputString[4096] = "";
    int parentInfo;
    while (true) {

        // get an instruction from the parent
        read(writeToChildFds[FD_READ], &parentInfo, sizeof(int));
        if (parentInfo != USER_START_FLAG) {
            // TODO: Remove before submitting
            printf("User process ended.\n");
            break;
        }

        struct utmp *data = getutent();
        while (data != NULL)
        {
            if (data->ut_type == USER_PROCESS)
            {
                snprintf(outputString, 4096, "%s%s\t %s (%s)\n", outputString, data->ut_user, data->ut_line, data->ut_host);
            }
            data = getutent();
        }

        int outLen = strlen(outputString);
        write(readFromChildFds[FD_WRITE], &outLen, sizeof(int)); 
        write(readFromChildFds[FD_WRITE], outputString, sizeof(char) * (outLen + 1));

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