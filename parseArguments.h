#ifndef PARSE_ARGUMENTS_H
#define PARSE_ARGUMENTS_H

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

/**
 * Max length of command line argument
*/
#define COMMAND_LINE_LENGTH 256

/**
 * Command line string representing the --system flag
*/
#define ARG_SYSTEM "--system"

/**
 * Command line string representing the --user flag
*/
#define ARG_USER "--user"

/**
 * Command line string representing the --graphics flag
*/
#define ARG_GRAPHICS "--graphics"

/**
 * Command line string representing the --sequential flag
*/
#define ARG_SEQUENTIAL "--sequential"

/**
 * Command line string representing the --samples= flag
*/
#define ARG_SAMPLES "--samples="

/**
 * Command line string representing the --tdelay= flag
*/
#define ARG_TDELAY "--tdelay="

/**
 * Print a standardized error to stderr to indicate to the user that the command arguments are incorrect.
*/
void notifyInvalidArguments() {
    fprintf(stderr, "Error: Command has invalid formatting and could not be parsed.\n");
}

/**
 * Parse an command argument key-value pair separated by an equal sign and store its result.
 * @param result Pointer to where the value will be assigned to
 * @param argv A string representing the command string and the value (e.g. "--samples=3")
 * @returns 0 if operation was successful, 1 otherwise
*/
int parseNumericalArgument(long *result, char *argv)
{
    char *splitToken = strtok(argv, "=");
    splitToken = strtok(NULL, "=");
    if (splitToken == NULL)
    {
        // failed to find a string after the = character
        notifyInvalidArguments();
        return 1;
    }
    long tempResult = atol(splitToken);
    if (tempResult == 0)
    {
        // failed to parse string to number
        notifyInvalidArguments();
        return 1;
    }
    *result = atol(splitToken);
    return 0;
}

/**
 * Parse all command line arguments and store the results.
 * @param argc The number of command line arguments.
 * @param argv An string array of the arguments
 * @param showSystem Pointer to boolean where the --system flag value will be set
 * @param showUser Pointer to boolean where the --user flag value will be set
 * @param showGraphics Pointer to boolean where the --graphics flag value will be set
 * @param showSequential Pointer to boolean where the --sequential flag value will be set
 * @param numSamples Pointer to long where the --samples value will be set
 * @param sampleDelay Pointer to long where the --tdelay value will be set
*/
// Parse command line arguments. Return zero if succeeded, non-zero otherwise.
int parseArguments(const int argc, char **argv, bool *showSystem, bool *showUser, bool *showGraphics, bool *showSequential, long* numSamples, long *sampleDelay)
{
    int positionalArgumentsSet = 0;
    // parse command line arguments
    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {

            // check for a match with each flag

            if (strncmp(argv[i], ARG_SYSTEM, COMMAND_LINE_LENGTH) == 0) {
                *showSystem = true;
            }
            else if (strncmp(argv[i], ARG_USER, COMMAND_LINE_LENGTH) == 0) {
                *showUser = true;
            }
            else if (strncmp(argv[i], ARG_GRAPHICS, COMMAND_LINE_LENGTH) == 0) {
                *showGraphics = true;
            }
            else if (strncmp(argv[i], ARG_SEQUENTIAL, COMMAND_LINE_LENGTH) == 0)  {
                *showSequential = true;
            }
            else if (startsWith(argv[i], ARG_SAMPLES)) {
                if (parseNumericalArgument(numSamples, argv[i]) != 0) {
                    // return non-zero if parsing failed
                    return 1;
                }
            }
            else if (startsWith(argv[i], ARG_TDELAY)) {
                if (parseNumericalArgument(sampleDelay, argv[i]) != 0) {
                    // return non-zero if parsing failed
                    return 1;
                }
            }
            else
            {
                if (positionalArgumentsSet == 0) {
                    // set --samples if this is the first positional argument set
                    *numSamples = atoi(argv[i]);
                    if (*numSamples == 0) {
                        notifyInvalidArguments();
                        return 1;
                    }
                    positionalArgumentsSet++;
                }
                else if (positionalArgumentsSet == 1) {
                    // set --tdelay if this is the second positional argument set
                    *sampleDelay = atoi(argv[i]);
                    if (*sampleDelay == 0) {
                        notifyInvalidArguments();
                        return 1;
                    }
                    positionalArgumentsSet++;
                } 
                else {
                    // there is an unidentifiable command or excess positional arguments
                    notifyInvalidArguments();
                    return 1;
                }
            }
        }
    }
    return 0;
}

#endif