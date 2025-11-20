================================================================================
MIXAR SENIOR SDE TECHNICAL ASSIGNMENT
UV Unwrapping System
================================================================================

GETTING STARTED
---------------

1. EXTRACT THE PACKAGE
   unzip mixar_assignment_package.zip
   cd mixar_assignment_package

2. READ THE ASSIGNMENT
   - Open ASSIGNMENT.md for complete requirements
   - This README provides setup instructions only

3. VERIFY TEST DATA
   - Check test_data/meshes/ contains 7 OBJ files
   - Check test_data/reference_outputs/ for ground truth

4. BUILD PART 1 (C++)
   cd starter_code/part1_cpp
   mkdir build && cd build
   cmake ..
   make
   ./test_unwrap

   Expected: 10 tests (will fail until you implement)

5. SETUP PART 2 (Python)
   cd starter_code/part2_python
   pip install -r requirements.txt
   python -m pytest tests/

6. TEST PART 3 (Blender)
   - Open Blender (4.2+)
   - Edit > Preferences > Add-ons > Install
   - Select starter_code/part3_blender/
   - Enable "UV: Smart UV Unwrapping"

================================================================================
PACKAGE STRUCTURE
================================================================================

mixar_assignment_package/
├── README.txt                     ← YOU ARE HERE
├── ASSIGNMENT.md                  ← COMPLETE REQUIREMENTS
│
├── test_data/                     ← TEST MESHES & VALIDATION
│   ├── meshes/                    (7 test OBJ files)
│   ├── reference_outputs/         (Expected results)
│   └── validation/                (Comparison tools)
│
└── starter_code/                  ← YOUR IMPLEMENTATION GOES HERE
    ├── part1_cpp/                 (C++ unwrapping engine)
    ├── part2_python/              (Python batch processor)
    └── part3_blender/             (Blender add-on)

================================================================================
WHAT TO IMPLEMENT
================================================================================

PART 1 - C++ ENGINE (70 points, 8-10 hours)
--------------------------------------------
Files you'll implement:
  ✏️  src/topology.cpp          - Build edge connectivity
  ✏️  src/seam_detection.cpp    - Spanning tree + angular defect
  ✏️  src/lscm.cpp              - LSCM parameterization
  ✏️  src/packing.cpp           - Island packing
  ✏️  src/unwrap.cpp            - Main orchestrator

Files provided (DO NOT MODIFY):
  ✅  include/mesh.h             - Mesh data structure
  ✅  include/topology.h         - Topology API
  ✅  src/mesh_io.cpp            - OBJ file I/O
  ✅  src/math_utils.cpp         - Vector math
  ✅  tests/test_unwrap.cpp      - Test suite

Reference materials:
  📚  reference/algorithms.md              - Algorithm descriptions
  📚  reference/lscm_math.md               - Mathematical background
  📚  reference/lscm_matrix_example.cpp    - LSCM example
  📚  reference/topology_example.cpp       - Topology example

PART 2 - PYTHON PROCESSOR (35 points, 4-5 hours)
-------------------------------------------------
Files you'll implement:
  ✏️  uvwrap/bindings.py        - C++ library wrapper
  ✏️  uvwrap/processor.py       - Multi-threaded batch processing
  ✏️  uvwrap/metrics.py         - Quality metrics
  ✏️  uvwrap/optimizer.py       - Parameter optimization
  ✏️  cli.py                    - Command-line interface

Reference materials:
  📚  reference/metrics_spec.md        - Exact metric formulas
  📚  reference/metrics_example.py     - Metric implementation example
  📚  reference/cli_examples.txt       - Expected CLI behavior
  📚  reference/threading_guide.md     - Threading best practices

PART 3 - BLENDER ADD-ON (35 points, 6-8 hours)
-----------------------------------------------
Files you'll implement:
  ✏️  __init__.py               - Add-on registration
  ✏️  operators.py              - Unwrap, batch, seam operators
  ✏️  panels.py                 - UI panel
  ✏️  core/cache.py             - Caching system

Reference materials:
  📚  reference/implementation_guide.md      - Complete guide
  📚  reference/ui_mockup.png                - UI design
  📚  reference/cache_example.py             - Caching example
  📚  reference/direct_mesh_extraction.py    - Mesh data extraction

================================================================================
DEPENDENCIES
================================================================================

PART 1 - C++:
  - CMake 3.15+
  - C++14 compiler (GCC 7+, Clang 6+, MSVC 2017+)
  - Eigen 3.3+ (included in third_party/)

PART 2 - Python:
  - Python 3.8+
  - numpy
  - See requirements.txt for full list

PART 3 - Blender:
  - Blender 4.2+ (download from blender.org)
  - Uses Blender's bundled Python

================================================================================
DEVELOPMENT WORKFLOW
================================================================================

RECOMMENDED ORDER:
1. Part 1: C++ Engine (foundation for everything)
   - Start with topology.cpp (simplest)
   - Then seam_detection.cpp
   - Then lscm.cpp (most complex - budget 4-5 hours)
   - Finally packing.cpp

2. Part 2: Python Processor (builds on Part 1)
   - Start with bindings.py to wrap C++ library
   - Implement metrics.py
   - Add processor.py for multi-threading
   - Build cli.py
   - Add optimizer.py

3. Part 3: Blender Add-on (integrates everything)
   - Start with basic unwrap operator
   - Add UI panel
   - Implement caching
   - Add seam editing
   - Add batch processing
   - Add live preview

TESTING:
- Part 1: Run ./test_unwrap after each implementation
- Part 2: Run pytest tests/ after each module
- Part 3: Test in Blender after each feature

VALIDATION:
cd test_data/validation
python compare_uvs.py \
    ../../starter_code/part1_cpp/build/output/cube.obj \
    ../reference_outputs/cube_lscm.obj

================================================================================
SUBMISSION CHECKLIST
================================================================================

Before submitting, ensure you have:

PART 1:
  ☐  All tests pass (10/10)
  ☐  ALGORITHM.md explains your approach
  ☐  TEST_RESULTS.txt contains test output
  ☐  Code compiles without warnings
  ☐  No memory leaks (run valgrind)

PART 2:
  ☐  All CLI commands work
  ☐  Multi-threading functional
  ☐  Quality metrics match spec
  ☐  README.md has usage examples

PART 3:
  ☐  Add-on installs in Blender
  ☐  All operators functional
  ☐  Caching works correctly
  ☐  demo.blend included
  ☐  Screenshots included

OVERALL:
  ☐  README.md in root with build instructions
  ☐  TIME_LOG.md with time breakdown
  ☐  DESIGN_DECISIONS.md with rationale
  ☐  All code documented
  ☐  Validated against reference outputs

================================================================================
GETTING HELP
================================================================================

QUESTIONS:
- Email: hiring@mixar.com
- Subject: "Assignment Question - [Your Name]"
- Ask within first 72 hours for best response time
- We respond within 1 business day (weekdays)

DEBUGGING:
1. Check reference/ directories for examples
2. Compare your output against reference_outputs/
3. Use validation tools in test_data/validation/
4. Check test output for specific failure messages

COMMON ISSUES:
- "CMake can't find Eigen" → Use bundled version in third_party/
- "Tests segfault" → Check memory management, use valgrind
- "LSCM doesn't converge" → Check boundary conditions, matrix assembly
- "Blender can't import addon" → Check __init__.py has bl_info

================================================================================
RESOURCES
================================================================================

ALGORITHMS:
- starter_code/part1_cpp/reference/algorithms.md
- starter_code/part1_cpp/reference/lscm_math.md

EXAMPLES:
- All reference/ directories contain working examples
- Study these before implementing

VALIDATION:
- test_data/reference_outputs/ - Expected results
- test_data/validation/ - Comparison tools

================================================================================
GOOD LUCK!
================================================================================

This assignment reflects real work at Mixar. We're excited to see your solution!

Tips for success:
  ✓ Start early, ask questions
  ✓ Test incrementally
  ✓ Document as you go
  ✓ Use reference implementations
  ✓ Validate against ground truth

— The Mixar Team

================================================================================
