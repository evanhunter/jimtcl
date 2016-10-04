#!/bin/bash

set -ex

# Go to source root directory
SCRIPT=$(readlink -f "$0")
SRC_DIR=$(dirname "$SCRIPT")/../..
BUILD_DIR=`pwd`

type valgrind >/dev/null 2>&1 || { echo >&2 "valgrind, lcov and genhtml are required - please ensure they are installed.  Aborting."; exit 1; }
type lcov     >/dev/null 2>&1 || { echo >&2 "valgrind, lcov and genhtml are required - please ensure they are installed.  Aborting."; exit 1; }
type genhtml  >/dev/null 2>&1 || { echo >&2 "valgrind, lcov and genhtml are required - please ensure they are installed.  Aborting."; exit 1; }

# check tree is clean
if [[ $(git -C $SRC_DIR status --ignored -s) ]]; then
	echo "Source tree is not clean (including ignored files) - aborting"
    exit -1
fi

mkdir -p install

echo "Configure with tclsh"
# ensure tclsh is in the path
which tclsh

$SRC_DIR/configure CFLAGS="--coverage -DCI_TEST" \
	               LDFLAGS="--coverage" \
	               --full \
                   --maintainer \
                   --random-hash \
                   --prefix=$BUILD_DIR/install

echo "Configure by building jimsh0"
# tools/ci-test has bogus versions of jimsh & tclsh
PATH=$SRC_DIR/tools/ci-test:$PATH \
$SRC_DIR/configure CFLAGS="--coverage -DCI_TEST" \
	               LDFLAGS="--coverage" \
                   --full \
                   --maintainer \
                   --random-hash \
                   --prefix=$BUILD_DIR/install

make install ship

# Should be replaced with "make Tcl.html" when that command properly fails on parsing error
./jimsh $SRC_DIR/make-index $SRC_DIR/jim_tcl.txt | asciidoc -o $SRC_DIR/Tcl_shipped.html -d manpage -

echo "Normal Test"
$SRC_DIR/tools/ci-test/coverage.sh --no-configure

echo "Valgrind Test"
make -C $SRC_DIR/tests jimsh="valgrind --leak-check=full --show-reachable=yes --error-exitcode=1 $BUILD_DIR/jimsh" TOPSRCDIR=$SRC_DIR

# Parse test coverage results without exclusions
lcov -c --no-markers -d . -o lcov_output_without_exclusions.txt

echo "Check test coverage is not reduced"
./jimsh $SRC_DIR/tools/ci-test/lcov_parse.tcl

# cleanup
rm -rf install *.gcno *.gcda lcov.txt coverage_html lcov_output_with_exclusions.txt lcov_output_without_exclusions.txt
git -C $SRC_DIR checkout Tcl_shipped.html

# test clean targets
make clean
make distclean

# check tree is clean
if [[ $(git -C $SRC_DIR status --ignored -s) ]]; then
	echo "distclean did not remove everything"
    exit -1
fi
