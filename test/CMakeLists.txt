cmake_minimum_required(VERSION 3.16)

include(FetchContent)
FetchContent_Declare(
        asio
        URL https://github.com/chriskohlhoff/asio/archive/f693a3eb7fe72a5f19b975289afc4f437d373d9c.zip)
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_Declare(
        websocketpp
        URL https://github.com/zaphoyd/websocketpp/archive/4dfe1be74e684acca19ac1cf96cce0df9eac2a2d.zip)
# Desktop-only, without submodule:
#FetchContent_Declare(
#        wswrap
#        URL https://github.com/black-sliver/wswrap/archive/e71695c80e0338d0aa4f18e7e0f45b67681206be.zip)
# Complete, with submodule:
FetchContent_Declare(
        wswrap
        GIT_REPOSITORY https://github.com/black-sliver/wswrap.git
        GIT_TAG e71695c80e0338d0aa4f18e7e0f45b67681206be)

FetchContent_MakeAvailable(asio json websocketpp wswrap)

add_executable(TestBasic test_basic.cpp)
target_precompile_headers(TestBasic
        PRIVATE "<apclient.hpp>")
target_link_libraries(TestBasic
        PRIVATE apclientpp
        PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(TestBasic
        PRIVATE "${wswrap_SOURCE_DIR}/include"
        PRIVATE "${asio_SOURCE_DIR}/asio/include"
        PRIVATE ${websocketpp_SOURCE_DIR})
target_compile_definitions(TestBasic
        PRIVATE AP_NO_SCHEMA ASIO_STANDALONE _WEBSOCKETPP_CPP11_THREAD_)
if(WIN32 OR MSYS OR MINGW)
    target_link_libraries(TestBasic
            PRIVATE crypt32 # required for system cert loading with OpenSSL
            PRIVATE ws2_32)
endif()
if ((MSVC) AND (MSVC_VERSION GREATER_EQUAL 1914))
    target_compile_options(TestBasic
            PRIVATE "/Zc:__cplusplus" /bigobj)
endif()

# find OpenSSL and enable SSL support if found
find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_link_libraries(TestBasic
            PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    target_compile_definitions(TestBasic
            PRIVATE WSWRAP_NO_SSL)
endif()

# find zlib and enable compression support if found
find_package(ZLIB)
if(ZLIB_FOUND)
    target_link_libraries(TestBasic PRIVATE ZLIB::ZLIB)
else()
    target_compile_definitions(TestBasic
            PRIVATE WSWRAP_NO_COMPRESSION)
endif()

# require C++14 support
# target_compile_features(TestBasic PUBLIC cxx_std_14)
# force C++14 for test
set_target_properties(TestBasic PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

if(EMSCRIPTEN)
    message("Configuring TestBasic for emscripten")
    # embind requires C++17
    set_target_properties(TestBasic PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED YES
    )
    target_link_options(TestBasic BEFORE PRIVATE "--bind")
endif()

# enable more warnings
if(MSVC)
    target_compile_options(TestBasic PRIVATE /W4)
    # NOTE: we can not do /WX unless we build/install ZLIB
else()
    target_compile_options(TestBasic PRIVATE -Wall -Wextra -Werror)
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(TestBasic PRIVATE -Wno-template-id-cdtor)
    # NOTE: fixing -Wtemplate-id-cdtor would require using a websocketpp fork
endif()

# enable ASAN and UBSAN on compatible platforms
if(NOT MSYS AND NOT MINGW AND NOT MSVC)
    target_compile_options(TestBasic
            PRIVATE -fsanitize=address -fsanitize=undefined)
    target_link_options(TestBasic
            PRIVATE -fsanitize=address -fsanitize=undefined)
endif()

add_test(NAME TestBasic
        COMMAND TestBasic)
