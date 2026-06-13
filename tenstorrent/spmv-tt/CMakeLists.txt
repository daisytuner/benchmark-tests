cmake_minimum_required(VERSION 3.14)

project(spmv-tt C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)


include(../../cmake/CPM.cmake)

set(CMAKE_LINKER_TYPE LLD)

CPMAddPackage(
    NAME Boost
    VERSION 1.86.0
    URL
        https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
        URL_HASH
        SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
    OPTIONS
        "BOOST_ENABLE_CMAKE ON"
        "BOOST_SKIP_INSTALL_RULES ON"
        "BUILD_SHARED_LIBS OFF"
        "BOOST_INCLUDE_LIBRARIES core\\\;container"
    FIND_PACKAGE_ARGUMENTS "CONFIG REQUIRED"
)

add_executable(main
    util.cpp
    main.cpp
    kernel.cpp
)

target_compile_definitions(main PRIVATE
    SPDLOG_FMT_EXTERNAL
)

find_package(Boost REQUIRED COMPONENTS container)

target_include_directories(main PRIVATE
    extract/2516758880658744274
    ../../docc/rtl/include
    # reflect
    $ENV{TT_METAL_INSTALL_DIR}/include/metalium-thirdparty
	# tt_metal APIs
	$ENV{TT_METAL_INSTALL_DIR}/include
)

target_link_options(main PRIVATE

)

target_link_directories(main PRIVATE
    $ENV{TT_METAL_INSTALL_DIR}/lib
)

target_link_libraries(main PRIVATE
    # metallium dependencies that ship with it
    tt_metal
    fmt # internal dependency of tt_metal
    # dependencies of metallium that do not ship with TT-Metal libs dir (yet)
    Boost::container
    Boost::core
    # nlohmann_json::nlohmann_json
    # TracyClient
)
