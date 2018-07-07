#!/usr/bin/env bash
swig -Wall -c++ -cppext cpp -python swig/pysbrl.i
mv swig/pysbrl.py pysbrl/pysbrl.py