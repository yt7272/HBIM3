#include <gtest/gtest.h>
#include <string>
#include <vector>

// Simple pure C++ tests to verify Google Test works
namespace Basic {
namespace Test {

TEST(BasicTest, SimpleAssertion) {
    EXPECT_EQ(1 + 1, 2);
}

TEST(BasicTest, StringComparison) {
    std::string str = "test";
    EXPECT_EQ(str, "test");
}

TEST(BasicTest, VectorOperations) {
    std::vector<int> vec = {1, 2, 3};
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], 1);
}

// Test that demonstrates Google Test features
class BasicTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        value = 42;
    }
    
    void TearDown() override {
        // cleanup
    }
    
    int value;
};

TEST_F(BasicTestFixture, FixtureWorks) {
    EXPECT_EQ(value, 42);
    value = 100;
    EXPECT_EQ(value, 100);
}

} // namespace Test
} // namespace Basic