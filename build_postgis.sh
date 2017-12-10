set -e

TOPDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
INSTALLDIR=$(< INSTALLDIR)

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
( cd gpdb/contrib/postgis &&  \
	./configure --with-projdir=$TOPDIR/toolchain/installed --with-geosconfig=$TOPDIR/toolchain/installed/bin/geos-config --prefix=$INSTALLDIR ) 2>&1 > postgis.out || fail configure

echo Making
( cd gpdb/contrib/postgis &&  \
	make clean && \
	(make -j8 || make) && \
	make install ) 2>&1 >> postgis.out || fail make


( (cd toolchain/installed/lib && tar cf - libgeos*.so* libgda*.so* libproj*.so*) | \
       (cd $INSTALLDIR/lib && tar xf - ) ) 2>&1 >> postgis.out || fail tar

echo "SUCCESS"

