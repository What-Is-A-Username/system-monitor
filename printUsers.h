#ifndef PRINT_USERS_H
#define PRINT_USERS_H

#define USER_START_FLAG 3
#define USER_DATA_ID 3
#define MAX_USERS 512

#ifndef FD_WRITE
#define FD_WRITE 1
#endif

#ifndef FD_READ
#define FD_READ 0
#endif

extern void printUsers(int writeToChildFds[2], int readFromChildFds[2], int incomingDataPipe[2]);

#endif