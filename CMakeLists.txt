cmake_minimum_required(VERSION 3.5)

# NOTE: you currently need to add dependencies listed in the README.md to your target yourself.

project("apclientpp" VERSION 0.6.3
        DESCRIPTION "A header-only c++ library to connect to Archipelago."
        HOMEPAGE_URL "https://github.com/black-sliver/apclientpp")

include(GNUInstallDirs)

add_library(apclientpp INTERFACE
        apclient.hpp
        apuuid.hpp
        defaultdatapackagestore.hpp)

target_include_directories(
        apclientpp
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
