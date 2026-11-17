# SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
# SPDX-License-Identifier: BSD-2-Clause

if(NOT TARGET doctest::doctest)
    CPMAddPackage("gh:doctest/doctest@2.4.11")
endif()

# Integration tests mint a P-256 certificate in a temp directory. They do not embed key material.
find_package(OpenSSL REQUIRED)

# CPM puts the CMake helper next to the fetched sources. A parent build that
# already provided doctest exposes the same tree through the include directory.
if(NOT DEFINED doctest_SOURCE_DIR)
    get_target_property(_doctest_include doctest::doctest INTERFACE_INCLUDE_DIRECTORIES)
    list(GET _doctest_include 0 _doctest_include)
    get_filename_component(doctest_SOURCE_DIR "${_doctest_include}" DIRECTORY)
endif()
include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")

add_executable(moq_fabric_test
    main.cc
    fabric_node.cc
    fabric_connect.cc
    fabric_subscribe.cc
    fabric_announce.cc
    fabric_sns.cc
    fabric_data_header.cc
    fabric_info_base.cc
    fabric_integration.cc
    fabric_name.cc
)
target_include_directories(moq_fabric_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(moq_fabric_test PRIVATE moq-fabric doctest::doctest OpenSSL::Crypto)
set_target_properties(moq_fabric_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS OFF
)

doctest_discover_tests(moq_fabric_test)
