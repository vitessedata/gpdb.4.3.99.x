set -e

TOPDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
INSTALLDIR=$(< INSTALLDIR)
INSTALLDIR="$INSTALLDIR/madlib"

function fail 
{
    echo
    echo "ERROR: " "$@"
    echo
    exit 1
}


if [ "$GPHOME" == "" ]; then 
    echo
    echo GPHOME env missing
    exit 1
fi

if [ "$INSTALLDIR" == "" ]; then
    echo
    echo 'missing INSTALLDIR ... check ./INSTALLDIR file'
    exit 1
fi

echo Installing into $INSTALLDIR


echo Configuring
( cd madlib && mkdir -p build &&  \
    ( cat boost_1_61_0.[ab] > boost_1_61_0.tar.gz ) ) 2>&1 > madlib.out \
	|| fail untar
( cd madlib && \
    ./configure \
	-DCMAKE_INSTALL_PREFIX=$INSTALLDIR \
	-DEIGEN_TAR_SOURCE=$TOPDIR/madlib/eigen.3.2.10.tar.gz \
	-DBOOST_TAR_SOURCE=$TOPDIR/madlib/boost_1_61_0.tar.gz \
	-DPYXB_TAR_SOURCE=$TOPDIR/madlib/PyXB-1.2.4.tar.gz \
) 2>&1 > madlib.out || fail configure

echo Making
( cd madlib/build && \
	make clean && \
	(make -j8 || make -j8 || make) && \
	make install ) 2>&1 >> madlib.out || fail make

( cd $INSTALLDIR && rm Current && ln -s Versions/1.9.1 Current &&
	rm bin && ln -s ./Current/bin &&
	rm doc && ln -s ./Current/doc ) 2>&1 >> madlib.out || fail linkup


echo "SUCCESS"

