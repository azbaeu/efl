#!/bin/sh

set -e
. .ci/travis.sh
if [ "$1" = "release-ready" ] ; then
  exit 0
fi
travis_fold check-build "make check-build"
if [ "$DISTRO" != "" ] ; then
  docker exec --env MAKEFLAGS="-j5 -rR" --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) make check-build
else
  export PATH="/usr/local/opt/ccache/libexec:$(brew --prefix gettext)/bin:$PATH"
  pwd
  make check-build || grep -B5 check-build Makefile
fi
travis_endfold check-build
