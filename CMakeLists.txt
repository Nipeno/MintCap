# MintCap - build definition for the ReShade add-on (Windows x64, MSVC).
# Copyright (c) 2026 Nipeno. All rights reserved.
# Source available, not open source. See LICENSE.

cmake_minimum_required(VERSION 3.20)

# Addon version. Single source of truth: the git tag vX.Y.Z, which CI passes as
# -DMINTCAP_VERSION=X.Y.Z. Untagged/local builds fall back to 0.0.0. This flows into the
# DLL's VERSIONINFO resource (so Windows file properties show a version) and into the
# overlay's "Version X.Y.Z" label via MINTCAP_VERSION_STR.
set(MINTCAP_VERSION "0.0.0" CACHE STRING "MintCap version (X.Y.Z)")

# Link the CRT statically. This DLL is injected into someone else's game process, so a
# dependency on the VC++ redist would make it fail to load on PCs that lack it - which
# looks exactly like "the addon is broken" to a player. Must be set before project().
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Windows x64 only. Checked against the HOST before project(), because project() enables
# the RC language and would fail first with a generic "no CMAKE_RC_COMPILER" error
# elsewhere, burying this explanation.
if(NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "MintCap is a Windows x64 ReShade addon and only builds on Windows.")
endif()

project(MintCap VERSION ${MINTCAP_VERSION} LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Comma-separated form (a.b.c.0) for the binary FILEVERSION/PRODUCTVERSION fields of the
# VERSIONINFO resource, generated from version.rc.in.
set(MINTCAP_VERSION_COMMA "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)

add_library(MintCap SHARED src/main.cpp "${CMAKE_CURRENT_BINARY_DIR}/version.rc")

# Expose the version string to the C++ source (overlay label).
target_compile_definitions(MintCap PRIVATE MINTCAP_VERSION_STR="${PROJECT_VERSION}")

# ReShade addon SDK header + the matching Dear ImGui declarations (v1.90.4).
target_include_directories(MintCap PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/reshade"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui")

# winmm        -> timeBeginPeriod/timeEndPeriod
# shell32      -> ShellExecuteA (source-code link)
target_link_libraries(MintCap PRIVATE winmm shell32)

# Output the addon with ReShade's expected extension: MintCap.addon64
set_target_properties(MintCap PROPERTIES
    OUTPUT_NAME "MintCap"
    SUFFIX ".addon64"
    PREFIX "")
