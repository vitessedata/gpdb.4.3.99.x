rm -f gpfdist 

./configure CPPFLAGS=-I$TOOLCHAIN_DIR/installed/include \
	LDFLAGS=-L$TOOLCHAIN_DIR/installed/lib \
	--with-apr-config=$TOOLCHAIN_DIR/installed/bin/apr-1-config

make 

