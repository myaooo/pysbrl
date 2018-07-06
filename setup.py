#!/usr/bin/env python

"""
setup.py file
"""

# import sys
import sysconfig
from setuptools import setup, find_packages
from setuptools.extension import Extension
from setuptools.command.build_ext import build_ext

libraries = ['gsl', 'gslcblas']

if sysconfig.get_config_var("LIBM") == "-lm":
    libraries.append("m")

include_dirs = ['/usr/local/include', '/usr/include', 'c/']

library_dirs = ['/usr/local/lib', 'usr/lib']

pysbrl_module = Extension('_pysbrl',
                          sources=['c/pysbrl.c', 'c/train.c', 'c/rulelib.c', 'c/save_load.c',
                                   'c/bit_vector.c', 'c/utils.c', 'swig/pysbrl_wrap.c'],
                          include_dirs=include_dirs,
                          libraries=libraries,
                          library_dirs=library_dirs,
                          swig_opts=['-keyword'],
                          extra_link_args=["-Bstatic"],
                          extra_compile_args=['-std=c99']
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
      version='0.4',
      author="Yao, Ming",
      description="""A python interface of Scalable Bayesian Rule List""",
      ext_modules=[pysbrl_module],
      url='https://github.com/myaooo/pysbrl',
      py_modules=["pysbrl"],
      requires=['numpy'],
      cmdclass={'build_ext': CustomBuildExtCommand},
      #    extra_link_args = ["-bundle"],
      )
