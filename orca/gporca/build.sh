rm -fr ./build; mkdir build
cd build
cmake ..
make VERBOSE=1 -j8
sudo make install
