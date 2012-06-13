# !/bin/bash
tar xzvf js-1.7.0.tar.gz && 
cd js/src/ &&
export CFLAGS="-DJS_C_STRINGS_ARE_UTF8"
make -f Makefile.ref BUILD_OPT=1 && 
make -f Makefile.ref BUILD_OPT=1 export &&
mv ../../dist/lib64/* ../../dist/lib/ &&
cp *.h ../../dist/include/
