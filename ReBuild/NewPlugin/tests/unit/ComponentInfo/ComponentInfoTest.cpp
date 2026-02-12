#include <gtest/gtest.h>

// Include the actual ComponentInfo header
#include "ComponentInfo.hpp"

// Include mock headers if needed
// #include "mocks/API/MockACAPI.hpp"

namespace ComponentInfo {
namespace Test {

// Test fixture for ComponentInfo tests
class ComponentInfoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code for each test
    }
    
    void TearDown() override {
        // Cleanup code for each test
    }
};

// Test case: GuidToFolderName returns non-empty string for valid GUID
TEST_F(ComponentInfoTest, GuidToFolderNameReturnsNonEmpty) {
    // Arrange
    API_Guid testGuid;
    // Initialize with a known value (null GUID)
    testGuid = APINULLGuid;
    
    // Act
    GS::UniString result = GuidToFolderName(testGuid);
    
    // Assert
    EXPECT_FALSE(result.IsEmpty());
    // GUID string should be 36 characters (standard format)
    EXPECT_EQ(result.GetLength(), 36);
}

// Test case: GlobalIdToFolderName delegates to GuidToFolderName
TEST_F(ComponentInfoTest, GlobalIdToFolderNameDelegates) {
    // Arrange
    API_Guid testGuid;
    testGuid = APINULLGuid;
    
    // Act
    GS::UniString guidResult = GuidToFolderName(testGuid);
    GS::UniString globalIdResult = GlobalIdToFolderName(testGuid);
    
    // Assert
    EXPECT_EQ(globalIdResult, guidResult);
}

// Test case: Property group name constant (if exposed)
// This test would require the constant to be accessible
// For now, we can skip or test through other means

} // namespace Test
} // namespace ComponentInfo