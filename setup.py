#!/usr/bin/env python

"""
setup.py file
"""

import os

from distutils import sysconfig
from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext

# includes = []
# includes.append(os.path.join(sysconfig.get_python_inc(plat_specific=0), 'numpy'))

libraries=['gsl', 'gslcblas', 'gmp']

if sysconfig.get_config_var("LIBM") == "-lm":
    libraries.append("m")

include_dirs = ['/usr/local/include']

library_dirs = ['/usr/local/lib']

pysbrl_module = Extension('_pysbrl',
                           sources=['pysbrl.i', 'pysbrl.c', 'train.c', 'rulelib.c'],
                           include_dirs= include_dirs,
                           libraries=libraries,
                           library_dirs=library_dirs,
                           swig_opts=['-keyword'],
                           extra_link_args=["-static"],
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


setup (name = 'pysbrl',
       version = '0.6',
       author      = "Yao, Ming",
       description = """A python interface of Scalable Bayesian Rule List""",
       ext_modules = [pysbrl_module],
       py_modules = ["pysbrl"],
        install_requires = ['numpy'],
        cmdclass = {'build_ext': CustomBuildExtCommand},
    #    extra_link_args = ["-bundle"],
       )