add_executable(ComponentInfoTest
    ComponentInfoTest.cpp
    ${CMAKE_SOURCE_DIR}/Src/ComponentInfo.cpp
)

# Set C++ standard and compiler options similar to main plugin
target_compile_features(ComponentInfoTest PRIVATE cxx_std_20)
target_compile_options(ComponentInfoTest PRIVATE -Wall -Werror -fvisibility=hidden
    -Wno-multichar
    -Wno-ctor-dtor-privacy
    -Wno-invalid-offsetof
    -Wno-ignored-qualifiers
    -Wno-reorder
    -Wno-overloaded-virtual
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-unknown-pragmas
    -Wno-missing-braces
    -Wno-unused-private-field
    -Wno-return-std-move
    -Wno-unused-value
    -Wno-switch
    -Wno-deprecated
    -Wno-shorten-64-to-32
)

# Find Cocoa framework (macOS)
find_library(CocoaFramework Cocoa REQUIRED)

target_link_libraries(ComponentInfoTest
    PRIVATE
        gtest_main
        "${AC_API_DEVKIT_DIR}/Support/Frameworks/GSRoot.framework"
        "${AC_API_DEVKIT_DIR}/Support/Lib/libACAP_STAT.a"
        ${CocoaFramework}
        # Link with testable parts of ComponentInfo if separated
)

# Link with all GS libraries (same as main plugin)
link_gs_libraries(ComponentInfoTest)

gtest_discover_tests(ComponentInfoTest)