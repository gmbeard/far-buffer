cmake_minimum_required(VERSION 3.8)

project(FarBuffer CXX)

add_library(FarBuffer INTERFACE)

target_include_directories(
    FarBuffer
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(
    FarBuffer
    INTERFACE
        cxx_decltype_auto
)

install(
    TARGETS
        FarBuffer
    EXPORT
        FarBufferTargets
)

install(
    FILES
        far_buffer.hpp
    DESTINATION
        include/far_buffer
)

install(
    FILES
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FarBufferConfig.cmake
    DESTINATION
        lib/cmake/FarBuffer
)

install(
    EXPORT
        FarBufferTargets
    DESTINATION
        lib/cmake/FarBuffer
    FILE
        FarBufferTargets.cmake
    NAMESPACE
        FarBuffer::
)
