#ifndef PRINT_USERS_H
#define PRINT_USERS_H

#include <utmp.h>
#include <stdio.h>

/**
 * Print users and sessions with data from getutent().
*/
void printUsers()
{
    struct utmp *data = getutent();
    while (data != NULL)
    {
        if (data->ut_type == USER_PROCESS)
        {
            printf("%s\t %s (%s)\n", data->ut_user, data->ut_line, data->ut_host);
        }
        data = getutent();
    }
    // close the currently open utmp file
    endutent();
}

#endif