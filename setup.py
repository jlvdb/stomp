#!/usr/bin/env python
import sys
import os
import subprocess

from distutils.core import setup, Extension


def run_swig(swig_file, swig_opts, out_file):
    swig_call = "swig -python {opts:} -o {o:} {i:}".format(
        o=out_file, opts=" ".join(swig_opts), i=swig_file)
    pobj = subprocess.Popen(swig_call, shell=True)
    stdout_ret, stderr_ret = pobj.communicate()


root = os.path.dirname(__file__)

# prepare compiler inputs and flags
include_dirs = ["src"]
depends = []
extra_compile_args = ["-std=c++11"]
swig_opts = ["-c++", "-py3"]
try:
    import numpy
    include_dirs.append(numpy.get_include())
    depends.append("NumpyVector.h")
    extra_compile_args.append("-DWITH_NUMPY3")
    swig_opts.append("-DWITH_NUMPY3")
except ImportError:
    sys.stdout.write("Numpy not found, not building with numpy support\n")

# run swig; don't know how to do this the proper way with distutils but this
# solution works
run_swig("stomp/stomp.i", swig_opts, "stomp/stomp_wrap.cpp")

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
        "stomp/stomp_wrap.cpp"],
    extra_compile_args=extra_compile_args,
    swig_opts=swig_opts)

# read the long description
with open(os.path.join(root, "README.md"), "r") as f:
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
    ext_modules=[stomp_module],
    packages=["stomp"],
    include_dirs=include_dirs)
