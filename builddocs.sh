#!/bin/bash

./bin/minilang src/document.mini docs/library src/*.c
cd docs && sphinx-build . ../html/ && cd ..
echo `date` > html/reload
