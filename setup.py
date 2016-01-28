import io
import re
from setuptools import setup, find_packages

with open("rig_c_sa/version.py", "r") as f:
    exec(f.read())

setup(
    name="rig_c_sa",
    version=__version__,
    packages=find_packages(),
    
    # Files required by CFFI wrapper
    package_data = {'rig_c_sa': ['sa.c', 'sa.h',
                                 'usort/defs.c',
                                 'usort/u1_sort.c',
                                 'usort/u1_sort.h']},

    # Metadata for PyPi
    url="https://github.com/project-rig/rig_c_sa",
    author="The Rig Authors",
    description="A C library (and CFFI Python Interface) for simulated annealing.",
    license="GPLv2",
    classifiers=[
        "Development Status :: 3 - Alpha",

        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",

        "License :: OSI Approved :: GNU General Public License v2 (GPLv2)",

        "Operating System :: POSIX :: Linux",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: MacOS",

        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.4",
        "Programming Language :: Python :: 3.5",

        "Topic :: Software Development :: Libraries",
    ],
    keywords="spinnaker placement cffi simulated-annealing",

    # Build CFFI Interface
    cffi_modules=["rig_c_sa/cffi_compile.py:ffi"],
    setup_requires=["cffi>=1.0.0"],
    install_requires=["cffi>=1.0.0"],
)
