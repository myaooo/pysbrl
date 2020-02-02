#!/bin/bash
set -e -x

echo "\n===================\nBuilding wheels for $PLAT\n===================\n"

# Install a system package required by our library
yum install -y gsl-devel

# Collect the pythons
pys=(/opt/python/*/bin)

# Print list of Python's available
echo "All Pythons: ${pys[@]}"

# Filter out Python 3.4
pys=(${pys[@]//*34*/})

# Compile wheels
# for PYBIN in /opt/python/*/bin; do
for PYBIN in "${pys[@]}"; do
    # if [[ "$PYBIN" =~ (cp27|cp34|cp35|cp36|cp37) ]]; then
        ${PYBIN}/pip install -r /io/dev-requirements.txt
        ${PYBIN}/pip install -r /io/test-requirements.txt
        ${PYBIN}/pip wheel /io/ -w wheelhouse/
    # fi
done

# Bundle external shared libraries into the wheels
for whl in wheelhouse/$PACKAGE_NAME-*.whl; do
    auditwheel repair $whl --plat $PLAT -w /io/wheelhouse/
done

# Install packages and test
# for PYBIN in /opt/python/*/bin/; do
for PYBIN in "${pys[@]}"; do
    # if [[ "$PYBIN" =~ (cp27|cp34|cp35|cp36|cp37) ]]; then
        ${PYBIN}/pip install pysbrl --no-index -f /io/wheelhouse
        (cd /io; ${PYBIN}/pytest)
    # fi
done