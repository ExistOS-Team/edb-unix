#!/bin/sh -e
source $stdenv/setup
mkdir -p $out/bin

cd $src
mkdir -p $TMP/edb/build
cmake -B $TMP/edb/build/
cmake --build $TMP/edb/build/
cp $TMP/edb/build/edb $out/bin/edb
