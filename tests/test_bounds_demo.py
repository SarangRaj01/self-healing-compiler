#!/usr/bin/env python3
"""
==============================================================================
Milestone 1 Test Suite: Verification for Live College Demonstration
BCSE307P: Compiler Design Lab Project
Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
==============================================================================
Requirements to Verify:
  1. The runtime compiles cleanly.
  2. The demonstration executable builds successfully via shc_cc.py.
  3. The valid-input test passes (Test 1).
  4. The oversized-input test is rejected safely before copying (Test 2).
  5. A valid operation succeeds after the rejected operation (Test 3).
  6. Final counters match the operations performed (2 passed, 1 rejected).
  7. Compiler failures are reported correctly with non-zero exit code.
==============================================================================
"""

import os
import sys
import subprocess
import shutil

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COMPILER_DRIVER = os.path.join(ROOT_DIR, "compiler", "shc_cc.py")
INSTRUMENT_PASS = os.path.join(ROOT_DIR, "compiler", "shc_instrument.py")
BUILD_DIR = os.path.join(ROOT_DIR, "build")
BENCHMARKS_DIR = os.path.join(ROOT_DIR, "benchmarks")

def run_test(name, fn):
    print(f"[*] Running test: {name} ...", end=" ", flush=True)
    try:
        fn()
        print("[PASSED]")
        return True
    except AssertionError as e:
        print(f"[FAILED]\n    Assertion Error: {e}")
        return False
    except Exception as e:
        print(f"[ERROR]\n    Unexpected Exception: {e}")
        return False

def test_compiler_failure_reporting():
    """Verify compiler driver handles invalid C syntax and returns non-zero code."""
    bad_c = os.path.join(BUILD_DIR, "bad_syntax.c")
    bad_exe = os.path.join(BUILD_DIR, "bad_syntax.exe")
    with open(bad_c, "w") as f:
        f.write("int main() { syntax error here; return 0; }\n")

    res = subprocess.run([sys.executable, COMPILER_DRIVER, bad_c, "-o", bad_exe],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    assert res.returncode != 0, f"Expected non-zero exit code, got {res.returncode}"
    assert not os.path.exists(bad_exe), "Executable should not have been created on syntax failure"

def test_build_and_run_demo():
    """Builds and executes benchmark_bounds.c and validates the entire output sequence."""
    demo_src = os.path.join(BENCHMARKS_DIR, "benchmark_bounds.c")
    demo_exe = os.path.join(BUILD_DIR, "test_bounds_demo.exe")

    # 1. Build
    build_res = subprocess.run([sys.executable, COMPILER_DRIVER, demo_src, "-o", demo_exe],
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    assert build_res.returncode == 0, f"Build failed: {build_res.stderr}"
    assert os.path.exists(demo_exe), f"Output binary {demo_exe} does not exist"

    # 2. Run
    run_res = subprocess.run([os.path.abspath(demo_exe)],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    assert run_res.returncode == 0, f"Demo exited with non-zero code {run_res.returncode}: {run_res.stderr}"

    out = run_res.stdout + run_res.stderr

    # 3. Check Test 1 (Valid Input)
    assert ">>> TEST 1: Valid Input Processing <<<" in out, "Test 1 heading missing"
    assert "[SUCCESS] Input safely stored in buffer" in out, "Test 1 success line missing"

    # 4. Check Test 2 (Oversized Input Rejected)
    assert ">>> TEST 2: Oversized Input Detection & Rejection <<<" in out, "Test 2 heading missing"
    assert "Memory Bounds Violation Detected" in out, "Violation detection message missing"
    assert "Buffer content remains untouched" in out, "Buffer was corrupted during oversized copy"

    # 5. Check Test 3 (Continued Execution)
    assert ">>> TEST 3: Continued Execution & Service Continuity <<<" in out, "Test 3 heading missing"
    assert "System is healthy and accepted valid request" in out, "Test 3 failed to accept valid input"

    # 6. Check Audit Telemetry
    assert "Total memory checks performed: 3" in out, "Total checks counter mismatch"
    assert "Successful checks (Passed):   2" in out, "Passed checks counter mismatch"
    assert "Rejected operations (Blocked): 1" in out, "Failed checks counter mismatch"
    assert "ALL 3 TESTS PASSED SUCCESSFULLY" in out, "Demonstration did not finish successfully"

def test_source_instrumentation_pass():
    """Verifies that compiler/shc_instrument.py accurately transforms unsafe strcpy."""
    from shc_instrument import instrument_c_source
    sample = """
    void test() {
        char buf[16];
        strcpy(buf, "Dangerous Payload");
    }
    """
    transformed, count, log = instrument_c_source(sample)
    assert count == 1, f"Expected 1 transformation, got {count}"
    assert "shc_safe_strcpy(buf, sizeof(buf), \"Dangerous Payload\");" in transformed, \
        f"Transformation incorrect: {transformed}"
    assert '#include "shc_runtime.h"' in transformed, "Missing header injection"

def main():
    print("================================================================================")
    print("       SHC MILESTONE 1 VERIFICATION SUITE (LIVE DEMO PRE-FLIGHT CHECKS)         ")
    print("================================================================================")
    os.makedirs(BUILD_DIR, exist_ok=True)
    sys.path.insert(0, os.path.join(ROOT_DIR, "compiler"))

    tests = [
        ("Source Instrumentation Pass Verification", test_source_instrumentation_pass),
        ("Compiler Driver Error Reporting on Invalid C", test_compiler_failure_reporting),
        ("End-to-End Build & Run of Bounds Demonstration", test_build_and_run_demo),
    ]

    passed = 0
    for name, fn in tests:
        if run_test(name, fn):
            passed += 1

    print("\n--------------------------------------------------------------------------------")
    print(f"Results: {passed} / {len(tests)} tests passed.")
    print("--------------------------------------------------------------------------------")
    sys.exit(0 if passed == len(tests) else 1)

if __name__ == "__main__":
    main()
