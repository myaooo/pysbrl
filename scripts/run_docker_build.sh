#!/usr/bin/env bash

DOCKER_IMAGE="quay.io/pypa/manylinux1_x86_64"

DOCKER_IMAGE2="quay.io/pypa/manylinux1_i686"

docker run --rm -v `pwd`:/code $DOCKER_IMAGE /code/scripts/docker_build.sh

docker run --rm -v `pwd`:/code $DOCKER_IMAGE2 linux32 /code/scripts/docker_build.sh