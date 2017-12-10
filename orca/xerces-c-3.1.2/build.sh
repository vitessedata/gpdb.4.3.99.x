rm -fr ./build; mkdir build
cd build
../configure
make VERBOSE=1 -j8
sudo make install 
