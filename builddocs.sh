#!/bin/bash

./bin/minilang src/extract_docs.mini docs.xml src/*.c
./bin/minilang src/document.mini docs/library src/*.c
doxygen
./bin/minilang docgroups.mini
cd docs && sphinx-build . ../html/ && cd ..
echo `date` > html/reload
