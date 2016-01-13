Rig-SA
======

A simple simulated-annealing based placement algorithm implemented in C.

Running tests
-------------

Tests use the 'check' library. To build these the following snippet can be
used:

	$ gcc -lm -lcheck -g -Iinclude -o run_tests tests/*.c lib/*.c

Tests should be run under valgrind to ensure any memory leaks are found:

	$ valgrind -q --leak-check=full ./run_tests
