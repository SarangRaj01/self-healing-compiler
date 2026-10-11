/**
 * ==============================================================================
 * Benchmark 3: Benign Workload Performance & False-Positive Rate Evaluation
 * BCSE307P: Compiler Design Lab Project
 * Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
 * ==============================================================================
 * Description:
 *   Executes 500,000 iterations of legitimate memory and control operations.
 *   Measures execution overhead to verify it meets the <10% target, and checks
 *   that False Positive Rate (FPR) is 0.00%.
 * ==============================================================================
 */

#include "shc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
static double get_time_ms(void) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return ((double)count.QuadPart * 1000.0) / (double)freq.QuadPart;
}
#else
#include <time.h>
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((double)ts.tv_sec * 1000.0) + ((double)ts.tv_nsec / 1000000.0);
}
#endif

#define ITERATIONS 500000
#define BUFFER_LEN 128
#define FUNC_BENIGN_WORKER 3

/* Pure Baseline Function without instrumentation */
unsigned int benign_worker_raw(unsigned int seed, const char* src, size_t len) {
    char buf[BUFFER_LEN];
    if (len < BUFFER_LEN) {
        memcpy(buf, src, len);
        buf[len] = '\0';
    }
    unsigned int hash = seed;
    for (size_t i = 0; i < len; ++i) {
        hash = (hash * 33) ^ (unsigned char)buf[i];
    }
    return hash;
}

/* SHC-Instrumented Function */
unsigned int benign_worker_instrumented(unsigned int seed, const char* src, size_t len) {
    void* frame_cookie = (void*)0xCAFEBABE;
    shc_probe_enter(FUNC_BENIGN_WORKER, frame_cookie);

    char buf[BUFFER_LEN];
    if (shc_probe_bounds((void*)src, (void*)src, len, len) && len < BUFFER_LEN) {
        memcpy(buf, src, len);
        buf[len] = '\0';
    }

    unsigned int hash = seed;
    for (size_t i = 0; i < len; ++i) {
        hash = (hash * 33) ^ (unsigned char)buf[i];
    }

    shc_probe_exit(FUNC_BENIGN_WORKER, frame_cookie);
    return hash;
}

int main(int argc, char** argv) {
    const char* sample_text = "CompilerDesignLabBCSE307PSelfHealingCompiler2026";
    size_t sample_len = strlen(sample_text);

    printf("================ BENIGN WORKLOAD EVALUATION (%d ITERATIONS) ================\n", ITERATIONS);

    /* 1. Run Pure Baseline */
    printf("[1/2] Measuring Unhardened Baseline Execution...\n");
    double t_start_base = get_time_ms();
    unsigned int acc_base = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        acc_base ^= benign_worker_raw(acc_base + i, sample_text, sample_len);
    }
    double t_end_base = get_time_ms();
    double base_duration_ms = t_end_base - t_start_base;
    printf("      Baseline Time:      %.2f ms\n", base_duration_ms);

    /* 2. Run SHC Instrumented */
    printf("[2/2] Measuring SHC-Instrumented Execution...\n");
    shc_runtime_init();
    shc_register_function(
        FUNC_BENIGN_WORKER,
        "benign_worker",
        (void*)benign_worker_instrumented,
        (void*)benign_worker_instrumented,
        (void*)benign_worker_instrumented
    );

    double t_start_shc = get_time_ms();
    unsigned int acc_shc = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        acc_shc ^= benign_worker_instrumented(acc_shc + i, sample_text, sample_len);
    }
    double t_end_shc = get_time_ms();
    double shc_duration_ms = t_end_shc - t_start_shc;
    printf("      SHC Time:           %.2f ms\n", shc_duration_ms);

    /* Calculations */
    double overhead_pct = ((shc_duration_ms - base_duration_ms) / base_duration_ms) * 100.0;
    shc_telemetry_t telem = shc_get_telemetry();
    double false_positive_rate = telem.anomalies_detected > 0 ? 
        ((double)telem.anomalies_detected / (double)ITERATIONS) * 100.0 : 0.0;

    printf("\n---------------- RESULTS SUMMARY ----------------\n");
    printf("Unhardened Baseline Time:    %.2f ms\n", base_duration_ms);
    printf("SHC-Instrumented Time:       %.2f ms\n", shc_duration_ms);
    printf("Measured Performance Overhead: %.2f%%\n", overhead_pct);
    printf("Target Overhead Threshold:   < 10.00%%\n");
    printf("Overhead Target Met:         %s\n", overhead_pct < 10.0 ? "YES [PASSED]" : "NO");
    printf("False Positives Flagged:     %llu\n", (unsigned long long)telem.anomalies_detected);
    printf("False Positive Rate (FPR):   %.4f%% [TARGET: 0.0%%]\n", false_positive_rate);
    printf("-------------------------------------------------\n");

    shc_runtime_shutdown();
    return 0;
}
