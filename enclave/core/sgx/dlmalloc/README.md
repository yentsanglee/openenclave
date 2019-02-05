enclave/core/dlmalloc
=====================

This directory contains dummy standard C headers needed to compile dlmalloc.c
(included by enclave/core/malloc.c). These are generally redirected to the
enclave internal libc headers with the OE_NEED_STDC_NAMES compile flag enabled.
