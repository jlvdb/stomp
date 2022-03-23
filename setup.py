#!/usr/bin/env python
import sys
import os
import subprocess

from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext as _build_ext


class build_ext(_build_ext):

    def run(self):
        swig_call = "swig -python {opts:} -o {o:} {i:}".format(
            o=wrap_file, opts=" ".join(swig_opts), i=swig_file)
        with subprocess.Popen(swig_call, shell=True) as pobj:
            pobj.communicate()
        return super().run()


# compiler inputs
include_dirs = ["src"]
depends = []
swig_file = "stomp/stomp.i"
wrap_file = "stomp/stomp_wrap.cpp"
# compiler flags
extra_compile_args = ["-std=c++11"]
swig_opts = ["-c++", "-py3"]
try:  # check if numpy extension will be used
    import numpy
    # compiler inputs
    include_dirs.append(numpy.get_include())
    depends.append("NumpyVector.h")
    # compiler flags
    extra_compile_args.append("-DWITH_NUMPY3")
    swig_opts.append("-DWITH_NUMPY3")
except ImportError:
    sys.stdout.write("Numpy not found, not building with numpy support\n")

# define the C++ extension module
stomp_module = Extension(
    "stomp._stomp",
    depends=depends,
    sources=[
        "src/stomp/stomp_core.cc",
        "src/stomp/stomp_angular_bin.cc",
        "src/stomp/stomp_radial_bin.cc",
        "src/stomp/stomp_angular_coordinate.cc",
        "src/stomp/stomp_angular_correlation.cc",
        "src/stomp/stomp_radial_correlation.cc",
        "src/stomp/stomp_pixel.cc",
        "src/stomp/stomp_scalar_pixel.cc",
        "src/stomp/stomp_tree_pixel.cc",
        "src/stomp/stomp_itree_pixel.cc",
        "src/stomp/stomp_base_map.cc",
        "src/stomp/stomp_map.cc",
        "src/stomp/stomp_scalar_map.cc",
        "src/stomp/stomp_tree_map.cc",
        "src/stomp/stomp_itree_map.cc",
        "src/stomp/stomp_geometry.cc",
        "src/stomp/stomp_util.cc",
        wrap_file],
    extra_compile_args=extra_compile_args,
    swig_opts=swig_opts)

# read the long description
with open("README.md", "r") as f:
    long_description = f.read()

# define the package metadata
setup(
    name="stomp",
    version="1.0",
    author="Ryan Scranton",
    description="""STOMP is a set of libraries for doing astrostatistical analysis on the celestial sphere.""",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/jlvdb/astro-stomp3",
    cmdclass={'build_ext': build_ext},
    ext_modules=[stomp_module],
    packages=["stomp"],
    include_dirs=include_dirs)
