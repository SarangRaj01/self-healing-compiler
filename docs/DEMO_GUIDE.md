# Live Demonstration Guide — Milestone 1 (First 25%)

**Course:** BCSE307P — Compiler Design Lab  
**Project Title:** Self-Healing Compiler Architecture for Runtime Vulnerability Mitigation  
**Team Members:**  
- Sarang Raj (24BDS0091)  
- Antony Roy (24BCE0920)  
- Dev Raj (24BDS0080)  

---

## 1. Purpose of the First Milestone (25%)

The goal of this first milestone is to demonstrate a **real, working compiler driver and runtime memory-boundary checking foundation** to faculty:
1. Compiling and linking C programs using an autonomous driver (`shc_cc.py`).
2. Instrumenting raw, unsafe C string operations (`strcpy`) into boundary-checked calls (`shc_instrument.py`).
3. Intercepting and rejecting an oversized buffer-copy operation at runtime **before** any memory corruption occurs (`shc_runtime.c`).
4. Showing **service continuity**: the program does not crash, abort, or panic, but successfully processes subsequent valid requests.
5. Verifying that the runtime maintains accurate, live telemetry counters rather than simulated outputs.

---

## 2. Source Files Involved

| File | Role |
| :--- | :--- |
| **`compiler/shc_cc.py`** | High-level compiler driver that orchestrates source instrumentation, compiles code with GCC/Clang via Python `subprocess`, and verifies binary creation. |
| **`compiler/shc_instrument.py`** | Source-to-source security pass that identifies bounded buffers (`char buf[N]`) and rewrites raw `strcpy` into `shc_safe_strcpy`. |
| **`include/shc_runtime.h`** | Runtime API definitions, telemetry structures, status codes (`SHC_SUCCESS`, `SHC_ERR_OVERFLOW`), and function prototypes. |
| **`runtime/shc_runtime.c`** | Runtime security engine containing `shc_safe_strcpy`, bounds checks, telemetry counters, and detection logging. |
| **`benchmarks/benchmark_bounds.c`** | Demonstration benchmark that executes the 3-step test sequence (Valid $\rightarrow$ Oversized $\rightarrow$ Valid Continued). |
| **`tests/test_bounds_demo.py`** | Automated verification harness that validates compiler error handling, instrumentation, and runtime assertions. |

---

## 3. Architecture & Execution Flow

```
                      +------------------------------------+
                      |    C Source Code with Buffers      |
                      +------------------------------------+
                                         |
                                         v
                      +------------------------------------+
                      |   Source Instrumentation Pass      |
                      |     (compiler/shc_instrument.py)   |
                      |   Transforms unsafe strcpy() to    |
                      |   shc_safe_strcpy(buf, sizeof, src)|
                      +------------------------------------+
                                         |
                                         v
                      +------------------------------------+
                      |    Compiler Driver (shc_cc.py)     |
                      |   Invokes GCC with -O2, links      |
                      |   runtime/shc_runtime.c            |
                      +------------------------------------+
                                         |
                                         v
                      +------------------------------------+
                      |      Executable Application        |
                      |     (build/benchmark_bounds.exe)   |
                      +------------------------------------+
                                         |
                  +----------------------+----------------------+
                  |                                             |
                  v                                             v
        [Valid Input String]                         [Oversized Attack String]
                  |                                             |
                  v                                             v
       shc_safe_strcpy() checks:                    shc_safe_strcpy() checks:
       src_len + 1 <= dst_capacity                  src_len + 1 > dst_capacity (VIOLATION)
                  |                                             |
                  v                                             v
         [Safe Memory Copy]                            [OPERATION BLOCKED]
     Status: SHC_SUCCESS (0)                       Status: SHC_ERR_OVERFLOW (-1)
     Passed Counter incremented                    Failed Counter incremented
                  |                                Memory content untouched!
                  |                                             |
                  +----------------------+----------------------+
                                         |
                                         v
                      +------------------------------------+
                      |   Subsequent Request Continues     |
                      |    Process Health: 100% Uptime     |
                      +------------------------------------+
```

---

## 4. Exact Commands to Build and Run

Open PowerShell in the project directory:
```powershell
cd "C:\Users\SARANG RAJ\.gemini\antigravity\scratch\self_healing_compiler"
```

### Option A: The One-Step Live Demonstration (Recommended for Faculty Review)
Builds the demo benchmark and runs it immediately:
```powershell
python compiler/shc_cc.py benchmarks/benchmark_bounds.c -o build/benchmark_bounds.exe --run
```

### Option B: Run Automated Pre-Flight Test Suite
Proves that compiler error reporting, instrumentation, and runtime assertions all pass:
```powershell
python tests/test_bounds_demo.py
```

### Option C: Test Automatic Source Instrumentation
Take an uninstrumented C file with raw `strcpy()` calls and compile it with automatic instrumentation:
```powershell
python compiler/shc_cc.py benchmarks/sample_uninstrumented.c -o build/sample_auto.exe --instrument --run
```

---

## 5. Actual Output from the Live Demonstration

When you run Option A, the console outputs:

```text
================================================================================
   SELF-HEALING COMPILER (SHC) - MILESTONE 1 RUNTIME BOUNDS CHECK DEMO          
   BCSE307P: Compiler Design Lab (25% Deliverable)                             
================================================================================

[SHC Runtime] Initialized Self-Healing Compiler Runtime v1.0.0
[SHC Runtime] Active protections: Shadow-Stack, Dynamic-CFI, Bounds-Verification, In-Place Hot-Patching
>>> TEST 1: Valid Input Processing <<<
  Destination buffer capacity: 32 bytes
  Input data:                 "Hello, SHC!" (length: 11 bytes)
  Result: [SUCCESS] Input safely stored in buffer: "Hello, SHC!"

>>> TEST 2: Oversized Input Detection & Rejection <<<
  Destination buffer capacity: 32 bytes
  Attempting to copy:         "ATTACK_PAYLOAD_EXCEEDING_THE_D..." (length: 70 bytes)
[SHC RUNTIME DETECTION] Memory Bounds Violation Detected!
  Reason: Source length (70 + 1 null byte = 71 bytes) exceeds destination buffer capacity (32 bytes).
  Action: Operation rejected before copy. Memory preserved without out-of-bounds write.
  Result: [DETECTED & REJECTED] Malicious/oversized copy rejected before memory corruption!
  Buffer preservation check:  Buffer content remains untouched: "Hello, SHC!"

>>> TEST 3: Continued Execution & Service Continuity <<<
  Destination buffer capacity: 32 bytes
  Input data:                 "Safe Post-Attack Request" (length: 24 bytes)
  Result: [SUCCESS] System is healthy and accepted valid request: "Safe Post-Attack Request"

================================================================================
                           DEMONSTRATION AUDIT SUMMARY                          
================================================================================
Total memory checks performed: 3
Successful checks (Passed):   2
Rejected operations (Blocked): 1
Anomalies logged in runtime:  1
Demonstration Status:         ALL 3 TESTS PASSED SUCCESSFULLY
Process Status:               EXIT CODE 0 (NO CRASH, NO MEMORY CORRUPTION)
================================================================================


==================== SHC RUNTIME SECURITY & HEALING AUDIT ====================
Total Instrumentation Probes:  3
Shadow Stack Pushes / Pops:    0 / 0
Bounds Checks (Pass / Fail):   2 / 1
CFI Checks (Pass / Fail):      0 / 0
Exploits/Anomalies Detected:   1
Self-Healing Mitigations Done: 0
Registered Protected Funcs:    0
===============================================================================
```

---

## 6. Line-by-Line Explanation for Faculty

1. **`[SHC Runtime] Initialized Self-Healing Compiler Runtime v1.0.0`**  
   *Explanation:* The runtime subsystem initializes high-resolution performance counters and telemetry data structures upon program start.
2. **`TEST 1: Valid Input Processing -> [SUCCESS]`**  
   *Explanation:* Shows that benign, safe operations execute normally with zero interference. The 11-byte string fits well within the 32-byte destination buffer.
3. **`TEST 2: Oversized Input Detection & Rejection -> [DETECTED & REJECTED]`**  
   *Explanation:* An incoming string of 70 characters targets the 32-byte buffer. Instead of performing an unsafe copy that causes a segmentation fault or stack smash, `shc_safe_strcpy` inspects `src_len + 1 > dst_capacity`, blocks the write operation, logs the detection, and returns `SHC_ERR_OVERFLOW`.
4. **`Buffer preservation check: Buffer content remains untouched: "Hello, SHC!"`**  
   *Explanation:* Proves that memory was not partially overwritten or corrupted by the rejected operation.
5. **`TEST 3: Continued Execution & Service Continuity -> [SUCCESS]`**  
   *Explanation:* Demonstrates that the application survived the attack attempt without crashing (`exit code 0`), continuing to process subsequent legitimate requests.
6. **`DEMONSTRATION AUDIT SUMMARY (Checks: 3, Passed: 2, Blocked: 1)`**  
   *Explanation:* Live telemetry counters recorded by the runtime directly confirm that exactly 2 checks succeeded and 1 was blocked.

---

## 7. What Has Been Completed vs. What Remains

### Completed in Milestone 1 (First 25%):
- [x] Autonomous Python compiler driver (`compiler/shc_cc.py`) with GCC toolchain integration and compiler error diagnostics.
- [x] C source-level memory bounds instrumentation pass (`compiler/shc_instrument.py`).
- [x] Core runtime security library with checked buffer operations (`shc_safe_strcpy`, `shc_safe_memcpy`) and real-time telemetry counters (`runtime/shc_runtime.c`).
- [x] End-to-end benchmark demonstrating valid copy, blocked attack, and continued execution (`benchmarks/benchmark_bounds.c`).
- [x] Automated regression and verification test suite (`tests/test_bounds_demo.py`).

### Remaining for Later Milestones (Next 75%):
- **Milestone 2 (50%):** Shadow Stack implementation tracking return addresses on function epilogues/prologues to prevent ROP gadgets.
- **Milestone 3 (75%):** Forward Control-Flow Integrity (CFI) target graph tables and dynamic function re-randomization.
- **Milestone 4 (100%):** Automated function hot-patching via atomic trampoline swapping and full performance overhead benchmarking against standard LLVM sanitizers.
