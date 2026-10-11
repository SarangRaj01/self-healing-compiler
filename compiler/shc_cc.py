#!/usr/bin/env python3
"""
==============================================================================
Self-Healing Compiler (SHC) Driver: shc_cc (Milestone 1)
BCSE307P: Compiler Design Lab Project
Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
==============================================================================
Description:
  Compiler driver for the Self-Healing Compiler project.
  Features:
  1. Accepts C source file(s) via CLI.
  2. Accepts output binary path via `-o`.
  3. Optional `--instrument` flag to run the source instrumentation pass first.
  4. Optional `--run` flag to execute the generated binary.
  5. Invokes available underlying C compiler (GCC/Clang) via Python subprocess.
  6. Accurately reports compiler errors and returns correct exit codes.
  7. Verifies that the output executable was actually created on disk.
==============================================================================
"""

import sys
import os
import subprocess
import shutil

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INCLUDE_DIR = os.path.join(ROOT_DIR, "include")
RUNTIME_DIR = os.path.join(ROOT_DIR, "runtime")
COMPILER_DIR = os.path.join(ROOT_DIR, "compiler")

def find_compiler():
    """
    Finds an available C compiler.
    Prioritizes gcc (MinGW on Windows) then clang.
    """
    for c in ["gcc", "clang"]:
        path = shutil.which(c)
        if path:
            return c, path
    return None, None

def print_help():
    print("================================================================================")
    print("Self-Healing Compiler (SHC) Driver - Version 0.25 (Milestone 1)")
    print("BCSE307P: Compiler Design Lab Project")
    print("================================================================================")
    print("Usage:")
    print("  python compiler/shc_cc.py <source.c> [-o output_exe] [options]")
    print("\nOptions:")
    print("  -o <path>       Specify the output executable name/path")
    print("  --instrument    Run the SHC source instrumentation pass before compilation")
    print("  --run           Execute the binary immediately after successful build")
    print("  --help, -h      Display this help menu")
    print("\nExample:")
    print("  python compiler/shc_cc.py benchmarks/benchmark_bounds.c -o build/demo.exe --run")
    print("================================================================================")

def compile_and_build(source_files, output_exe, do_instrument=False):
    cc_name, cc_path = find_compiler()
    if not cc_name:
        print("[SHC Driver Error] No supported C compiler (gcc or clang) found in PATH!", file=sys.stderr)
        return False, 1

    # Ensure output directory exists
    out_dir = os.path.dirname(os.path.abspath(output_exe))
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir, exist_ok=True)

    sources_to_compile = []

    # Optional source instrumentation pass
    if do_instrument:
        from shc_instrument import instrument_c_source
        for src in source_files:
            if src.endswith(".c"):
                print(f"[SHC Driver] Running security instrumentation on {src}...")
                with open(src, "r", encoding="utf-8") as f:
                    content = f.read()
                transformed, count, log = instrument_c_source(content)
                inst_src = os.path.join(out_dir, f"inst_{os.path.basename(src)}")
                with open(inst_src, "w", encoding="utf-8") as f:
                    f.write(transformed)
                print(f"[SHC Driver] Inserted {count} security bounds checks into {inst_src}")
                sources_to_compile.append(inst_src)
            else:
                sources_to_compile.append(src)
    else:
        sources_to_compile = list(source_files)

    # Runtime sources needed for linking
    runtime_sources = [
        os.path.join(RUNTIME_DIR, "shc_runtime.c"),
        os.path.join(RUNTIME_DIR, "shc_mitigation.c")
    ]

    # Check that runtime sources exist
    for r_src in runtime_sources:
        if not os.path.exists(r_src):
            print(f"[SHC Driver Error] Missing runtime source: {r_src}", file=sys.stderr)
            return False, 1

    # Remove existing output executable if present to verify real creation
    if os.path.exists(output_exe):
        try:
            os.remove(output_exe)
        except OSError:
            pass

    # Build invocation command
    cmd = [
        cc_name,
        "-O2",
        f"-I{INCLUDE_DIR}",
        "-o", output_exe
    ] + sources_to_compile + runtime_sources

    print(f"[SHC Driver] Invoking compiler ({cc_name}): {' '.join(cmd)}")
    
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    if result.returncode != 0:
        print(f"[SHC Driver] Compilation FAILED with exit code {result.returncode}:", file=sys.stderr)
        if result.stderr:
            print(result.stderr, file=sys.stderr)
        if result.stdout:
            print(result.stdout)
        return False, result.returncode

    # Verify that executable was genuinely created
    if not os.path.exists(output_exe):
        print(f"[SHC Driver Error] Compiler returned code 0, but output file was not created: {output_exe}", file=sys.stderr)
        return False, 1

    print(f"[SHC Driver] Compilation SUCCEEDED. Generated binary: {output_exe} ({os.path.getsize(output_exe)} bytes)")
    return True, 0

def main():
    if len(sys.argv) < 2 or "-h" in sys.argv or "--help" in sys.argv:
        print_help()
        sys.exit(0 if len(sys.argv) >= 2 else 1)

    sources = []
    output_exe = "a.exe" if os.name == "nt" else "a.out"
    should_run = False
    do_instrument = False

    i = 1
    while i < len(sys.argv):
        arg = sys.argv[i]
        if arg == "-o" and i + 1 < len(sys.argv):
            output_exe = sys.argv[i + 1]
            i += 2
        elif arg == "--run":
            should_run = True
            i += 1
        elif arg == "--instrument":
            do_instrument = True
            i += 1
        elif arg.startswith("-"):
            print(f"[SHC Driver Warning] Ignoring unknown flag: {arg}")
            i += 1
        else:
            sources.append(arg)
            i += 1

    if not sources:
        print("[SHC Driver Error] No input source files specified!", file=sys.stderr)
        sys.exit(1)

    for src in sources:
        if not os.path.exists(src):
            print(f"[SHC Driver Error] Source file does not exist: {src}", file=sys.stderr)
            sys.exit(1)

    success, code = compile_and_build(sources, output_exe, do_instrument=do_instrument)
    if not success:
        sys.exit(code)

    if should_run:
        print(f"\n[SHC Driver] Executing binary: {output_exe}\n")
        run_res = subprocess.run([os.path.abspath(output_exe)])
        sys.exit(run_res.returncode)

    sys.exit(0)

if __name__ == "__main__":
    main()
