# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(fuzz_test tcbinfoparser.cpp)
add_executable(remote_test remote_verify.cpp)
if(USE_LIBSGX)
    target_compile_definitions(fuzz_test PRIVATE OE_USE_LIBSGX )
	target_compile_definitions(remote_test PRIVATE OE_USE_LIBSGX )
endif()

add_custom_command( TARGET fuzz_test
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/seeds/tcbinfoparser ${CMAKE_CURRENT_BINARY_DIR}/seeds/tcbinfoparser )
add_custom_command( TARGET remote_test
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/seeds/reportinfoparser ${CMAKE_CURRENT_BINARY_DIR}/seeds/reportinfoparser )

target_link_libraries(fuzz_test oehostapp )
target_link_libraries(remote_test oehostapp )

add_test(NAME tests/fuzz_test COMMAND ./fuzz_test ${CMAKE_CURRENT_BINARY_DIR}/seeds/tcbinfoparser/tcbInfo.json)
add_test(NAME tests/remote_test COMMAND ./remote_test ${CMAKE_CURRENT_BINARY_DIR}/seeds/reportinfoparser/generate_report.txt)
