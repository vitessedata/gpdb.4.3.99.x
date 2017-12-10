Greenplum Orca Build instruction.

Downloaded gpos, gporca, and xerces-c and added to git.  Xerces-c patch applied.  
To build, first make sure you have installed cmake 3+.  

Then run 
    bash ./build.sh 

The script will ask for sudo passwd, because it will make install into /usr/local.
It must be /usr/local, because gpdb build assume this.  I would believe we can some
how fix the gpdb Makefile, but it is just PITA.

And, it won't even start.  One must add /usr/local/lib to LD_LIBRARY_PATH.  Must be
done on all segs so the right place is to edit greenplum_path.sh.   Or, we just copy 
the lib dir into the greenplum build/install.  Either way, not pretty.

