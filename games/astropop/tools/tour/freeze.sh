#!/bin/bash
# A copy of the port for a long tour (build/tourexe/astropop-<tag>.exe beside its DLLs), so that the build can go on while the
# tour runs; prints its path:  PORT_EXE=$(bash games/astropop/tools/tour/freeze.sh <tag>) games/astropop/tools/tour/gamerun.sh ...
cd "$(dirname "$0")/../../../.."
tag=${1:-x}
mkdir -p build/tourexe
cp build/native/astropop.exe "build/tourexe/astropop-$tag.exe"
cp -n build/native/*.dll build/tourexe/
echo "build/tourexe/astropop-$tag.exe"
