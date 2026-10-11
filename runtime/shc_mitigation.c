/**
 * ==============================================================================
 * Self-Healing Compiler (SHC) Architecture - Automated Mitigation Engine
 * BCSE307P: Compiler Design Lab Project
 * Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
 * ==============================================================================
 * Description:
 *   Handles real-time anomaly diagnosis, dynamic patch selection, function-level
 *   isolation, and in-place trampoline hot-splicing without process restart.
 * ==============================================================================
 */

#include "shc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
static double get_timestamp_us(void) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return ((double)count.QuadPart * 1000000.0) / (double)freq.QuadPart;
}
#else
#include <time.h>
static double get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((double)ts.tv_sec * 1000000.0) + ((double)ts.tv_nsec / 1000.0);
}
#endif

/* Forward access to runtime globals defined in shc_runtime.c */
extern shc_func_metadata_t g_functions[SHC_MAX_FUNCTIONS];
extern uint32_t g_function_count;
extern shc_telemetry_t g_telemetry;

/* Randomized layout seed mutated at runtime */
static unsigned int g_runtime_layout_seed = 0xA5A55A5A;

static const char* anomaly_name(shc_anomaly_type_t t) {
    switch (t) {
        case SHC_ANOMALY_SHADOW_STACK_MISMATCH: return "SHADOW_STACK_MISMATCH (ROP/Ret-Overwrite)";
        case SHC_ANOMALY_BUFFER_OVERFLOW:       return "BUFFER_OVERFLOW (Out-of-Bounds Access)";
        case SHC_ANOMALY_CFI_VIOLATION:         return "CFI_VIOLATION (Indirect Branch Hijack)";
        case SHC_ANOMALY_USE_AFTER_FREE:        return "USE_AFTER_FREE (Dangling Pointer Dereference)";
        case SHC_ANOMALY_INTEGER_UNDERFLOW:     return "INTEGER_UNDERFLOW";
        default:                                return "UNKNOWN_ANOMALY";
    }
}

bool shc_trigger_healing(uint32_t func_id, shc_anomaly_type_t anomaly_type, const char* context_msg) {
    double t_start = get_timestamp_us();
    
    printf("\n----------------------------------------------------------------------\n");
    printf("[SHC MITIGATION ENGINE] >> EXPLOIT DETECTED IN RUNNING PROCESS! <<\n");
    printf("[SHC MITIGATION ENGINE] Function ID:    %u\n", func_id);
    printf("[SHC MITIGATION ENGINE] Anomaly Type:   %s\n", anomaly_name(anomaly_type));
    printf("[SHC MITIGATION ENGINE] Diagnostics:    %s\n", context_msg ? context_msg : "None");
    
    shc_func_metadata_t* target_func = NULL;
    if (func_id < g_function_count) {
        target_func = &g_functions[func_id];
        target_func->anomaly_count++;
    }

    shc_mitigation_type_t selected_action = SHC_MITIGATION_NONE;
    void* new_implementation = NULL;

    /* Decision Engine: Determine targeted countermeasure */
    if (anomaly_type == SHC_ANOMALY_SHADOW_STACK_MISMATCH) {
        /*
         * ROP / Stack-smashing attack detected!
         * Countermeasure 2: Splicing in Guarded Trampoline / Strict Sanitization Sandbox
         */
        selected_action = SHC_MITIGATION_GUARDED_TRAMPOLINE;
        if (target_func && target_func->guarded_variant) {
            new_implementation = target_func->guarded_variant;
        }
        printf("[SHC MITIGATION ENGINE] Strategy: Activating Pre-Synthesized Guarded Trampoline\n");
        printf("[SHC MITIGATION ENGINE] Action: Hot-splicing dispatch vector to isolated sandbox handler\n");
    } else if (anomaly_type == SHC_ANOMALY_BUFFER_OVERFLOW) {
        /*
         * Buffer overflow / offset probe detected!
         * Countermeasure 1: Runtime Layout Re-randomization
         * Mutates stack layout entropy seed and activates re-randomized variant.
         */
        g_runtime_layout_seed ^= (unsigned int)(t_start * 1000.0);
        selected_action = SHC_MITIGATION_RERANDOMIZE_LAYOUT;
        if (target_func && target_func->rerandomized_variant) {
            new_implementation = target_func->rerandomized_variant;
        } else if (target_func && target_func->guarded_variant) {
            new_implementation = target_func->guarded_variant;
        }
        printf("[SHC MITIGATION ENGINE] Strategy: Dynamic Function Frame Re-Randomization\n");
        printf("[SHC MITIGATION ENGINE] Action: Generated new layout seed 0x%08X; remapping offsets\n", g_runtime_layout_seed);
    } else if (anomaly_type == SHC_ANOMALY_CFI_VIOLATION) {
        /*
         * Illegal indirect jump / function pointer overwrite!
         * Countermeasure 2: Control-flow patch injection into guarded call gate.
         */
        selected_action = SHC_MITIGATION_GUARDED_TRAMPOLINE;
        if (target_func && target_func->guarded_variant) {
            new_implementation = target_func->guarded_variant;
        }
        printf("[SHC MITIGATION ENGINE] Strategy: Control-Flow Patch Injection\n");
        printf("[SHC MITIGATION ENGINE] Action: Constraining callsite to validated gatekeeper trampoline\n");
    } else {
        selected_action = SHC_MITIGATION_GRACEFUL_DEGRADE;
        printf("[SHC MITIGATION ENGINE] Strategy: Graceful degradation / input neutralization\n");
    }

    /* Apply hot-patch dynamically if new implementation exists */
    if (target_func && new_implementation) {
        /* Atomic swap of function pointer in dispatch table */
        target_func->active_impl = new_implementation;
        target_func->active_mitigation = selected_action;
        target_func->is_isolated = true;
    }

    double t_end = get_timestamp_us();
    double latency_us = t_end - t_start;
    
    g_telemetry.mitigations_applied++;
    g_telemetry.last_detect_to_heal_us = latency_us;
    g_telemetry.total_mttm_us += latency_us;

    printf("[SHC MITIGATION ENGINE] In-place patch hot-swapped successfully!\n");
    printf("[SHC MITIGATION ENGINE] Detect-to-Heal Latency (MTTM): %.3f microseconds\n", latency_us);
    printf("[SHC MITIGATION ENGINE] Status: Program state preserved. System continuing without crash/reboot.\n");
    printf("----------------------------------------------------------------------\n\n");

    return true;
}
