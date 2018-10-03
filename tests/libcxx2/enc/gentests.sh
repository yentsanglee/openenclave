#!/bin/bash

TESTS=$(cat ../tests.supported)

##
## Generate the tests:
##
n=1
for test in ${TESTS}
do
    file=test${n}.cpp
cat > ${file} <<END
#include "../${test}"
END

    echo "Created ${file}"
    n=$((n+1))
done

##
## Generate tests.h:
##
rm -f tests.h
n=1
for test in ${TESTS}
do

cat >> tests.h <<END
extern "C" int test${n}(void);
END

    n=$((n+1))
done

echo "Created tests.h"

##
## Generate tests.cpp:
##
rm -f tests.cpp
n=1
for test in ${TESTS}
do

cat >> tests.cpp <<END
printf("=== running test${n}()\n");
test${n}();
END

    n=$((n+1))
done

echo "Created tests.cpp"
