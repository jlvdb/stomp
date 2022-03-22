#!/usr/bin/env python
import sys
import os
import subprocess

from distutils.core import setup, Extension

with open("README.md", "r") as f:
    long_description = f.read()

os.chdir("python")  # where the swig code is located

# prepare compiler inputs and flags
include_dirs = ["../"]
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

# run swig; don't know how to do this the proper way with setuptools/distutils
# but this solution works
swig_call = "swig -python {:} -o stomp_wrap.cpp stomp.i".format(
    " ".join(swig_opts))
pobj = subprocess.Popen(swig_call, shell=True)
stdout_ret, stderr_ret = pobj.communicate()

# create the ups table
pyvers = "{:}.{:}".format(*sys.version_info[0:2])
dir_lib = "lib/python%s/site-packages" % pyvers
dir_lib64 = "lib64/python%s/site-packages" % pyvers

if not os.path.exists("ups"):
    os.mkdir("ups")
with open('ups/stomp.table','w') as tablefile:
    tablefile.write("""
setupOptional("gflags")
envPrepend(PATH,${{PRODUCT_DIR}}/bin)
envPrepend(LD_LIBRARY_PATH,${{PRODUCT_DIR}}/lib)
envPrepend(LIBRARY_PATH,${{PRODUCT_DIR}}/lib)
envPrepend(C_INCLUDE_PATH,${{PRODUCT_DIR}}/include)
envPrepend(CPATH,${{PRODUCT_DIR}}/include)
envPrepend(PYTHONPATH,${{PRODUCT_DIR}}/{:})
envPrepend(PYTHONPATH,${{PRODUCT_DIR}}/{:})
""".format(dir_lib, dir_lib64))

stomp_module = Extension(
    "_stomp",
    depends=depends,
    sources=[
        "../stomp/stomp_core.cc",
        "../stomp/stomp_angular_bin.cc",
        "../stomp/stomp_radial_bin.cc",
        "../stomp/stomp_angular_coordinate.cc",
        "../stomp/stomp_angular_correlation.cc",
        "../stomp/stomp_radial_correlation.cc",
        "../stomp/stomp_pixel.cc",
        "../stomp/stomp_scalar_pixel.cc",
        "../stomp/stomp_tree_pixel.cc",
        "../stomp/stomp_itree_pixel.cc",
        "../stomp/stomp_base_map.cc",
        "../stomp/stomp_map.cc",
        "../stomp/stomp_scalar_map.cc",
        "../stomp/stomp_tree_map.cc",
        "../stomp/stomp_itree_map.cc",
        "../stomp/stomp_geometry.cc",
        "../stomp/stomp_util.cc",
        "stomp_wrap.cpp"],
    extra_compile_args=extra_compile_args,
    swig_opts=swig_opts)

setup(
    name="stomp",
    version="1.0",
    author="Ryan Scranton",
    description="""STOMP is a set of libraries for doing astrostatistical analysis on the celestial sphere.""",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/jlvdb/astro-stomp3",
    data_files=[('ups', ['ups/stomp.table'])],
    ext_modules=[stomp_module],
    py_modules=["stomp"],
    include_dirs=include_dirs)
