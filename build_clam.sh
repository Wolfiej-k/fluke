#!/bin/bash

sudo apt-get install libboost-all-dev libboost-program-options-dev
sudo apt-get install libgmp-dev
sudo apt-get install libmpfr-dev	
sudo apt-get install libflint-dev

python3 -m fluke .venv
source .venv/bin/activate
pip3 install --user lit
pip3 install --user OutputCheck

cd clam && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$DIR ../
cmake --build . --target crab && cmake ..   
cmake --build . --target extra && cmake ..                  
cmake --build . --target install 
