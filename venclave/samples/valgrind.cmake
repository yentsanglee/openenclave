# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(VALGRIND_OPTS
    --tool=memcheck
    --error-exitcode=99
    --leak-check=full
    --show-leak-kinds=definite,possible
    --errors-for-leak-kinds=definite,possible
    --trace-children=yes
    "--trace-children-skip=*/oevproxyhost"
    --soname-synonyms=somalloc=NONE
    --track-fds=yes
    --show-reachable=yes
    --show-possibly-lost=yes
    --track-origins=yes
    --malloc-fill=0xAA
    --free-fill=0xDD)
