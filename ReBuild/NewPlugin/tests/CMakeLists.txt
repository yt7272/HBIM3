cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# CTest configuration
include(GoogleTest)

# Add gtest include directories
include_directories(${googletest_SOURCE_DIR}/googletest/include)

# Set C++ standard to match main plugin (C++20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/Src
    ${CMAKE_SOURCE_DIR}/tests/mocks
    ${AC_API_DEVKIT_DIR}/Support/Inc
)

# Add definitions for ArchiCAD API
add_definitions(-Dmacintosh=1 -DGS_MAC -DACExtension)

# Add GS module include directories (same as main target)
get_filename_component (APIDevKitModulesDir "${AC_API_DEVKIT_DIR}/Support/Modules" ABSOLUTE)
file (GLOB GSIncludeFolders
    ${APIDevKitModulesDir}/*
)
foreach (includeFolder ${GSIncludeFolders})
    get_filename_component(folderName ${includeFolder} NAME)
    if (NOT folderName STREQUAL ".DS_Store")
        include_directories(${includeFolder})
    endif ()
endforeach ()

# GS library linking setup (same as main plugin)
if (WIN32)
    set (APIDevKitLinkLibDir "${APIDevKitModulesDir}")
else ()
    get_filename_component (APIDevKitLinkLibDir "${AC_API_DEVKIT_DIR}/Support/Frameworks" ABSOLUTE)
endif ()

file (GLOB GSLinkLibFolders
    ${APIDevKitLinkLibDir}/*
)

# Function to link a target with all GS libraries
function (link_gs_libraries target)
    foreach (linkLibFolder ${GSLinkLibFolders})
        get_filename_component(libName ${linkLibFolder} NAME)
        if (NOT libName STREQUAL ".DS_Store")
            target_link_libraries(${target} ${linkLibFolder})
        endif ()
    endforeach ()
endfunction ()

# Test targets
add_subdirectory(unit/ComponentInfo)
add_subdirectory(unit/PropertyUtils)
add_subdirectory(unit/Utils)
add_subdirectory(unit/Basic)