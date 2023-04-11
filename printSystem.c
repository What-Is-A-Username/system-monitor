#include <sys/utsname.h>
#include <stdio.h>

/**
 * Print system information according to data from uname()
 * @return 0 if operation was successful, 1 otherwise
*/
int printSystemInfo()
{
    // Print system information
    // DOCS: https://man7.org/linux/man-pages/man2/uname.2.html
    struct utsname unameInfo;
    int unameStatus = uname(&unameInfo);
    if (unameStatus == 0)
    {
        // On success
        printf("### System Information ###\n");
        printf("System Name = %s\n", unameInfo.sysname);
        printf("Machine Name = %s\n", unameInfo.nodename);
        printf("Version = %s\n", unameInfo.version);
        printf("Release = %s\n", unameInfo.release);
        printf("Architecture = %s\n", unameInfo.machine);
    }
    else if (unameStatus == -1)
    {
        // On error
        perror("Error retrieving system data from uname()");
        return 1;
    }
    return 0;
}

