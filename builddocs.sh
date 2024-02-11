#!/bin/bash

./bin/minilang src/document.mini docs/library src/*.c
doxygen
minilang docgroups.mini
cd docs && sphinx-build . ../html/ && cd ..
echo `date` > html/reload
