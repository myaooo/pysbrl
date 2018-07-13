#!/usr/bin/env bash
swig -Wall -python `pwd`/../swig/sbrl.i
mv `pwd`/../swig/sbrl.py `pwd`/../pysbrl/sbrl.py