#!/bin/bash

set -ex

# Go to source root directory
SCRIPT=$(readlink -f "$0")
SRC_DIR=$(dirname "$SCRIPT")/../..
BUILD_DIR=`pwd`

type lcov    >/dev/null 2>&1 || { echo >&2 "lcov and genhtml are required - please ensure they are installed.  Aborting."; exit 1; }
type genhtml >/dev/null 2>&1 || { echo >&2 "lcov and genhtml are required - please ensure they are installed.  Aborting."; exit 1; }

while [[ $# -gt 0 ]]
do
	key="$1"
	case $key in
		--no-configure)
			NO_CONFIGURE=true
		;;
		--open)
	    	OPEN_PAGE=true
	    ;;
		*)
			echo "Unknown option $1"
			echo "options: --no-configure : do not reconfigure"
			echo "         --open         : open results in web browser"
			exit -1
		;;
	esac
	shift
done

if [ "$NO_CONFIGURE" != "true" ]; then
	$SRC_DIR/configure \
	               CFLAGS="--coverage -DCI_TEST" \
	               LDFLAGS="--coverage" \
                   --full \
                   --maintainer \
                   --random-hash
fi

make test
lcov -c -d . -o lcov.txt
genhtml -o coverage_html lcov.txt

if [ "$OPEN_PAGE" == "true" ]; then
	xdg-open coverage_html/index.html
fi