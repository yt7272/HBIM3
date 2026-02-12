#include <gtest/gtest.h>

// Include the actual PropertyUtils header
#include "PropertyUtils.hpp"

namespace PropertyUtils {
namespace Test {

// Test fixture for PropertyUtils tests
class PropertyUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code for each test
    }
    
    void TearDown() override {
        // Cleanup code for each test
    }
};

// Test case: PropertyInfo structure default initialization
TEST_F(PropertyUtilsTest, PropertyInfoDefaultValues) {
    // Arrange
    PropertyInfo info;
    
    // Act & Assert
    EXPECT_TRUE(info.propertyName.IsEmpty());
    EXPECT_TRUE(info.propertyValue.IsEmpty());
    EXPECT_TRUE(info.propertyType.IsEmpty());
    EXPECT_TRUE(info.propertySetName.IsEmpty());
    EXPECT_TRUE(info.unit.IsEmpty());
    EXPECT_FALSE(info.isValid);
}

// Test case: Empty property list for null element
// Note: This test would require mocking ArchiCAD API calls
// For now, we'll skip actual function calls

} // namespace Test
} // namespace PropertyUtils