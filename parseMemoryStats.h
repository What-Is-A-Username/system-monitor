#ifndef PARSE_MEMORY_H
#define PARSE_MEMORY_H

#define GIGABYTE_BYTE_SIZE 1073741824
#define GRAPHICS_MAX_BAR_COUNT 512
#define GRAPHICS_MAX_NUM_COUNT 32

/**
 * Flag used to start memory reading
 */
#define MEM_START_FLAG 1

/**
 * Flag used to indicate that data came from memory process
 */
#define MEM_DATA_ID 1

#ifndef FD_WRITE
#define FD_WRITE 1
#endif

#ifndef FD_READ
#define FD_READ 0
#endif

/**
 * Handle processing and printing of memory stats
 * @param numSamples The number of iterations specified by command line arguments
 * @param showGraphics Command line argument for whether to show memory use graphics
 * @param writeToChildFds Pipes used to read input data from main
 * @param readFromChildFds Pipes used to write input data to main
 * @param incomingDataPipe Pipe used to notify parent of data ready in readFromChildFds
 */
extern void displayMemory(int numSamples, bool showGraphics, int writeToChildFds[2], int readFromChildFds[2], int incomingDataPipe[2]);

#endif 
