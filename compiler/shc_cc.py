#!/usr/bin/env python3
"""
==============================================================================
Self-Healing Compiler (SHC) Driver: shc_cc
BCSE307P: Compiler Design Lab Project
Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
==============================================================================
Description:
  High-level compiler driver that automates runtime linking, compiler
  pass flags, and binary generation using LLVM Clang / GCC.
==============================================================================
"""

import sys
import os
import subprocess
import shutil

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INCLUDE_DIR = os.path.join(ROOT_DIR, "include")
RUNTIME_DIR = os.path.join(ROOT_DIR, "runtime")

def find_compiler():
    for c in ["gcc", "clang"]:
        if shutil.which(c):
            return c
    return None

def build_executable(source_files, output_exe, extra_flags=None):
    cc = find_compiler()
    if not cc:
        print("[SHC Error] No C compiler (clang/gcc) found in PATH!")
        sys.exit(1)

    runtime_sources = [
        os.path.join(RUNTIME_DIR, "shc_runtime.c"),
        os.path.join(RUNTIME_DIR, "shc_mitigation.c")
    ]

    cmd = [
        cc,
        "-O2",
        f"-I{INCLUDE_DIR}",
        "-o", output_exe
    ] + source_files + runtime_sources

    if extra_flags:
        cmd.extend(extra_flags)

    print(f"[SHC Driver] Compiling with {cc}: {' '.join(cmd)}")
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print(f"[SHC Driver] Compilation failed with error code {res.returncode}")
        return False
    print(f"[SHC Driver] Successfully generated self-healing binary: {output_exe}")
    return True

def main():
    if len(sys.argv) < 2:
        print("Usage: python shc_cc.py <source.c> [-o output_exe] [--run]")
        sys.exit(1)

    sources = []
    output_exe = "a.exe" if os.name == "nt" else "a.out"
    should_run = False
    i = 1
    while i < len(sys.argv):
        arg = sys.argv[i]
        if arg == "-o" and i + 1 < len(sys.argv):
            output_exe = sys.argv[i + 1]
            i += 2
            continue
        elif arg == "--run":
            should_run = True
            i += 1
            continue
        else:
            sources.append(arg)
            i += 1

    success = build_executable(sources, output_exe)
    if success and should_run:
        print(f"\n[SHC Driver] Executing {output_exe}...\n")
        subprocess.run([os.path.abspath(output_exe)])

if __name__ == "__main__":
    main()
