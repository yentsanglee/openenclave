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

if(VENCLAVE)
    target_link_libraries(oehosttest INTERFACE oevhostapp)
else()
    target_link_libraries(oehosttest INTERFACE oehostapp)
endif()

install(TARGETS oehosttest EXPORT openenclave-targets)

##==============================================================================
##
## oeenclavetest:
##
##==============================================================================

add_library(oeenclavetest INTERFACE)

if(VENCLAVE)
    target_link_libraries(oeenclavetest INTERFACE oevenclaveapp)
else()
    target_link_libraries(oeenclavetest INTERFACE oeenclaveapp)
endif()

install(TARGETS oeenclavetest EXPORT openenclave-targets)
