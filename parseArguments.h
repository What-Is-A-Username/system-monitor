#ifndef PARSE_ARGUMENTS_H
#define PARSE_ARGUMENTS_H

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
 * Parse all command line arguments and store the results.
 * @param argc The number of command line arguments.
 * @param argv An string array of the arguments
 * @param showSystem Pointer to boolean where the --system flag value will be set
 * @param showUser Pointer to boolean where the --user flag value will be set
 * @param showGraphics Pointer to boolean where the --graphics flag value will be set
 * @param showSequential Pointer to boolean where the --sequential flag value will be set
 * @param numSamples Pointer to long where the --samples value will be set
 * @param sampleDelay Pointer to long where the --tdelay value will be set
 * @return Returns zero if command line arguments successfully returned, non-zero otherwise.
*/
extern int parseArguments(const int argc, char **argv, bool *showSystem, bool *showUser, bool *showGraphics, bool *showSequential, long* numSamples, long *sampleDelay);

#endif