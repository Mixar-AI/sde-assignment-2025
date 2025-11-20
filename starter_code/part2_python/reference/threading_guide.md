# Threading and Parallel Processing Guide

## Overview

This guide covers best practices for implementing multi-threaded batch processing in the UV unwrapping CLI tool. The goal is to efficiently process multiple meshes in parallel while avoiding common pitfalls.

## Why Multi-threading?

UV unwrapping is CPU-intensive. When processing hundreds of meshes:
- **Sequential**: 100 meshes × 0.5s = 50 seconds
- **Parallel (8 cores)**: 100 meshes / 8 × 0.5s ≈ 6.25 seconds (8x speedup)

## Python Threading Options

### 1. ThreadPoolExecutor (RECOMMENDED)

✅ **Best choice for I/O-bound tasks and calling C libraries**

```python
from concurrent.futures import ThreadPoolExecutor, as_completed
import os

def process_mesh(input_path, output_path):
    """Process a single mesh (thread-safe)."""
    # Load mesh
    mesh = load_obj(input_path)

    # Unwrap (calls C library via ctypes)
    result = unwrap_mesh(mesh)

    # Save result
    save_obj(output_path, result)

    return {
        'input': input_path,
        'success': True,
        'time': result.time
    }

def batch_process(input_dir, output_dir, num_threads=4):
    """Process all meshes in directory using thread pool."""

    # Find all .obj files
    input_files = [
        os.path.join(input_dir, f)
        for f in os.listdir(input_dir)
        if f.endswith('.obj')
    ]

    os.makedirs(output_dir, exist_ok=True)

    # Create thread pool
    with ThreadPoolExecutor(max_workers=num_threads) as executor:
        # Submit all tasks
        futures = {}
        for input_path in input_files:
            filename = os.path.basename(input_path)
            output_path = os.path.join(output_dir, filename)

            future = executor.submit(process_mesh, input_path, output_path)
            futures[future] = filename

        # Collect results as they complete
        results = []
        for future in as_completed(futures):
            filename = futures[future]
            try:
                result = future.result()
                results.append(result)
                print(f"✓ {filename} ({result['time']:.2f}s)")
            except Exception as e:
                print(f"✗ {filename}: {e}")
                results.append({'input': filename, 'success': False})

    return results
```

**Advantages:**
- Simple API
- Works well with C extension calls (releases GIL)
- Good for I/O-bound operations (file loading/saving)
- Automatic resource cleanup

**Disadvantages:**
- Subject to Python GIL for pure Python code
- Not ideal for CPU-bound pure Python computations

### 2. ProcessPoolExecutor (Alternative)

⚠️ **Use for CPU-intensive pure Python operations**

```python
from concurrent.futures import ProcessPoolExecutor

def batch_process_multiprocess(input_dir, output_dir, num_processes=4):
    """Process meshes using multiple processes."""

    input_files = [...]  # Same as above

    with ProcessPoolExecutor(max_workers=num_processes) as executor:
        futures = {
            executor.submit(process_mesh, inp, out): inp
            for inp, out in zip(input_files, output_files)
        }

        for future in as_completed(futures):
            result = future.result()
            # Handle result
```

**Advantages:**
- True parallelism (bypasses GIL)
- Better for CPU-intensive pure Python code

**Disadvantages:**
- Higher overhead (process spawning)
- Cannot share memory easily
- Slower for I/O operations
- Not needed if calling C library (which releases GIL)

### 3. Threading Module (Manual)

❌ **Not recommended** - use ThreadPoolExecutor instead

```python
import threading

# Don't use this approach for batch processing
# It's more verbose and error-prone
```

## Our Use Case: Which to Choose?

**Use ThreadPoolExecutor** because:
1. Our heavy computation is in C library (via ctypes)
2. C code releases GIL during processing
3. We do I/O (reading/writing OBJ files)
4. ThreadPoolExecutor has simpler API
5. Lower overhead than ProcessPoolExecutor

## Implementation Details

### Optimal Thread Count

```python
import os

def get_optimal_thread_count():
    """Determine optimal number of threads."""
    cpu_count = os.cpu_count() or 4

    # Use CPU count for C library work
    # For pure I/O, could use 2x CPU count
    return cpu_count

# Usage
num_threads = get_optimal_thread_count()
```

**Guidelines:**
- Default: `os.cpu_count()`
- Maximum: `min(num_files, 2 * cpu_count)`
- Minimum: 1 (for small batches)

### Progress Tracking

```python
from concurrent.futures import ThreadPoolExecutor, as_completed
from tqdm import tqdm

def batch_process_with_progress(input_files, output_files, num_threads=4):
    """Process with progress bar."""

    with ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = {
            executor.submit(process_mesh, inp, out): (inp, out)
            for inp, out in zip(input_files, output_files)
        }

        # Create progress bar
        with tqdm(total=len(futures), desc="Processing") as pbar:
            for future in as_completed(futures):
                try:
                    result = future.result()
                    pbar.update(1)
                    pbar.set_postfix(last=os.path.basename(result['input']))
                except Exception as e:
                    pbar.update(1)
                    print(f"\nError: {e}")

    return results
```

### Error Handling

```python
def safe_process_mesh(input_path, output_path):
    """Process mesh with error handling."""
    try:
        result = process_mesh(input_path, output_path)
        return {
            'input': input_path,
            'output': output_path,
            'success': True,
            'metrics': result.metrics
        }
    except FileNotFoundError as e:
        return {
            'input': input_path,
            'success': False,
            'error': f'File not found: {e}'
        }
    except ValueError as e:
        return {
            'input': input_path,
            'success': False,
            'error': f'Invalid mesh data: {e}'
        }
    except Exception as e:
        return {
            'input': input_path,
            'success': False,
            'error': f'Unexpected error: {e}'
        }
```

### Timeout Handling

```python
from concurrent.futures import TimeoutError

def batch_process_with_timeout(input_files, output_files,
                               num_threads=4, timeout=60):
    """Process with per-mesh timeout."""

    with ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = {
            executor.submit(process_mesh, inp, out): inp
            for inp, out in zip(input_files, output_files)
        }

        for future in as_completed(futures, timeout=timeout):
            try:
                result = future.result(timeout=timeout)
                print(f"✓ {result['input']}")
            except TimeoutError:
                print(f"✗ {futures[future]}: Timeout after {timeout}s")
            except Exception as e:
                print(f"✗ {futures[future]}: {e}")
```

## Thread Safety Considerations

### ✅ Thread-Safe Operations

These are safe to use in multi-threaded code:

```python
# File I/O (different files)
with open(filepath, 'r') as f:
    data = f.read()

# ctypes calls to C library (releases GIL)
result = unwrap_mesh_c(mesh_ptr, params_ptr)

# Creating new objects
mesh = Mesh(vertices, triangles)

# Local variables (each thread has its own)
local_result = compute_metrics(mesh)
```

### ❌ Not Thread-Safe (Avoid)

```python
# Shared mutable state
global_counter = 0  # Don't do this!

def process_mesh(path):
    global global_counter
    global_counter += 1  # Race condition!
    ...

# Writing to same file from multiple threads
with open('log.txt', 'a') as f:  # Dangerous!
    f.write(message)
```

### ✅ Safe Shared State (if needed)

```python
import threading

# Use locks for shared state
lock = threading.Lock()
shared_results = []

def process_mesh(path):
    result = unwrap_mesh(path)

    with lock:
        shared_results.append(result)
```

**Better approach:** Avoid shared state entirely

```python
# Each thread returns its result
def process_mesh(path):
    return unwrap_mesh(path)

# Collect results from futures
results = [future.result() for future in futures]
```

## Memory Management

### Avoiding Memory Bloat

```python
def batch_process_memory_efficient(input_files, output_files, num_threads=4):
    """Process in chunks to control memory usage."""

    chunk_size = num_threads * 2
    all_results = []

    for i in range(0, len(input_files), chunk_size):
        chunk_inputs = input_files[i:i+chunk_size]
        chunk_outputs = output_files[i:i+chunk_size]

        with ThreadPoolExecutor(max_workers=num_threads) as executor:
            futures = [
                executor.submit(process_mesh, inp, out)
                for inp, out in zip(chunk_inputs, chunk_outputs)
            ]

            results = [f.result() for f in futures]
            all_results.extend(results)

    return all_results
```

## Best Practices Summary

### DO ✅

1. **Use ThreadPoolExecutor** for calling C libraries
2. **Limit thread count** to CPU cores (or 2x for I/O)
3. **Handle errors** in each thread
4. **Use `as_completed()`** for responsive progress updates
5. **Return results** from threads (avoid shared state)
6. **Process in chunks** for large batches
7. **Validate inputs** before submitting to thread pool
8. **Close pools properly** (use `with` statement)

### DON'T ❌

1. **Don't use ProcessPoolExecutor** for C library calls
2. **Don't share mutable state** between threads
3. **Don't create unlimited threads** (thread count = num files)
4. **Don't ignore exceptions** in worker threads
5. **Don't forget timeouts** for long-running operations
6. **Don't write to same files** from multiple threads
7. **Don't use global variables** for coordination

## Example: Complete Implementation

```python
#!/usr/bin/env python3
"""Complete batch processing example."""

import os
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from tqdm import tqdm

def process_single_mesh(input_path, output_path, params):
    """Thread-safe mesh processing function."""
    import time
    start = time.time()

    try:
        # Load mesh
        mesh = load_obj(input_path)

        # Unwrap (C library call - releases GIL)
        result = unwrap_mesh(mesh, params)

        # Save result
        save_obj(output_path, result.mesh_with_uvs)

        elapsed = time.time() - start

        return {
            'input': input_path,
            'output': output_path,
            'success': True,
            'time': elapsed,
            'islands': result.num_islands,
            'stretch': result.max_stretch
        }

    except Exception as e:
        return {
            'input': input_path,
            'success': False,
            'error': str(e)
        }

def batch_unwrap(input_dir, output_dir, num_threads=None, params=None):
    """Batch process all meshes in directory."""

    # Setup
    if num_threads is None:
        num_threads = os.cpu_count()

    if params is None:
        params = UnwrapParams()

    input_path = Path(input_dir)
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)

    # Find input files
    input_files = list(input_path.glob('*.obj'))

    if not input_files:
        print(f"No .obj files found in {input_dir}")
        return []

    print(f"Found {len(input_files)} meshes")
    print(f"Processing with {num_threads} threads...")

    # Process in parallel
    results = []

    with ThreadPoolExecutor(max_workers=num_threads) as executor:
        # Submit all jobs
        futures = {}
        for input_file in input_files:
            output_file = output_path / input_file.name
            future = executor.submit(
                process_single_mesh,
                str(input_file),
                str(output_file),
                params
            )
            futures[future] = input_file.name

        # Collect results with progress bar
        with tqdm(total=len(futures)) as pbar:
            for future in as_completed(futures):
                filename = futures[future]
                result = future.result()
                results.append(result)

                if result['success']:
                    pbar.set_postfix({
                        'last': filename,
                        'islands': result['islands']
                    })
                else:
                    print(f"\n✗ {filename}: {result['error']}")

                pbar.update(1)

    # Summary
    success_count = sum(1 for r in results if r['success'])
    total_time = sum(r['time'] for r in results if r['success'])

    print(f"\nBatch complete!")
    print(f"  Success: {success_count}/{len(results)}")
    print(f"  Total time: {total_time:.2f}s")
    print(f"  Average: {total_time/success_count:.2f}s per mesh")

    return results

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('input_dir')
    parser.add_argument('output_dir')
    parser.add_argument('--threads', type=int, default=None)

    args = parser.parse_args()

    results = batch_unwrap(args.input_dir, args.output_dir, args.threads)
```

## Performance Tips

1. **Profile first**: Measure where time is spent
2. **I/O optimization**: Read/write files efficiently
3. **Memory pools**: Reuse buffers in C library if possible
4. **Batch size**: Process 2-4x thread count at once
5. **CPU affinity**: Let OS handle (don't pin threads)

## Debugging Multi-threaded Code

```python
import logging
import threading

# Setup logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s [%(threadName)-12s] %(message)s'
)

def process_mesh(path):
    thread_id = threading.current_thread().name
    logging.debug(f"Starting: {path}")

    try:
        result = unwrap_mesh(path)
        logging.debug(f"Completed: {path}")
        return result
    except Exception as e:
        logging.error(f"Failed: {path} - {e}")
        raise
```

## Testing

```python
def test_thread_safety():
    """Test that parallel processing gives same results as sequential."""

    input_files = ['test1.obj', 'test2.obj', 'test3.obj']

    # Sequential
    seq_results = [process_mesh(f, 'out/' + f) for f in input_files]

    # Parallel
    with ThreadPoolExecutor(max_workers=3) as executor:
        par_results = list(executor.map(
            lambda f: process_mesh(f, 'out/' + f),
            input_files
        ))

    # Compare
    for seq, par in zip(seq_results, par_results):
        assert seq['stretch'] == par['stretch']
        assert seq['islands'] == par['islands']
```

Good luck with your implementation!
