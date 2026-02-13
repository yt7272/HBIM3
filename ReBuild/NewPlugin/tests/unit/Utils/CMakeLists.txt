add_executable(UtilsTest
    StringUtilsTest.cpp
)

# Set C++ standard and compiler options similar to main plugin
target_compile_features(UtilsTest PRIVATE cxx_std_20)
target_compile_options(UtilsTest PRIVATE -Wall -Werror -fvisibility=hidden
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

target_link_libraries(UtilsTest
    PRIVATE
        gtest_main
        "${AC_API_DEVKIT_DIR}/Support/Lib/libACAP_STAT.a"
        ${CocoaFramework}
        # Link with testable parts of Utils if separated
)

# Link with GS libraries (ArchiCAD API)
link_gs_libraries(UtilsTest)

# Allow undefined symbols (plugin entry points not needed for tests)
target_link_options(UtilsTest PRIVATE "LINKER:-undefined,dynamic_lookup")

gtest_discover_tests(UtilsTest)