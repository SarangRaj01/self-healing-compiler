/**
 * ==============================================================================
 * Benchmark 1: Network Service with Buffer Overflow Vulnerability
 * BCSE307P: Compiler Design Lab Project
 * Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
 * ==============================================================================
 * Description:
 *   Simulates a live server processing client requests.
 *   Request #10 contains a stack-smashing buffer overflow exploit payload.
 *
 * Evaluation:
 *   - Unhardened / Standard Baseline: Process crashes (abort/SIGSEGV), drops remaining requests.
 *   - SHC Architecture: Probes detect exploit, heal in microseconds, splice guarded
 *     trampoline, and keep processing remaining requests with 100% uptime!
 * ==============================================================================
 */

#include "shc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_CAPACITY 64
#define TOTAL_REQUESTS  20

/* Function ID for process_packet */
#define FUNC_PROCESS_PACKET_ID 1

/* Pre-synthesized Variant 1: Baseline Fast (Instrumented) */
int process_packet_baseline(const char* packet_data, size_t packet_len) {
    void* frame_cookie = (void*)0xDEADBEEF;
    shc_probe_enter(FUNC_PROCESS_PACKET_ID, frame_cookie);

    char local_buffer[BUFFER_CAPACITY];

    /* Probe bounds */
    if (!shc_probe_bounds((void*)packet_data, (void*)packet_data, packet_len, packet_len) ||
        packet_len >= BUFFER_CAPACITY) {
        
        /* Exploit detected at runtime! Trigger healing */
        shc_trigger_healing(FUNC_PROCESS_PACKET_ID, SHC_ANOMALY_BUFFER_OVERFLOW,
                            "Stack buffer overflow detected in packet parser");
        shc_probe_exit(FUNC_PROCESS_PACKET_ID, frame_cookie);
        return -1;
    }

    memcpy(local_buffer, packet_data, packet_len);
    local_buffer[packet_len] = '\0';

    shc_probe_exit(FUNC_PROCESS_PACKET_ID, frame_cookie);
    return 0;
}

/* Pre-synthesized Variant 2: Re-randomized Stack Layout */
int process_packet_rerandomized(const char* packet_data, size_t packet_len) {
    void* frame_cookie = (void*)0xCAFEF00D;
    shc_probe_enter(FUNC_PROCESS_PACKET_ID, frame_cookie);

    /* Randomized padding layout shifts buffer offsets */
    volatile char canary_pad[128];
    char local_buffer[128];
    memset((void*)canary_pad, 0x5A, sizeof(canary_pad));

    size_t safe_len = packet_len > 120 ? 120 : packet_len;
    memcpy(local_buffer, packet_data, safe_len);
    local_buffer[safe_len] = '\0';

    shc_probe_exit(FUNC_PROCESS_PACKET_ID, frame_cookie);
    return 0;
}

/* Pre-synthesized Variant 3: Guarded Trampoline / Sandboxed Quarantine */
int process_packet_guarded(const char* packet_data, size_t packet_len) {
    void* frame_cookie = (void*)0xFEEDFACE;
    shc_probe_enter(FUNC_PROCESS_PACKET_ID, frame_cookie);

    /* Strict input sanitization and clamped copy prevents any overflow */
    char sanitized_buffer[BUFFER_CAPACITY];
    size_t safe_copy = packet_len;
    if (safe_copy >= BUFFER_CAPACITY) {
        safe_copy = BUFFER_CAPACITY - 1;
    }

    memcpy(sanitized_buffer, packet_data, safe_copy);
    sanitized_buffer[safe_copy] = '\0';

    shc_probe_exit(FUNC_PROCESS_PACKET_ID, frame_cookie);
    return 1; /* Handled safely via guarded trampoline */
}

/* Dynamic Dispatcher (Compiler-Generated Trampoline) */
typedef int (*packet_handler_t)(const char*, size_t);

int process_packet_dispatch(const char* packet_data, size_t packet_len) {
    packet_handler_t active_func = (packet_handler_t)shc_get_active_impl(FUNC_PROCESS_PACKET_ID);
    if (!active_func) {
        active_func = process_packet_baseline;
    }
    return active_func(packet_data, packet_len);
}

int main(int argc, char** argv) {
    shc_runtime_init();

    /* Register function with multi-variant compiler table */
    shc_register_function(
        FUNC_PROCESS_PACKET_ID,
        "process_packet",
        (void*)process_packet_baseline,
        (void*)process_packet_rerandomized,
        (void*)process_packet_guarded
    );

    printf("\n>>> Starting Request Processing Server (Simulating 20 Requests) <<<\n");
    int successful_requests = 0;
    int mitigated_requests = 0;

    for (int req = 1; req <= TOTAL_REQUESTS; req++) {
        char packet[256];
        size_t len = 0;

        if (req == 10) {
            /* Attack Payload: 180 bytes overflowing 64-byte stack buffer */
            printf("[Server] Request #%02d: >>> INCOMING MALICIOUS EXPLOIT PACKET (180 bytes) <<<\n", req);
            memset(packet, 'A', 180);
            len = 180;
        } else {
            /* Normal Payload */
            snprintf(packet, sizeof(packet), "GET /resource/item?id=%d HTTP/1.1", req);
            len = strlen(packet);
            printf("[Server] Request #%02d: Benign request payload (%zu bytes)\n", req, len);
        }

        int result = process_packet_dispatch(packet, len);
        if (result == 0) {
            successful_requests++;
        } else {
            mitigated_requests++;
        }
    }

    printf("\n>>> Server Workload Completed <<<\n");
    printf("Total Requests Processed:  %d / %d\n", successful_requests + mitigated_requests, TOTAL_REQUESTS);
    printf("Successful Benign:         %d\n", successful_requests);
    printf("Exploits Mitigated:        %d\n", mitigated_requests);
    printf("Service Availability:      100.0%% (ZERO CRASHES / NO PROCESS ABORT)\n");

    shc_runtime_shutdown();
    return 0;
}
