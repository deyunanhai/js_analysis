# !/bin/bash
tar xzvf js-1.7.0.tar.gz && 
cat macos.patch | patch -p0 &&
cd js/src/ &&
export CFLAGS="-DJS_C_STRINGS_ARE_UTF8"
make -f Makefile.ref BUILD_OPT=1 && 
make -f Makefile.ref BUILD_OPT=1 export &&
cp *.h ../../dist/include/
mv ../../dist/lib64/* ../../dist/lib/
