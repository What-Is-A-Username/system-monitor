#include <string.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * Check if a substring exists in a string. 
 * @param haystack string to search in
 * @param needle substring to search for
 * @return true is substring needle is in haystack, false otherwise
*/
bool startsWith(const char *haystack, const char *needle)
{
    return strstr(haystack, needle) == haystack;
}

/**
 * Print a single row of divider text to separate sections. 
*/
void printDivider()
{
    printf("---------------------------------------\n");
}