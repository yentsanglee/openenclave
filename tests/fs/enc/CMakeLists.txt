# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


oeedl_file(../fs.edl enclave gen)

#add_enclave(TARGET fs_enc SOURCES enc.c ${gen})

add_executable(fs_enc enc.cpp libcex.cpp ${gen})

set_source_files_properties(libcex.cpp
    PROPERTIES COMPILE_FLAGS
    "-I${PROJECT_SOURCE_DIR}/include/openenclave/libcex")

target_include_directories(fs_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(fs_enc oelibcxx oesgxfsenclave oeenclave)
