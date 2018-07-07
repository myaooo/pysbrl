#!/usr/bin/env python

"""
setup.py file
"""

# import sys
import sysconfig
from setuptools import setup
from setuptools.extension import Extension
from setuptools.command.build_ext import build_ext

libraries = ['gsl', 'gslcblas']

if sysconfig.get_config_var("LIBM") == "-lm":
    libraries.append("m")

include_dirs = ['/usr/local/include', '/usr/include', 'c/']

library_dirs = ['/usr/local/lib', 'usr/lib']

sources = ['c/pysbrl.cpp', 'c/train.cpp', 'c/rulelib.cpp', 'c/save_load.cpp',
           'c/bit_vector.cpp', 'c/utils.cpp', 'swig/pysbrl_wrap.cpp']

pysbrl_module = Extension('_pysbrl',
                          sources=sources,
                          include_dirs=include_dirs,
                          libraries=libraries,
                          library_dirs=library_dirs,
                          swig_opts=['-keyword'],
                          extra_link_args=["-Bstatic"],
                          extra_compile_args=['-std=c++98', '-D DEBUG']
                          )


class CustomBuildExtCommand(build_ext):
    """build_ext command for use when numpy headers are needed."""

    def run(self):
        # Import numpy here, only when headers are needed
        import numpy

        # Add numpy headers to include_dirs
        self.include_dirs.append(numpy.get_include())

        # Call original build_ext command
        build_ext.run(self)


setup(name='pysbrl',
      version='0.4rc',
      author="Yao, Ming",
      description="""A python interface of Scalable Bayesian Rule List""",
      ext_modules=[pysbrl_module],
      url='https://github.com/myaooo/pysbrl',
      py_modules=["pysbrl"],
      requires=['numpy'],
      cmdclass={'build_ext': CustomBuildExtCommand},
      #    extra_link_args = ["-bundle"],
      )
