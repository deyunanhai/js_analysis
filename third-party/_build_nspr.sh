# !/bin/bash
export DIR=`pwd`
tar xzvf nspr-4.9.tar.gz && cd nspr-4.9/mozilla/nsprpub/ &&
./configure --prefix=${DIR}/dist --enable-64bit && make && make install
