/**
 * Test program containing unsafe strcpy calls to test the SHC instrumentation pass.
 */
#include <stdio.h>
#include <string.h>

int main(void) {
    char user_buffer[20];
    
    /* This raw strcpy should be transformed by shc_instrument.py */
    strcpy(user_buffer, "Short Text");
    printf("Buffer content: %s\n", user_buffer);
    return 0;
}
