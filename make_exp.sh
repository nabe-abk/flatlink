#!/bin/sh
echo "#############################"
echo "# Need Open Watcom Compiler #"
echo "#############################"

cd `dirname $0`
cd src
wmake -f watcom.mak $@

