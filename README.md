`rig_c_sa`: Python C module for the Rig simulated annealing placer C kernel
===========================================================================

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

	$ gcc -lm -lcheck -g -Irig_c_sa -o run_tests tests/*.c rig_c_sa/*.c

The test suite should then be run under valgrind to ensure any memory leaks are found:

	$ valgrind -q --leak-check=full ./run_tests
