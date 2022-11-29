#!/bin/sh

config=Release

for param in $*; do
  name=${param%%=*}
  value=${param#*=}
  case "$name" in
    --config) config=$value;;
  esac
done

for dir in build build/$config; do
  if [ ! -d $dir ]; then
    mkdir $dir
  fi
done

cd build/$config
cmake  -DCMAKE_BUILD_TYPE="$config" ../..
make -j4
