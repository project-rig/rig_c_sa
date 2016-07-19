`rig_c_sa`: Python C module for the Rig simulated annealing placer C kernel
===========================================================================

[![PyPi version](https://img.shields.io/pypi/v/rig_c_sa.svg?style=flat)](https://pypi.python.org/pypi/rig_c_sa)
[![Linux and Mac build status](https://travis-ci.org/project-rig/rig_c_sa.svg?branch=master)](https://travis-ci.org/project-rig/rig_c_sa)
[![Windows build status](https://ci.appveyor.com/api/projects/status/apl31daxqh2594kj?svg=true)](https://ci.appveyor.com/project/mossblaser/rig-c-sa)

This Python package contains an efficient set of datastructures and
manipulation functions for implementing a kernel for
[Rig's](https://github.com/project-rig/rig) simulated annealing placement
algorithm with a [wafer-thin](https://www.youtube.com/watch?v=HJZPzQESq_0)
[CFFI](http://cffi.readthedocs.org/) based interface.

The library is not intended for standalone usage and, as a result,
the whole API should be considered "internal" and unstable for public use.

Running tests locally
---------------------

The C library is tested using using the
[check](http://libcheck.github.io/check/) library. The test suite can be built
using the following command:

	$ gcc -std=c99 -g -o run_tests -Irig_c_sa tests/*.c rig_c_sa/sa.c -lm $(pkg-config --cflags --libs check)

The test suite should then be run under valgrind to ensure any memory leaks are found:

	$ valgrind -q --leak-check=full ./run_tests

Continuous Integration and Deployment to PyPI
---------------------------------------------

When committing changeds to GitHub tests (and/or compilation checks) are run
against:

* [Travis CI](https://travis-ci.org/project-rig/rig_c_sa) will test the code on
  Linux and Mac OS X.
* [AppVeyor](https://ci.appveyor.com/project/mossblaser/rig-c-sa) will test the
  code on Windows 32- and 64-bit.

A PyPI release is automatically created when a new tag is pushed to GitHub.
To create a new `rig_c_sa` release, please follow these steps:

* Increment the version number in
  [`rig_c_sa/version.py`](./rig_c_sa/version.py) according to [Semantic
  Versioning 2.0.0](http://semver.org/) in master.
* Ensure that ensure that all tests on Travis and AppVeyor pass (critical!)
* Create a new tag (`git tag -a vX.Y.Z`) which describes the changes since the
  last release.
* Push this to GitHub. Travis and AppVeyor will automatically build and deploy
  the tagged release to [PyPI](https://pypi.python.org/pypi/rig_c_sa). This
  deployment includes:
  - Source distribution
  - Precompiled OS X binaries
  - Precompiled Windows 32- and 64-bit binaries

The ugly workings of this process are described more fully in
[`.travis.yml`](./.travis.yml) and [`appveyor.yml`](./appveyor.yml).
