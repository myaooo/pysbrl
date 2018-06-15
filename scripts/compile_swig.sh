#!/usr/bin/env bash
swig -Wall -python swig/pysbrl.i
mv swig/pysbrl.py pysbrl/pysbrl.py