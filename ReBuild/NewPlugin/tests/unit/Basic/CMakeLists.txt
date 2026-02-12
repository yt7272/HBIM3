add_executable(BasicTest
    BasicTest.cpp
)

# Set C++ standard and compiler options similar to main plugin
target_compile_features(BasicTest PRIVATE cxx_std_20)
target_compile_options(BasicTest PRIVATE -Wall -Werror -fvisibility=hidden
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

target_link_libraries(BasicTest
    PRIVATE
        gtest_main
)

gtest_discover_tests(BasicTest)