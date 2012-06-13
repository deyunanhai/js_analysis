# !/bin/bash
cd $(dirname $0)
./clean.sh
./_build_nspr.sh && ./_build_js.sh

