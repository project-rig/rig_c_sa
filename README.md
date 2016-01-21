`rig_c_sa`: Python C module for the Rig simulated annealing placer C kernel
===========================================================================

[![PyPi version](https://img.shields.io/pypi/v/rig_c_sa.svg?style=flat)](https://pypi.python.org/pypi/rig_c_sa)
[![Linux build and test status](https://travis-ci.org/project-rig/rig_c_sa.svg?branch=master)](https://travis-ci.org/project-rig/rig_c_sa)
[![Windows build status](https://ci.appveyor.com/api/projects/status/apl31daxqh2594kj/branch/master?svg=true)](https://ci.appveyor.com/project/mossblaser/rig-c-sa/branch/master)

This Python package contains an efficient set of datastructures and
manipulation functions for implementing a kernel for
[Rig's](https://github.com/project-rig/rig) simulated annealing placement
algorithm with a [wafer-thin](https://www.youtube.com/watch?v=HJZPzQESq_0)
[CFFI](http://cffi.readthedocs.org/) based interface.

The library is not intended for standalone usage and, as a result,
the whole API should be considered "internal" and unstable for public use.

Running tests
-------------

The C library is tested using using the 'check' library. The test suite can be
built using the following command:

	$ gcc -std=c99 -g -o run_tests -Irig_c_sa tests/*.c rig_c_sa/*.c -lm $(pkg-config --cflags --libs check)

The test suite should then be run under valgrind to ensure any memory leaks are found:

	$ valgrind -q --leak-check=full ./run_tests
