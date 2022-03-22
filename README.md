STOMP version 1.0
Author: ryan.scranton@gmail.com (Ryan Scranton)

STOMP is a set of libraries for doing astrostatistical analysis on the
celestial sphere.  The goal is to enable descriptions of arbitrary regions
on the sky which may or may not encode futher spatial information (galaxy
density, CMB temperature, observational depth, etc.) and to do so in such
a way as to make the analysis of that data as algorithmically efficient as
possible.


## to build with python support
follow instructions in morriscb/astro-stomp, but replace any python with python3 in the commandline


# INSTALL

    ./autogen.sh
    ./configure --prefix=[directory]
    make clean
    make
    make install
    cd python
    python3 runswig.py
    python3 setup.py build
    python3 setup.py install --prefix=[directory]
