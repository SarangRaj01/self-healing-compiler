/**
 * ==============================================================================
 * Self-Healing Compiler (SHC) Architecture - Runtime Header
 * BCSE307P: Compiler Design Lab Project
 * Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
 * ==============================================================================
 * Description:
 *   Core runtime definitions for compiler-inserted monitoring probes,
 *   shadow stack management, control-flow integrity (CFI), memory bounds
 *   checking, and the automated self-healing mitigation engine.
 * ==============================================================================
 */

#ifndef SHC_RUNTIME_H
#define SHC_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum sizes and configuration */
#define SHC_MAX_SHADOW_DEPTH      4096
#define SHC_MAX_FUNCTIONS         256
#define SHC_MAX_CFI_TARGETS       64
#define SHC_MAX_BOUNDS_ENTRIES    1024

/* Threat and Anomaly Classification */
typedef enum {
    SHC_ANOMALY_NONE               = 0,
    SHC_ANOMALY_SHADOW_STACK_MISMATCH = 1,
    SHC_ANOMALY_BUFFER_OVERFLOW       = 2,
    SHC_ANOMALY_CFI_VIOLATION         = 3,
    SHC_ANOMALY_USE_AFTER_FREE        = 4,
    SHC_ANOMALY_INTEGER_UNDERFLOW     = 5
} shc_anomaly_type_t;

/* Status codes for checked operations */
typedef enum {
    SHC_SUCCESS             = 0,
    SHC_ERR_OVERFLOW        = -1,
    SHC_ERR_NULL_POINTER    = -2,
    SHC_ERR_INVALID_PARAM   = -3
} shc_status_t;

/* Active Mitigation Countermeasures */
typedef enum {
    SHC_MITIGATION_NONE                 = 0,
    SHC_MITIGATION_RERANDOMIZE_LAYOUT   = 1, /* Countermeasure 1: Dynamic function stack/layout re-randomization */
    SHC_MITIGATION_GUARDED_TRAMPOLINE   = 2, /* Countermeasure 2: Control-flow hot-patch injection / sandboxed trampoline */
    SHC_MITIGATION_GRACEFUL_DEGRADE     = 3  /* Fallback: sanitize malformed request and continue */
} shc_mitigation_type_t;

/* Function Variant Definition */
typedef void* (*shc_func_ptr_t)(void* arg);

typedef struct {
    uint32_t func_id;
    const char* func_name;
    void* active_impl;             /* Currently active function pointer (hot-patchable) */
    void* baseline_variant;        /* Default high-performance variant */
    void* rerandomized_variant;    /* Pre-synthesized variant 1: randomized layout/stack */
    void* guarded_variant;         /* Pre-synthesized variant 2: strictly guarded / sandboxed */
    uint32_t execution_count;
    uint32_t anomaly_count;
    shc_mitigation_type_t active_mitigation;
    bool is_isolated;
} shc_func_metadata_t;

/* Runtime Performance and Healing Statistics */
typedef struct {
    uint64_t total_probes_executed;
    uint64_t shadow_stack_pushes;
    uint64_t shadow_stack_pops;
    uint64_t bounds_checks_passed;
    uint64_t bounds_checks_failed;
    uint64_t cfi_checks_passed;
    uint64_t cfi_checks_failed;
    uint64_t anomalies_detected;
    uint64_t mitigations_applied;
    double   last_detect_to_heal_us;  /* Mean Time to Mitigate / Latency in microseconds */
    double   total_mttm_us;
} shc_telemetry_t;

/* --------------------------------------------------------------------------
 * Core Runtime Probes (Inserted by Compiler Pass)
 * -------------------------------------------------------------------------- */

/**
 * Initialize the SHC runtime environment, telemetry, and shadow stack.
 */
void shc_runtime_init(void);

/**
 * Tear down the SHC runtime and print final security & performance audit report.
 */
void shc_runtime_shutdown(void);

/**
 * Register a function and its pre-synthesized mitigation variants with the compiler dispatcher.
 */
void shc_register_function(uint32_t func_id,
                           const char* name,
                           void* baseline,
                           void* rerandomized,
                           void* guarded);

/**
 * Probe: Function Entry
 * Pushes the caller return address or frame token to the protected shadow stack.
 */
void shc_probe_enter(uint32_t func_id, void* ret_addr);

/**
 * Probe: Function Exit
 * Verifies that the actual return address matches the top of the shadow stack.
 * Returns true if valid; if invalid, triggers self-healing mitigation.
 */
bool shc_probe_exit(uint32_t func_id, void* ret_addr);

/**
 * Probe: Memory Buffer Access / Bounds Checking
 * Checks if target buffer operation falls strictly within [base, base + size).
 */
bool shc_probe_bounds(void* target_ptr, void* base_ptr, size_t size, size_t access_len);

/**
 * Probe: Forward Control-Flow Integrity (CFI)
 * Verifies that an indirect call target is within the precomputed valid target set.
 */
bool shc_probe_cfi(uint32_t callsite_id, void* target_func_ptr);

/**
 * Register a valid target for an indirect callsite.
 */
void shc_register_cfi_target(uint32_t callsite_id, void* valid_target);

/**
 * Checked buffer-copy operation: verifies destination capacity before copying.
 * Safe replacement for strcpy/strncpy with runtime bounds tracking.
 * Returns SHC_SUCCESS if copied safely, SHC_ERR_OVERFLOW if source exceeds dst_capacity.
 */
shc_status_t shc_safe_strcpy(char* dst, size_t dst_capacity, const char* src);

/**
 * Checked memory-copy operation: verifies destination capacity before copying.
 * Safe replacement for memcpy.
 * Returns SHC_SUCCESS if copied safely, SHC_ERR_OVERFLOW if count exceeds dst_capacity.
 */
shc_status_t shc_safe_memcpy(void* dst, size_t dst_capacity, const void* src, size_t count);

/* --------------------------------------------------------------------------
 * Self-Healing Mitigation Engine Interface
 * -------------------------------------------------------------------------- */

/**
 * Invoked upon anomaly detection: Diagnoses threat, applies targeted mitigation,
 * splices pre-synthesized trampoline/patch, and calculates MTTM latency.
 * 
 * Returns true if the program state was successfully healed and execution can continue.
 */
bool shc_trigger_healing(uint32_t func_id, shc_anomaly_type_t anomaly_type, const char* context_msg);

/**
 * Get active implementation pointer for dynamic dispatch.
 */
void* shc_get_active_impl(uint32_t func_id);

/**
 * Retrieve current telemetry snapshot.
 */
shc_telemetry_t shc_get_telemetry(void);

/**
 * Reset telemetry counters for benchmarking.
 */
void shc_reset_telemetry(void);

#ifdef __cplusplus
}
#endif

#endif /* SHC_RUNTIME_H */
