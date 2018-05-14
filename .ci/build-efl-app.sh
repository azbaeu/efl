#!/bin/sh

set -e

cd /

#clone our examples from efl
git clone http://git.enlightenment.org/tools/examples.git/

cd examples/apps/c/life/

#get meson
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python get-pip.py --user
pip install meson

#build the example
mkdir build
meson . ./build
ninja -C build all

#remove the folder again so its not left in the artifacts
cd /
rm -rf examples
