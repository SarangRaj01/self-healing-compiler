/**
 * ==============================================================================
 * Benchmark 2: Indirect Branch / Control-Flow Integrity (CFI) Vulnerability
 * BCSE307P: Compiler Design Lab Project
 * Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
 * ==============================================================================
 * Description:
 *   Simulates a dynamic dispatch system with callback function pointers.
 *   An adversary corrupts a function pointer to hijack control-flow towards
 *   an unauthorized gadget (`unauthorized_admin_shell`).
 * ==============================================================================
 */

#include "shc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CALLSITE_EVENT_DISPATCH 101
#define FUNC_EVENT_ROUTER_ID    2

typedef void (*event_handler_fn)(const char* event_name);

/* Legitimate Targets */
void handle_login(const char* event_name) {
    printf("  [Handler] Successfully processed legitimate event: %s\n", event_name);
}

void handle_logout(const char* event_name) {
    printf("  [Handler] Successfully processed logout event: %s\n", event_name);
}

/* Attacker Target (Should NEVER be invoked by legitimate dispatch) */
void unauthorized_admin_shell(const char* event_name) {
    printf("  [CRITICAL ALERT] UNAUTHORIZED SHELL / PRIVILEGE ESCALATION EXECUTED! Exploit succeeded!\n");
}

/* Safe Sandboxed Fallback Target injected by SHC */
void safe_quarantine_handler(const char* event_name) {
    printf("  [SHC Guarded Gate] Redirected hijacked call safely to quarantine handler for event: %s\n", event_name);
}

/* Router Baseline Implementation */
void route_event_baseline(event_handler_fn target, const char* event_name) {
    void* frame_cookie = (void*)0xBEEF0001;
    shc_probe_enter(FUNC_EVENT_ROUTER_ID, frame_cookie);

    /* Compiler-injected CFI Probe */
    if (!shc_probe_cfi(CALLSITE_EVENT_DISPATCH, (void*)target)) {
        /* Control-Flow Integrity Violation detected! */
        shc_trigger_healing(FUNC_EVENT_ROUTER_ID, SHC_ANOMALY_CFI_VIOLATION,
                            "Illegal indirect jump detected - target not in CFG target set");
        /* Hot-patch to quarantine handler */
        target = safe_quarantine_handler;
    }

    /* Execute indirect call */
    target(event_name);

    shc_probe_exit(FUNC_EVENT_ROUTER_ID, frame_cookie);
}

/* Router Guarded Implementation */
void route_event_guarded(event_handler_fn target, const char* event_name) {
    void* frame_cookie = (void*)0xBEEF0002;
    shc_probe_enter(FUNC_EVENT_ROUTER_ID, frame_cookie);

    /* Enforced gatekeeper */
    safe_quarantine_handler(event_name);

    shc_probe_exit(FUNC_EVENT_ROUTER_ID, frame_cookie);
}

int main(int argc, char** argv) {
    shc_runtime_init();

    /* Register Legitimate Targets in Compiler CFI Call Graph */
    shc_register_cfi_target(CALLSITE_EVENT_DISPATCH, (void*)handle_login);
    shc_register_cfi_target(CALLSITE_EVENT_DISPATCH, (void*)handle_logout);

    /* Register function with multi-variant table */
    shc_register_function(
        FUNC_EVENT_ROUTER_ID,
        "route_event",
        (void*)route_event_baseline,
        (void*)route_event_baseline,
        (void*)route_event_guarded
    );

    printf("\n>>> Testing Indirect Call Dispatcher with CFI Monitoring <<<\n");

    /* Transaction 1: Legitimate login */
    printf("[Test 1] Dispatching legitimate login event:\n");
    route_event_baseline(handle_login, "user_alice_login");

    /* Transaction 2: Legitimate logout */
    printf("[Test 2] Dispatching legitimate logout event:\n");
    route_event_baseline(handle_logout, "user_alice_logout");

    /* Transaction 3: Simulated vtable/pointer corruption attempt */
    printf("[Test 3] Simulating corrupted function pointer -> unauthorized_admin_shell:\n");
    event_handler_fn corrupted_ptr = unauthorized_admin_shell;
    route_event_baseline(corrupted_ptr, "malicious_payload_trigger");

    /* Transaction 4: Subsequent legitimate operation */
    printf("[Test 4] Verifying system remains operational after mitigation:\n");
    route_event_baseline(handle_login, "user_bob_login");

    shc_runtime_shutdown();
    return 0;
}
