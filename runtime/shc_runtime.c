/**
 * ==============================================================================
 * Self-Healing Compiler (SHC) Architecture - Runtime Implementation
 * BCSE307P: Compiler Design Lab Project
 * Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
 * ==============================================================================
 */

#include "shc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
static LARGE_INTEGER g_timer_freq;
static void init_timer(void) {
    QueryPerformanceFrequency(&g_timer_freq);
}
static double get_time_us(void) {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return ((double)count.QuadPart * 1000000.0) / (double)g_timer_freq.QuadPart;
}
#else
#include <time.h>
static void init_timer(void) {}
static double get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((double)ts.tv_sec * 1000000.0) + ((double)ts.tv_nsec / 1000.0);
}
#endif

/* Shadow Stack State */
typedef struct {
    void* return_addresses[SHC_MAX_SHADOW_DEPTH];
    uint32_t func_ids[SHC_MAX_SHADOW_DEPTH];
    int top;
} shc_shadow_stack_t;

static shc_shadow_stack_t g_shadow_stack;
shc_func_metadata_t g_functions[SHC_MAX_FUNCTIONS];
uint32_t g_function_count = 0;

/* Control-Flow Integrity (CFI) Valid Targets Table */
typedef struct {
    uint32_t callsite_id;
    void* valid_targets[SHC_MAX_CFI_TARGETS];
    size_t target_count;
} shc_cfi_callsite_t;

static shc_cfi_callsite_t g_cfi_table[SHC_MAX_FUNCTIONS];
static size_t g_cfi_callsite_count = 0;

/* Telemetry data */
shc_telemetry_t g_telemetry;
static bool g_initialized = false;

void shc_runtime_init(void) {
    if (g_initialized) return;
    init_timer();
    g_shadow_stack.top = -1;
    memset(g_functions, 0, sizeof(g_functions));
    memset(g_cfi_table, 0, sizeof(g_cfi_table));
    memset(&g_telemetry, 0, sizeof(g_telemetry));
    g_function_count = 0;
    g_cfi_callsite_count = 0;
    g_initialized = true;
    
    printf("[SHC Runtime] Initialized Self-Healing Compiler Runtime v1.0.0\n");
    printf("[SHC Runtime] Active protections: Shadow-Stack, Dynamic-CFI, Bounds-Verification, In-Place Hot-Patching\n");
}

void shc_runtime_shutdown(void) {
    if (!g_initialized) return;
    printf("\n==================== SHC RUNTIME SECURITY & HEALING AUDIT ====================\n");
    printf("Total Instrumentation Probes:  %llu\n", (unsigned long long)g_telemetry.total_probes_executed);
    printf("Shadow Stack Pushes / Pops:    %llu / %llu\n", 
           (unsigned long long)g_telemetry.shadow_stack_pushes,
           (unsigned long long)g_telemetry.shadow_stack_pops);
    printf("Bounds Checks (Pass / Fail):   %llu / %llu\n",
           (unsigned long long)g_telemetry.bounds_checks_passed,
           (unsigned long long)g_telemetry.bounds_checks_failed);
    printf("CFI Checks (Pass / Fail):      %llu / %llu\n",
           (unsigned long long)g_telemetry.cfi_checks_passed,
           (unsigned long long)g_telemetry.cfi_checks_failed);
    printf("Exploits/Anomalies Detected:   %llu\n", (unsigned long long)g_telemetry.anomalies_detected);
    printf("Self-Healing Mitigations Done: %llu\n", (unsigned long long)g_telemetry.mitigations_applied);
    if (g_telemetry.mitigations_applied > 0) {
        double avg_mttm = g_telemetry.total_mttm_us / g_telemetry.mitigations_applied;
        printf("Mean Time to Mitigate (MTTM):  %.3f us (detect-to-heal latency)\n", avg_mttm);
    }
    printf("Registered Protected Funcs:    %u\n", g_function_count);
    for (uint32_t i = 0; i < g_function_count; ++i) {
        printf("  - [%u] %-20s Calls: %-6u Anomalies: %-3u Active Variant: %s\n",
               g_functions[i].func_id,
               g_functions[i].func_name,
               g_functions[i].execution_count,
               g_functions[i].anomaly_count,
               g_functions[i].active_mitigation == SHC_MITIGATION_NONE ? "Baseline (Fast)" :
               g_functions[i].active_mitigation == SHC_MITIGATION_RERANDOMIZE_LAYOUT ? "Re-randomized Layout" :
               g_functions[i].active_mitigation == SHC_MITIGATION_GUARDED_TRAMPOLINE ? "Guarded Trampoline" : "Degraded");
    }
    printf("===============================================================================\n\n");
}

void shc_register_function(uint32_t func_id,
                           const char* name,
                           void* baseline,
                           void* rerandomized,
                           void* guarded) {
    if (func_id >= SHC_MAX_FUNCTIONS) return;
    g_functions[func_id].func_id = func_id;
    g_functions[func_id].func_name = name;
    g_functions[func_id].baseline_variant = baseline;
    g_functions[func_id].rerandomized_variant = rerandomized;
    g_functions[func_id].guarded_variant = guarded;
    g_functions[func_id].active_impl = baseline;
    g_functions[func_id].execution_count = 0;
    g_functions[func_id].anomaly_count = 0;
    g_functions[func_id].active_mitigation = SHC_MITIGATION_NONE;
    g_functions[func_id].is_isolated = false;

    if (func_id >= g_function_count) {
        g_function_count = func_id + 1;
    }
}

void shc_probe_enter(uint32_t func_id, void* ret_addr) {
    g_telemetry.total_probes_executed++;
    g_telemetry.shadow_stack_pushes++;
    
    if (func_id < g_function_count) {
        g_functions[func_id].execution_count++;
    }

    if (g_shadow_stack.top < SHC_MAX_SHADOW_DEPTH - 1) {
        g_shadow_stack.top++;
        g_shadow_stack.return_addresses[g_shadow_stack.top] = ret_addr;
        g_shadow_stack.func_ids[g_shadow_stack.top] = func_id;
    } else {
        fprintf(stderr, "[SHC ERROR] Shadow stack capacity overflow!\n");
    }
}

bool shc_probe_exit(uint32_t func_id, void* ret_addr) {
    g_telemetry.total_probes_executed++;
    g_telemetry.shadow_stack_pops++;

    if (g_shadow_stack.top < 0) {
        /* Underflow or corrupted stack */
        shc_trigger_healing(func_id, SHC_ANOMALY_SHADOW_STACK_MISMATCH, "Shadow stack underflow at exit");
        return false;
    }

    void* expected_ret = g_shadow_stack.return_addresses[g_shadow_stack.top];
    uint32_t expected_id = g_shadow_stack.func_ids[g_shadow_stack.top];
    g_shadow_stack.top--;

    if (expected_ret != ret_addr || expected_id != func_id) {
        g_telemetry.anomalies_detected++;
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Return address hijacked! Expected: %p, Found: %p in func_id %u",
                 expected_ret, ret_addr, func_id);
        shc_trigger_healing(func_id, SHC_ANOMALY_SHADOW_STACK_MISMATCH, msg);
        return false;
    }

    return true;
}

bool shc_probe_bounds(void* target_ptr, void* base_ptr, size_t size, size_t access_len) {
    g_telemetry.total_probes_executed++;
    uintptr_t target = (uintptr_t)target_ptr;
    uintptr_t base = (uintptr_t)base_ptr;
    uintptr_t limit = base + size;

    if (target < base || (target + access_len) > limit) {
        g_telemetry.bounds_checks_failed++;
        g_telemetry.anomalies_detected++;
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Out-of-bounds write/read! Access: [%p..%p], Allowed Buffer: [%p..%p] (size %zu)",
                 (void*)target, (void*)(target + access_len), (void*)base, (void*)limit, size);
        shc_trigger_healing(0, SHC_ANOMALY_BUFFER_OVERFLOW, msg);
        return false;
    }

    g_telemetry.bounds_checks_passed++;
    return true;
}

void shc_register_cfi_target(uint32_t callsite_id, void* valid_target) {
    for (size_t i = 0; i < g_cfi_callsite_count; ++i) {
        if (g_cfi_table[i].callsite_id == callsite_id) {
            if (g_cfi_table[i].target_count < SHC_MAX_CFI_TARGETS) {
                g_cfi_table[i].valid_targets[g_cfi_table[i].target_count++] = valid_target;
            }
            return;
        }
    }
    if (g_cfi_callsite_count < SHC_MAX_FUNCTIONS) {
        g_cfi_table[g_cfi_callsite_count].callsite_id = callsite_id;
        g_cfi_table[g_cfi_callsite_count].valid_targets[0] = valid_target;
        g_cfi_table[g_cfi_callsite_count].target_count = 1;
        g_cfi_callsite_count++;
    }
}

bool shc_probe_cfi(uint32_t callsite_id, void* target_func_ptr) {
    g_telemetry.total_probes_executed++;

    for (size_t i = 0; i < g_cfi_callsite_count; ++i) {
        if (g_cfi_table[i].callsite_id == callsite_id) {
            for (size_t j = 0; j < g_cfi_table[i].target_count; ++j) {
                if (g_cfi_table[i].valid_targets[j] == target_func_ptr) {
                    g_telemetry.cfi_checks_passed++;
                    return true;
                }
            }
            /* Target not in precomputed set! */
            g_telemetry.cfi_checks_failed++;
            g_telemetry.anomalies_detected++;
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Illegal indirect branch target %p at callsite %u (CFI violation)",
                     target_func_ptr, callsite_id);
            shc_trigger_healing(callsite_id, SHC_ANOMALY_CFI_VIOLATION, msg);
            return false;
        }
    }

    /* If no targets registered for callsite, allow or enforce */
    g_telemetry.cfi_checks_passed++;
    return true;
}

void* shc_get_active_impl(uint32_t func_id) {
    if (func_id < g_function_count && g_functions[func_id].active_impl != NULL) {
        return g_functions[func_id].active_impl;
    }
    return NULL;
}

shc_telemetry_t shc_get_telemetry(void) {
    return g_telemetry;
}

void shc_reset_telemetry(void) {
    memset(&g_telemetry, 0, sizeof(g_telemetry));
}
