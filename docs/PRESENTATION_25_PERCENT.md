# Project Review Presentation — Milestone 1 (First 25%)

**Course:** BCSE307P — Compiler Design Lab  
**Project Title:** Self-Healing Compiler Architecture for Runtime Vulnerability Mitigation  
**Review Stage:** First Review / Milestone 1 (25% Completion)  
**Team Members:**  
- **Sarang Raj** (24BDS0091) — *Problem identification, literature survey & benchmark design*  
- **Antony Roy** (24BCE0920) — *System novelty & compiler driver architecture*  
- **Dev Raj** (24BDS0080) — *Runtime instrumentation & report compilation*  

---

## Slide 1: Problem Statement & Motivation

### The Problem with Modern Compilers:
* Traditional compilers (GCC, Clang, MSVC) harden applications **only once, at build time** (e.g., ASLR, stack canaries, basic CFI).
* When a memory vulnerability (buffer overflow, ROP chain) occurs in deployed software, existing defenses trigger a fatal `abort()` or kernel panic (`SIGSEGV`).
* **Consequence:** The process terminates immediately, causing a **Denial of Service (DoS)** for critical services. Fixing the bug requires an offline recompile, testing, and full redeployment cycle.

### The Self-Healing Compiler (SHC) Vision:
* A compiler that monitors its own generated machine code at runtime, detects exploit-like behavior as it occurs, and applies a **targeted mitigation automatically without crashing the running process**.

---

## Slide 2: Overall System Architecture

The overarching Self-Healing Compiler architecture consists of four interconnected layers:

```
+-------------------------------------------------------------------------------+
| Layer 1: Compiler Frontend Pass & Multi-Variant Synthesis                     |
|  - Injects lightweight memory and control probes                              |
|  - Pre-synthesizes fallback and guarded variants at compile time              |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
| Layer 2: In-Process Runtime Monitoring Engine                                 |
|  - Shadow Stack verification on call/return                                   |
|  - Memory buffer boundary checks (shc_probe_bounds / shc_safe_strcpy)          |
|  - Forward Control-Flow Integrity (CFI) target set checks                     |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
| Layer 3: Anomaly Diagnosis & Self-Healing Engine                              |
|  - Threat classification (Buffer Overflow vs. CFI violation vs. ROP)           |
|  - Countermeasure selection: Dynamic Re-Randomization or Guarded Trampoline   |
|  - In-place atomic function pointer hot-patching                              |
+-------------------------------------------------------------------------------+
                                      |
                                      v
+-------------------------------------------------------------------------------+
| Layer 4: Application Resiliency (Zero Downtime)                               |
|  - Malicious payload neutralized; service continues running without abort()   |
+-------------------------------------------------------------------------------+
```

---

## Slide 3: Scope of Milestone 1 (First 25% Deliverable)

For this first milestone, we focused on building a **rock-solid, verified foundation** comprising:

1. **Autonomous Compiler Driver (`compiler/shc_cc.py`):**
   - Implements a unified CLI wrapper that accepts C source files, manages include paths, links the SHC runtime, invokes the installed compiler via subprocess, and verifies executable creation.
2. **C Source Instrumentation Pass (`compiler/shc_instrument.py`):**
   - Automatically detects fixed-size buffer allocations and rewrites unsafe string copy operations (`strcpy`) into boundary-verified runtime calls (`shc_safe_strcpy`).
3. **Runtime Bounds Checker & Telemetry (`runtime/shc_runtime.c`):**
   - Implements a checked buffer-copy mechanism that verifies destination capacity before copying.
   - Rejects oversized inputs before any out-of-bounds write occurs.
   - Maintains real-time telemetry counters for passed operations, failed bounds checks, and logged anomalies.
4. **Demonstration Benchmark (`benchmarks/benchmark_bounds.c`):**
   - Executes an end-to-end 3-step test scenario: Valid Input $\rightarrow$ Oversized Input (Rejected) $\rightarrow$ Continued Valid Execution.
5. **Automated Verification Test Suite (`tests/test_bounds_demo.py`):**
   - 100% passing tests for compiler error detection, source transformation, and runtime bounds validation.

---

## Slide 4: Milestone 1 Execution Flow

```
   [ C Source Code ]
           |
           v
   [ shc_instrument.py ] ---------> Identifies: char buffer[32]; strcpy(buffer, input);
           |                        Transforms to: shc_safe_strcpy(buffer, sizeof(buffer), input);
           v
   [ shc_cc.py Driver ] ----------> Invokes GCC -O2 with include/ & runtime/
           |
           v
   [ benchmark_bounds.exe ]
           |
           +---> Test 1: "Hello, SHC!" (11 B <= 32 B)  ===> SUCCESS (Copied safely)
           |
           +---> Test 2: Attack String (70 B > 32 B)   ===> DETECTED & BLOCKED
           |                                                (No buffer corruption!)
           |
           +---> Test 3: "Safe Post-Attack" (24 B)     ===> SUCCESS (Continued execution!)
           |
           v
   [ Audit Telemetry ] -----------> Total: 3 | Passed: 2 | Blocked: 1 | Crashes: 0
```

---

## Slide 5: Demonstration Results & Verification

Live test execution on our development environment (Windows 11, MinGW GCC 6.3.0, Python 3.14):

| Test Case | Input Size | Target Capacity | Expected Result | Actual Result | Status |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Test 1: Valid Copy** | 11 bytes | 32 bytes | Accepted | Accepted into buffer | **PASS** |
| **Test 2: Oversized Input** | 70 bytes | 32 bytes | Rejected safely | `SHC_ERR_OVERFLOW` (-1) | **PASS** |
| **Memory Integrity Check** | — | 32 bytes | Buffer preserved | Buffer untouched | **PASS** |
| **Test 3: Post-Attack Continuity** | 24 bytes | 32 bytes | Accepted | Accepted into buffer | **PASS** |
| **Process Survival** | — | — | Process active | Exit Code: 0 (No crash) | **PASS** |

### Automated Test Suite Output (`tests/test_bounds_demo.py`):
```text
[*] Running test: Source Instrumentation Pass Verification ... [PASSED]
[*] Running test: Compiler Driver Error Reporting on Invalid C ... [PASSED]
[*] Running test: End-to-End Build & Run of Bounds Demonstration ... [PASSED]
Results: 3 / 3 tests passed.
```

---

## Slide 6: Current Limitations & Scope Boundaries

To maintain scientific honesty, the following constraints of the Milestone 1 prototype are noted:

1. **Language Subset for Instrumentation:**
   - The current source instrumentation pass operates on fixed-size stack arrays (`char buf[N]`) and direct `strcpy()` operations.
   - Dynamically allocated heap memory (`malloc`/`free`) and pointers passed without size attributes will be addressed in Milestone 2.
2. **Mitigation Scope:**
   - Milestone 1 implements **safe runtime rejection and state preservation** (preventing corruption and continuing execution).
   - Dynamic layout re-randomization and in-memory function trampoline hot-swapping are designed in the architecture and scheduled for Milestones 3 and 4.
3. **Control-Flow Protection:**
   - Return-address shadow stacks and indirect call CFI verification tables will be integrated in Milestone 2.

---

## Slide 7: Roadmap for Remaining Milestones

| Milestone | Target Completion | Key Deliverables |
| :---: | :---: | :--- |
| **Milestone 1** (Current) | **25%** | **Compiler driver (`shc_cc.py`), C source instrumentation pass, runtime bounds-checking API, demo benchmark, and test harness.** |
| **Milestone 2** | **50%** | Shadow stack implementation for function call/return tracking; return-address overwrite detection. |
| **Milestone 3** | **75%** | Forward Control-Flow Integrity (CFI) target graph tables and dynamic function frame layout re-randomization. |
| **Milestone 4** | **100%** | Automated in-place trampoline hot-patching; detect-to-heal latency (MTTM) benchmarking; final lab report submission. |

---

## Slide 8: Summary for Reviewers

- **What works today:** We have a functional, end-to-end working pipeline that compiles, instruments, detects memory boundary violations before they happen, prevents corruption, and keeps the program alive.
- **Why it matters:** Unlike standard compiler security options that crash a program when an attack occurs, our architecture demonstrates that a runtime monitor can intercept the anomaly and allow the service to continue processing legitimate traffic.
- **Ready for demonstration:** The live demonstration can be executed with a single command:
  ```powershell
  python compiler/shc_cc.py benchmarks/benchmark_bounds.c -o build/benchmark_bounds.exe --run
  ```
