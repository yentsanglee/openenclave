# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(fingerprint)
add_subdirectory(proxy)
add_subdirectory(host)
add_subdirectory(core)
add_subdirectory(samples)

##==============================================================================
##
## oehosttest:
##
##==============================================================================

add_library(oehosttest INTERFACE)

target_link_libraries(oehosttest INTERFACE oehostapp)

install(TARGETS oehosttest EXPORT openenclave-targets)

##==============================================================================
##
## oeenclavetest:
##
##==============================================================================

add_library(oeenclavetest INTERFACE)

target_link_libraries(oeenclavetest INTERFACE oeenclaveapp)

install(TARGETS oeenclavetest EXPORT openenclave-targets)
