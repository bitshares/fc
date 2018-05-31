#!/bin/sh

if [ "$#" != 1 ]; then
    echo "Usage: $0 <boost-unit-test-executable>" 1>&2
    exit 1
fi

"$1" --list_content 2>&1 \
  | grep '\*$' \
  | sed 's=\*$==;s=^    =/=' \
  | while read t; do
	case "$t" in
	/*) echo "$pre$t"; ;;
	*) pre="$t"; ;;
	esac
    done \
  | parallel echo Running {}\; "$1" -t {}

