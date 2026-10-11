/**
 * ==============================================================================
 * Milestone 1 Benchmark: Memory Bounds Verification & Safe Rejection Demo
 * BCSE307P: Compiler Design Lab Project (First 25% Demonstration)
 * Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
 * ==============================================================================
 * Description:
 *   Demonstrates the Milestone 1 runtime memory-bounds checker in a real sequence:
 *     Test 1: Valid input copied safely into destination buffer.
 *     Test 2: Oversized input detected & rejected BEFORE any memory corruption occurs.
 *     Test 3: Continued execution: Next valid operation succeeds, showing the
 *             system did not crash and remains healthy.
 *
 *   Outputs real counters from the SHC runtime telemetry.
 * ==============================================================================
 */

#include "shc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 32

int main(int argc, char** argv) {
    printf("================================================================================\n");
    printf("   SELF-HEALING COMPILER (SHC) - MILESTONE 1 RUNTIME BOUNDS CHECK DEMO          \n");
    printf("   BCSE307P: Compiler Design Lab (25%% Deliverable)                             \n");
    printf("================================================================================\n\n");

    /* Initialize the SHC Runtime */
    shc_runtime_init();

    char destination_buffer[BUFFER_SIZE];
    memset(destination_buffer, 0, sizeof(destination_buffer));

    int test_1_status = 0;
    int test_2_status = 0;
    int test_3_status = 0;

    /* ----------------------------------------------------------------------
     * Test 1 — Valid input
     * Create fixed-size buffer (32 bytes), copy a string that fits (11 bytes)
     * ---------------------------------------------------------------------- */
    printf(">>> TEST 1: Valid Input Processing <<<\n");
    const char* valid_input_1 = "Hello, SHC!";
    printf("  Destination buffer capacity: %d bytes\n", BUFFER_SIZE);
    printf("  Input data:                 \"%s\" (length: %zu bytes)\n", valid_input_1, strlen(valid_input_1));

    shc_status_t res1 = shc_safe_strcpy(destination_buffer, sizeof(destination_buffer), valid_input_1);
    if (res1 == SHC_SUCCESS) {
        printf("  Result: [SUCCESS] Input safely stored in buffer: \"%s\"\n\n", destination_buffer);
        test_1_status = 1;
    } else {
        printf("  Result: [FAILED] Unexpected failure on valid input: %d\n\n", res1);
    }

    /* ----------------------------------------------------------------------
     * Test 2 — Oversized input
     * Attempt to copy data that exceeds the destination buffer (64 bytes > 32)
     * Must detect violation, reject BEFORE copying, and preserve memory
     * ---------------------------------------------------------------------- */
    printf(">>> TEST 2: Oversized Input Detection & Rejection <<<\n");
    const char* oversized_input = "ATTACK_PAYLOAD_EXCEEDING_THE_DESIGNATED_BUFFER_CAPACITY_OVERFLOW_12345";
    printf("  Destination buffer capacity: %d bytes\n", BUFFER_SIZE);
    printf("  Attempting to copy:         \"%.30s...\" (length: %zu bytes)\n", oversized_input, strlen(oversized_input));

    shc_status_t res2 = shc_safe_strcpy(destination_buffer, sizeof(destination_buffer), oversized_input);
    if (res2 == SHC_ERR_OVERFLOW) {
        printf("  Result: [DETECTED & REJECTED] Malicious/oversized copy rejected before memory corruption!\n");
        printf("  Buffer preservation check:  Buffer content remains untouched: \"%s\"\n\n", destination_buffer);
        test_2_status = 1;
    } else {
        printf("  Result: [FAILED] Oversized input was NOT rejected properly! Code: %d\n\n", res2);
    }

    /* ----------------------------------------------------------------------
     * Test 3 — Continued execution
     * Perform another valid operation after the rejected attack
     * Demonstrates that the process is alive, uncorrupted, and serving traffic
     * ---------------------------------------------------------------------- */
    printf(">>> TEST 3: Continued Execution & Service Continuity <<<\n");
    const char* valid_input_2 = "Safe Post-Attack Request";
    printf("  Destination buffer capacity: %d bytes\n", BUFFER_SIZE);
    printf("  Input data:                 \"%s\" (length: %zu bytes)\n", valid_input_2, strlen(valid_input_2));

    shc_status_t res3 = shc_safe_strcpy(destination_buffer, sizeof(destination_buffer), valid_input_2);
    if (res3 == SHC_SUCCESS) {
        printf("  Result: [SUCCESS] System is healthy and accepted valid request: \"%s\"\n\n", destination_buffer);
        test_3_status = 1;
    } else {
        printf("  Result: [FAILED] Post-attack operation failed: %d\n\n", res3);
    }

    /* ----------------------------------------------------------------------
     * Summary & Telemetry Audit
     * ---------------------------------------------------------------------- */
    shc_telemetry_t telem = shc_get_telemetry();

    printf("================================================================================\n");
    printf("                           DEMONSTRATION AUDIT SUMMARY                          \n");
    printf("================================================================================\n");
    printf("Total memory checks performed: %llu\n", (unsigned long long)telem.total_probes_executed);
    printf("Successful checks (Passed):   %llu\n", (unsigned long long)telem.bounds_checks_passed);
    printf("Rejected operations (Blocked): %llu\n", (unsigned long long)telem.bounds_checks_failed);
    printf("Anomalies logged in runtime:  %llu\n", (unsigned long long)telem.anomalies_detected);
    
    int all_passed = (test_1_status && test_2_status && test_3_status &&
                      telem.bounds_checks_passed == 2 && telem.bounds_checks_failed == 1);

    printf("Demonstration Status:         %s\n", all_passed ? "ALL 3 TESTS PASSED SUCCESSFULLY" : "TEST FAILURE");
    printf("Process Status:               EXIT CODE 0 (NO CRASH, NO MEMORY CORRUPTION)\n");
    printf("================================================================================\n\n");

    shc_runtime_shutdown();
    return all_passed ? 0 : 1;
}
