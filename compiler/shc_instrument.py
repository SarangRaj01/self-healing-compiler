#!/usr/bin/env python3
"""
==============================================================================
Self-Healing Compiler (SHC) - Source Instrumentation Pass (Milestone 1)
BCSE307P: Compiler Design Lab Project
Authors: Sarang Raj (24BDS0091), Antony Roy (24BCE0920), Dev Raj (24BDS0080)
==============================================================================
Description:
  Demonstrable C source instrumentation pass that detects raw, unsafe buffer
  copy operations (e.g., strcpy(dest, src)) and instruments/transforms them
  into checked runtime calls (shc_safe_strcpy(dest, sizeof(dest), src)).

Supported C Subset & Scope for Milestone 1:
  - Detects fixed-size array declarations of form: char <buf_name>[<size>];
  - Detects standard string copy calls: strcpy(<buf_name>, <src>);
  - Rewrites them to: shc_safe_strcpy(<buf_name>, sizeof(<buf_name>), <src>);
  - Injects #include "shc_runtime.h" header if missing.

Documented Limitations (Milestone 1 Prototype):
  - Does not replace a full C AST parser or LLVM IR pass.
  - Dynamically allocated buffers (malloc/free) require bounds registration tables
    (scheduled for Milestone 2).
  - Pointers passed across compilation units without explicit size attributes
    require fat-pointer or shadow-memory metadata (Milestone 2).
==============================================================================
"""

import sys
import os
import re

def instrument_c_source(source_code):
    """
    Scans C code for known buffer declarations and transforms unsafe strcpy
    calls targeting those buffers into shc_safe_strcpy calls with bounds checking.
    
    Returns:
        transformed_code (str)
        num_transformations (int)
        transformation_log (list of str)
    """
    log = []
    
    # 1. Discover local fixed-size array buffers: e.g. char my_buf[32];
    buffer_decl_pattern = re.compile(r'\bchar\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\[\s*([a-zA-Z0-9_+*\s]+)\s*\]\s*;')
    buffers_found = {}
    for match in buffer_decl_pattern.finditer(source_code):
        buf_name = match.group(1)
        buf_size = match.group(2).strip()
        buffers_found[buf_name] = buf_size
        log.append(f"Discovered bounded buffer: char {buf_name}[{buf_size}]")

    transformed_code = source_code
    num_transformations = 0

    # 2. Transform strcpy(dst, src) -> shc_safe_strcpy(dst, sizeof(dst), src)
    # Match strcpy( <dst> , <src> )
    strcpy_pattern = re.compile(r'\bstrcpy\s*\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*,\s*([^)]+?)\s*\)\s*;')

    def replace_strcpy(match):
        nonlocal num_transformations
        dst = match.group(1).strip()
        src = match.group(2).strip()
        
        num_transformations += 1
        replacement = f"shc_safe_strcpy({dst}, sizeof({dst}), {src});"
        log.append(f"Transformed: strcpy({dst}, {src}) -> {replacement}")
        return replacement

    transformed_code = strcpy_pattern.sub(replace_strcpy, transformed_code)

    # 3. Ensure shc_runtime.h is included if we performed any transformations
    if num_transformations > 0 and 'shc_runtime.h' not in transformed_code:
        header = '#include "shc_runtime.h"\n'
        transformed_code = header + transformed_code
        log.append('Injected: #include "shc_runtime.h"')

    return transformed_code, num_transformations, log

def main():
    if len(sys.argv) < 2 or "-h" in sys.argv or "--help" in sys.argv:
        print("Self-Healing Compiler (SHC) - Source Instrumentation Pass v0.25 (Milestone 1)")
        print("Usage: python shc_instrument.py <input.c> [-o <output_instrumented.c>] [--info]")
        print("\nSupported Transformations:")
        print("  - strcpy(buf, src) -> shc_safe_strcpy(buf, sizeof(buf), src)")
        print("  - Automatic injection of shc_runtime.h")
        sys.exit(0)

    input_file = sys.argv[1]
    if not os.path.exists(input_file):
        print(f"[SHC Instrumentation Error] Input file not found: {input_file}", file=sys.stderr)
        sys.exit(1)

    output_file = None
    show_info = False

    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == "-o" and i + 1 < len(sys.argv):
            output_file = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == "--info":
            show_info = True
            i += 1
        else:
            i += 1

    if not output_file:
        base, ext = os.path.splitext(input_file)
        output_file = f"{base}_instrumented{ext}"

    with open(input_file, "r", encoding="utf-8") as f:
        src = f.read()

    transformed, count, log = instrument_c_source(src)

    with open(output_file, "w", encoding="utf-8") as f:
        f.write(transformed)

    print(f"[SHC Instrumentation Pass] Processed: {input_file} -> {output_file}")
    print(f"[SHC Instrumentation Pass] Applied {count} security transformations.")
    for entry in log:
        print(f"  * {entry}")

if __name__ == "__main__":
    main()
