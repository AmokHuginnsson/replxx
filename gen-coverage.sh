#! /bin/sh

rm -rf build
mkdir build
cd build

env CXXFLAGS=--coverage cmake ..
env CXXFLAGS=--coverage make

lcov --base-directory .. --directory CMakeFiles --capture --initial --output-file replxx-baseline.info

cd -
./tests.py
cd -

lcov --base-directory .. --directory CMakeFiles --capture --output-file replxx-test.info
lcov --add-tracefile replxx-baseline.info --add-tracefile replxx-test.info --output-file replxx-total.info
lcov --extract replxx-total.info '*/replxx/src/*' '*/replxx/include/*' --output-file replxx-coverage.info
genhtml replxx-coverage.info --legend --num-spaces=2 --output-directory replxx-coverage-html

