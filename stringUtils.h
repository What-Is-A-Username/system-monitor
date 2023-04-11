#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stdbool.h>

/**
 * Check if a substring exists in a string. 
 * @param haystack string to search in
 * @param needle substring to search for
 * @return true is substring needle is in haystack, false otherwise
*/
bool startsWith(const char *haystack, const char *needle);

/**
 * Print a single row of divider text to separate sections. 
*/
extern void printDivider();

#endif