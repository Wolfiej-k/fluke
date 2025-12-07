#!/bin/bash

sudo apt-get install libboost-all-dev libboost-program-options-dev
sudo apt-get install libgmp-dev
sudo apt-get install libmpfr-dev	
sudo apt-get install libflint-dev

cd clam && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$DIR ../
cmake --build . --target crab && cmake ..   
cmake --build . --target extra && cmake ..                  
sudo cmake --build . --target install 
