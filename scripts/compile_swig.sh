#!/usr/bin/env bash
swig -Wall -python ./swig/sbrl.i
mv ./swig/sbrl.py ./pysbrl/sbrl.py