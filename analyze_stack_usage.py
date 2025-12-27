#!/usr/bin/env python3
"""
Stack Usage Analyzer for STM32 FreeRTOS Project

Analyzes build output (.su stack usage files and .callgraph files) to determine
worst-case stack usage for each FreeRTOS task.

Handles library functions by using realistic worst-case estimates for common
functions like printf, sprintf, etc.
"""

import os
import re
import sys
from pathlib import Path
from typing import Dict, Set, Tuple, Optional, List
from dataclasses import dataclass, field
from collections import defaultdict


@dataclass
class FunctionStackInfo:
    """Stack usage information for a single function."""
    name: str
    stack_usage: int
    is_dynamic: bool = False  # True if stack usage is dynamic/bounded
    is_estimated: bool = False  # True if this is an estimated value
    file_path: str = ""
    line_number: int = 0
    callees: Set[str] = field(default_factory=set)  # Functions this one calls


# Estimated stack usage for common library functions (in bytes)
# These are conservative estimates based on typical ARM implementations
LIBRARY_FUNCTION_ESTIMATES = {
    # Standard I/O
    'printf': 512,
    'sprintf': 256,
    'snprintf': 256,
    'fprintf': 512,
    'vprintf': 512,
    'vsprintf': 256,
    'vsnprintf': 256,

    # String functions
    'strcpy': 32,
    'strncpy': 32,
    'strcat': 32,
    'strncat': 32,
    'strcmp': 32,
    'strncmp': 32,
    'strlen': 16,
    'memcpy': 32,
    'memmove': 32,
    'memset': 32,
    'memcmp': 32,

    # Math functions
    'sqrt': 64,
    'sqrtf': 64,
    'sin': 128,
    'sinf': 128,
    'cos': 128,
    'cosf': 128,
    'atan2': 128,
    'atan2f': 128,
    'pow': 128,
    'powf': 128,
    'exp': 128,
    'expf': 128,
    'log': 128,
    'logf': 128,

    # Memory allocation
    'malloc': 64,
    'free': 64,
    'calloc': 64,
    'realloc': 128,

    # HAL functions (conservative estimates)
    'HAL_UART_Transmit': 64,
    'HAL_UART_Receive': 64,
    'HAL_I2C_Master_Transmit': 96,
    'HAL_I2C_Master_Receive': 96,
    'HAL_SPI_Transmit': 64,
    'HAL_SPI_Receive': 64,
    'HAL_CAN_AddTxMessage': 64,
    'HAL_GPIO_WritePin': 16,
    'HAL_GPIO_ReadPin': 16,
    'HAL_Delay': 32,

    # FreeRTOS API
    'osDelay': 32,
    'osThreadNew': 64,
    'osMessageQueuePut': 64,
    'osMessageQueueGet': 64,
    'osMutexAcquire': 48,
    'osMutexRelease': 48,
    'osEventFlagsSet': 48,
    'osEventFlagsWait': 48,

    # Default for unknown functions
    '__unknown__': 128,
}


class StackUsageAnalyzer:
    """Analyzes stack usage for FreeRTOS tasks."""

    def __init__(self, build_dir: str = "build"):
        self.build_dir = Path(build_dir)
        self.functions: Dict[str, FunctionStackInfo] = {}
        self.task_entry_points: Dict[str, str] = {}  # task_name -> function_name
        self.max_stack_cache: Dict[Tuple[str, frozenset], int] = {}  # Memoization

    def parse_su_files(self):
        """Parse all .su (stack usage) files in the build directory."""
        su_files = list(self.build_dir.rglob("*.su"))

        if not su_files:
            print(f"Warning: No .su files found in {self.build_dir}")
            print("Make sure the project is built with -fstack-usage flag")
            return

        print(f"Found {len(su_files)} .su files")

        # Pattern: filename:line:column:function_name	stack_size	qualifier
        # Example: main.c:123:1:myFunction	64	static
        pattern = re.compile(r'^(.+?):(\d+):(\d+):(\S+)\s+(\d+)\s+(\S+)')

        for su_file in su_files:
            try:
                with open(su_file, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if not line:
                            continue

                        match = pattern.match(line)
                        if match:
                            file_path, line_num, col, func_name, stack_size, qualifier = match.groups()

                            # Clean up function name (remove .part, .constprop, etc.)
                            func_name_clean = self._clean_function_name(func_name)

                            is_dynamic = qualifier in ['dynamic', 'bounded']

                            if func_name_clean not in self.functions:
                                self.functions[func_name_clean] = FunctionStackInfo(
                                    name=func_name_clean,
                                    stack_usage=int(stack_size),
                                    is_dynamic=is_dynamic,
                                    file_path=file_path,
                                    line_number=int(line_num)
                                )
                            else:
                                # If we see the same function multiple times, take the max
                                existing = self.functions[func_name_clean]
                                if int(stack_size) > existing.stack_usage:
                                    existing.stack_usage = int(stack_size)
                                    existing.is_dynamic = existing.is_dynamic or is_dynamic
            except Exception as e:
                print(f"Error parsing {su_file}: {e}")

        print(f"Parsed {len(self.functions)} unique functions")

    def parse_callgraph_files(self):
        """Parse .callgraph files to build function call relationships."""
        callgraph_files = list(self.build_dir.rglob("*.callgraph"))

        if not callgraph_files:
            print(f"Warning: No .callgraph files found in {self.build_dir}")
            print("Callgraph analysis will be limited")
            return

        print(f"Found {len(callgraph_files)} .callgraph files")

        # Pattern for callgraph: caller calls callee
        # Format varies:
        # - "caller/123" -> "callee/456"  (with quotes)
        # - caller/123 -> callee/456      (without quotes)
        # - caller -> callee              (simple format)
        pattern1 = re.compile(r'"([^"]+?)(?:/\d+)?"\s*->\s*"([^"]+?)(?:/\d+)?"')  # Quoted
        pattern2 = re.compile(r'(\S+?)(?:/\d+)?\s*->\s*(\S+?)(?:/\d+)?')         # Unquoted

        for cg_file in callgraph_files:
            try:
                with open(cg_file, 'r') as f:
                    for line in f:
                        line = line.strip()

                        # Try quoted pattern first
                        match = pattern1.search(line)
                        if not match:
                            # Try unquoted pattern
                            match = pattern2.search(line)

                        if match:
                            caller, callee = match.groups()
                            caller = self._clean_function_name(caller)
                            callee = self._clean_function_name(callee)

                            if caller in self.functions:
                                self.functions[caller].callees.add(callee)
            except Exception as e:
                print(f"Error parsing {cg_file}: {e}")

        # Count functions with callees
        funcs_with_callees = sum(1 for f in self.functions.values() if f.callees)
        print(f"Found call relationships for {funcs_with_callees} functions")

    def identify_task_entry_points(self):
        """Identify FreeRTOS task entry point functions."""
        # Common patterns for task names
        task_patterns = [
            r'(\w+Task)',           # e.g., BlinkTask, TSCTask
            r'(\w+Thread)',         # e.g., MainThread
            r'(vTask\w+)',          # FreeRTOS convention
            r'(Start\w+Task)',      # e.g., StartDefaultTask
        ]

        for func_name in self.functions.keys():
            for pattern in task_patterns:
                if re.match(pattern, func_name):
                    # Extract a friendly task name
                    task_name = func_name
                    self.task_entry_points[task_name] = func_name

        print(f"\nIdentified {len(self.task_entry_points)} task entry points:")
        for task_name in sorted(self.task_entry_points.keys()):
            print(f"  - {task_name}")

    def get_function_stack_usage(self, func_name: str) -> int:
        """Get stack usage for a function, using estimates for library functions."""
        if func_name in self.functions:
            return self.functions[func_name].stack_usage

        # Check for exact match in library estimates
        if func_name in LIBRARY_FUNCTION_ESTIMATES:
            return LIBRARY_FUNCTION_ESTIMATES[func_name]

        # Check for partial matches (e.g., printf.isra.0 matches printf)
        for lib_func, estimate in LIBRARY_FUNCTION_ESTIMATES.items():
            if func_name.startswith(lib_func):
                # Add estimated function to our database
                if func_name not in self.functions:
                    self.functions[func_name] = FunctionStackInfo(
                        name=func_name,
                        stack_usage=estimate,
                        is_estimated=True
                    )
                return estimate

        # Unknown function - use default estimate
        estimate = LIBRARY_FUNCTION_ESTIMATES['__unknown__']
        if func_name not in self.functions:
            self.functions[func_name] = FunctionStackInfo(
                name=func_name,
                stack_usage=estimate,
                is_estimated=True
            )
        return estimate

    def calculate_max_stack_usage(self, func_name: str, visited: Optional[Set[str]] = None) -> Tuple[int, List[str]]:
        """
        Calculate maximum stack usage for a function including all callees.
        Returns (max_stack_usage, call_path).
        """
        if visited is None:
            visited = set()

        # Check memoization cache
        cache_key = (func_name, frozenset(visited))
        if cache_key in self.max_stack_cache:
            return self.max_stack_cache[cache_key], []

        # Detect recursion
        if func_name in visited:
            print(f"  Warning: Recursion detected at {func_name}")
            return 0, [f"<RECURSION: {func_name}>"]

        visited.add(func_name)

        # Get this function's stack usage
        own_stack = self.get_function_stack_usage(func_name)

        # Get callees
        callees = set()
        if func_name in self.functions:
            callees = self.functions[func_name].callees

        # If no callees, just return own stack
        if not callees:
            result = own_stack
            self.max_stack_cache[cache_key] = result
            return result, [func_name]

        # Find the callee path with maximum stack usage
        max_callee_stack = 0
        max_callee_path = []

        for callee in callees:
            callee_stack, callee_path = self.calculate_max_stack_usage(callee, visited.copy())
            if callee_stack > max_callee_stack:
                max_callee_stack = callee_stack
                max_callee_path = callee_path

        total_stack = own_stack + max_callee_stack
        full_path = [func_name] + max_callee_path

        self.max_stack_cache[cache_key] = total_stack
        return total_stack, full_path

    def analyze_task_stacks(self) -> Dict[str, Tuple[int, List[str]]]:
        """Analyze worst-case stack usage for all identified tasks."""
        results = {}

        print("\n" + "="*80)
        print("ANALYZING TASK STACK USAGE")
        print("="*80)

        for task_name, func_name in sorted(self.task_entry_points.items()):
            print(f"\nAnalyzing: {task_name}")
            max_stack, path = self.calculate_max_stack_usage(func_name)
            results[task_name] = (max_stack, path)

            print(f"  Worst-case stack usage: {max_stack} bytes")
            if len(path) <= 5:
                print(f"  Call path: {' -> '.join(path)}")
            else:
                print(f"  Call path: {' -> '.join(path[:3])} -> ... -> {path[-1]}")

        return results

    def generate_report(self, results: Dict[str, Tuple[int, List[str]]]):
        """Generate a detailed report of stack usage analysis."""
        print("\n" + "="*80)
        print("STACK USAGE ANALYSIS REPORT")
        print("="*80)

        # Read task stack allocations from common.h if available
        task_allocations = self._read_task_stack_allocations()

        print("\n{:<25} {:>15} {:>15} {:>15}".format(
            "Task Name", "Worst-Case (B)", "Allocated (B)", "Margin (B)"
        ))
        print("-" * 80)

        total_allocated = 0
        total_used = 0

        for task_name in sorted(results.keys()):
            max_stack, path = results[task_name]
            allocated = task_allocations.get(task_name, 0) * 4  # Words to bytes
            margin = allocated - max_stack if allocated > 0 else 0

            total_allocated += allocated
            total_used += max_stack

            status = ""
            if allocated > 0:
                if margin < 0:
                    status = " ⚠️ OVERFLOW!"
                elif margin < 128:
                    status = " ⚠️ Tight"
                elif margin > allocated * 0.5:
                    status = " ✓ Generous"

            print("{:<25} {:>15} {:>15} {:>15}{}".format(
                task_name,
                max_stack,
                allocated if allocated > 0 else "Unknown",
                margin if allocated > 0 else "N/A",
                status
            ))

        if total_allocated > 0:
            print("-" * 80)
            print("{:<25} {:>15} {:>15} {:>15}".format(
                "TOTAL",
                total_used,
                total_allocated,
                total_allocated - total_used
            ))
            print(f"\nOverall stack efficiency: {(total_used/total_allocated)*100:.1f}% utilized")

        # Detailed call paths
        print("\n" + "="*80)
        print("DETAILED CALL PATHS (Worst-Case)")
        print("="*80)

        for task_name in sorted(results.keys()):
            max_stack, path = results[task_name]
            print(f"\n{task_name}: {max_stack} bytes")
            print("-" * 40)

            cumulative = 0
            for i, func in enumerate(path[:10]):  # Limit to first 10 functions
                func_stack = self.get_function_stack_usage(func)
                cumulative += func_stack

                indent = "  " * i
                estimated_mark = " [EST]" if (func in self.functions and
                                              self.functions[func].is_estimated) else ""
                print(f"{indent}{func}: {func_stack} bytes (cumulative: {cumulative}){estimated_mark}")

            if len(path) > 10:
                print(f"  ... and {len(path) - 10} more functions")

    def _read_task_stack_allocations(self) -> Dict[str, int]:
        """Read task stack size allocations from common.h."""
        allocations = {}
        common_h = Path("Core/Src/common.h")

        if not common_h.exists():
            return allocations

        try:
            with open(common_h, 'r') as f:
                content = f.read()

            # Pattern: #define TASKNAME_STACK_SIZE 128
            pattern = re.compile(r'#define\s+(\w+)_STACK_SIZE\s+(\d+)')

            for match in pattern.finditer(content):
                task_prefix, size = match.groups()
                # Try to match with known task names
                for task_name in self.task_entry_points.keys():
                    if task_name.upper().startswith(task_prefix):
                        allocations[task_name] = int(size)
                        break
                    elif task_prefix.lower() in task_name.lower():
                        allocations[task_name] = int(size)
                        break
        except Exception as e:
            print(f"Warning: Could not read task allocations from common.h: {e}")

        return allocations

    @staticmethod
    def _clean_function_name(name: str) -> str:
        """Clean up compiler-mangled function names."""
        # Remove common compiler suffixes
        for suffix in ['.part', '.isra', '.constprop', '.cold', '.lto_priv']:
            if suffix in name:
                name = name.split(suffix)[0]

        # Remove /node_id notation
        if '/' in name:
            name = name.split('/')[0]

        return name


def main():
    """Main entry point."""
    import argparse

    parser = argparse.ArgumentParser(
        description='Analyze worst-case stack usage for FreeRTOS tasks',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    # Analyze using default build/ directory
  %(prog)s -b mybuild         # Use custom build directory
  %(prog)s --tasks TSCTask    # Analyze specific task only
        """
    )
    parser.add_argument('-b', '--build-dir', default='build',
                       help='Build directory containing .su and .callgraph files')
    parser.add_argument('-t', '--tasks', nargs='+',
                       help='Specific task names to analyze (default: all)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')

    args = parser.parse_args()

    # Check build directory exists
    if not Path(args.build_dir).exists():
        print(f"Error: Build directory '{args.build_dir}' not found")
        print("Please build the project first with: make -j20")
        return 1

    print("Stack Usage Analyzer for STM32 FreeRTOS")
    print("=" * 80)

    analyzer = StackUsageAnalyzer(args.build_dir)

    # Parse build artifacts
    print("\nParsing build artifacts...")
    analyzer.parse_su_files()
    analyzer.parse_callgraph_files()

    # Identify tasks
    analyzer.identify_task_entry_points()

    if not analyzer.task_entry_points:
        print("\nError: No FreeRTOS tasks found!")
        print("Make sure the project has been built.")
        return 1

    # Filter tasks if specified
    if args.tasks:
        filtered_tasks = {}
        for task in args.tasks:
            if task in analyzer.task_entry_points:
                filtered_tasks[task] = analyzer.task_entry_points[task]
            else:
                print(f"Warning: Task '{task}' not found")
        analyzer.task_entry_points = filtered_tasks

    # Analyze stack usage
    results = analyzer.analyze_task_stacks()

    # Generate report
    analyzer.generate_report(results)

    print("\n" + "="*80)
    print("Analysis complete!")
    print("="*80)

    return 0


if __name__ == '__main__':
    sys.exit(main())
