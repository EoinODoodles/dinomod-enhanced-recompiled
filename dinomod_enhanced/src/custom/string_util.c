#include "recomputils.h"
#include "PR/ultratypes.h"
#include "string_util.h"
#include "macros.h"
#include "sys/print.h"

/* Reverses a string 'str' of length 'len' 
 *
 * Sourced from: https://www.geeksforgeeks.org/cpp/convert-floating-point-number-string/
 */
void reverse(char* str, int len) {
    int i = 0, j = len - 1, temp; 
    while (i < j) { 
        temp = str[i]; 
        str[i] = str[j]; 
        str[j] = temp; 
        i++; 
        j--; 
    } 
} 

/* 
 * Converts a given integer x to string str[]. 
 * d is the number of digits required in the output. 
 * If d is more than the number of digits in x, 
 * then 0s are added at the beginning. 
 *
 * Sourced from: https://www.geeksforgeeks.org/cpp/convert-floating-point-number-string/
 */
int intToStr(int x, char str[], int d) {
    int i = 0; 
    while (x) { 
        str[i++] = (x % 10) + '0'; 
        x = x / 10; 
    } 

    // If number of digits required is more, then 
    // add 0s at the beginning 
    while (i < d) {
        str[i++] = '0'; 
    }

    reverse(str, i); 
    str[i] = '\0'; 
    return i; 
} 

/* 
 * Converts a floating-point/double number to a string.
 *
 * Dinosaur Planet's diPrintf doesn't seem to handle floats by default, 
 * so this is being used as a workaround!
 *
 * Sourced from: https://www.geeksforgeeks.org/cpp/convert-floating-point-number-string/
 */
char* f2s(f32 n) {
    static char string[255];
    int afterpoint = 3;
    u8 isNegative;

    // Clear string
    for (u32 i = 0; i < ARRAYCOUNT(string); i++) {
        string[i] = 0;
    }

    // Handle negative numbers
    if (n < 0) {
        isNegative = TRUE;
        n = -n;
        string[0] = '-';
    } else {
        isNegative = FALSE;
    }

    // Extract integer part 
    int ipart = (int)n; 

    // Extract floating part 
    float fpart = n - (float)ipart; 

    // convert integer part to string 
    int i = intToStr(ipart, &string[isNegative], 0); 

    // check for display option after point 
    if (afterpoint != 0) { 
        string[i + isNegative] = '.'; // add dot 

        // Get the value of fraction part upto given no. 
        // of points after dot. The third parameter 
        // is needed to handle cases like 233.007 
        fpart = fpart * recomp_powf(10, afterpoint); 

        intToStr((int)fpart, string + i + 1 + isNegative, afterpoint); 
    } 

    return string;
}
