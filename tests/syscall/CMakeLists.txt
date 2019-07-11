# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if(UNIX)
add_subdirectory(cpio)
add_subdirectory(datagram)
add_subdirectory(dup)
add_subdirectory(fs)
add_subdirectory(hostfs)
add_subdirectory(ids)
add_subdirectory(poller)
if(NOT VENCLAVE)
add_subdirectory(resolver) # ATTN:VENCLAVE!
endif()
add_subdirectory(socket)
add_subdirectory(socketpair)
add_subdirectory(sendmsg)
endif()
