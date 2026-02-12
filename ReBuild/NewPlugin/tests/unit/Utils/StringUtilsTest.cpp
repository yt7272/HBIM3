#include <gtest/gtest.h>
#include "API_Guid.hpp"
#include "GSGuid.hpp"
#include "UniString.hpp"

// Test case: GUID to string conversion (basic test)
TEST(StringUtilsTest, GuidToStringConversion) {
    // Arrange
    API_Guid testGuid;
    // Initialize with null GUID
    memset(&testGuid, 0, sizeof(API_Guid));
    
    // Act
    GS::UniString guidString = APIGuid2GSGuid(testGuid).ToUniString();
    
    // Assert
    EXPECT_FALSE(guidString.IsEmpty());
    // Standard GUID string length is 36 characters
    EXPECT_EQ(guidString.GetLength(), 36);
}

// Test case: String comparison with Chinese characters
TEST(StringUtilsTest, ChineseStringComparison) {
    // Arrange
    GS::UniString chineseStr1 = "构件编号";
    GS::UniString chineseStr2 = "构件编号";
    
    // Act & Assert
    EXPECT_EQ(chineseStr1, chineseStr2);
}

// Test case: String concatenation
TEST(StringUtilsTest, StringConcatenation) {
    // Arrange
    GS::UniString str1 = "构件";
    GS::UniString str2 = "编号";
    
    // Act
    GS::UniString result = str1 + str2;
    
    // Assert
    EXPECT_EQ(result, "构件编号");
}