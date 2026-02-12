#pragma once

#include <gmock/gmock.h>
#include "APIEnvir.h"
#include "ACAPinc.h"

// Mock for ACAPI functions used in ComponentInfo and PropertyUtils
class MockACAPI {
public:
    // Element operations
    MOCK_METHOD(GSErrCode, ACAPI_Element_Get, (const API_Guid&, API_Element*), (const));
    MOCK_METHOD(GSErrCode, ACAPI_Element_Change, (const API_Element&, const API_ElementMemo*), (const));
    MOCK_METHOD(GSErrCode, ACAPI_Element_GetMemo, (const API_Guid&, API_ElementMemo*), (const));
    
    // Property operations
    MOCK_METHOD(GSErrCode, ACAPI_Attribute_GetPropertyGroup, (API_PropertyGroup&), (const));
    MOCK_METHOD(GSErrCode, ACAPI_Attribute_CreatePropertyGroup, (const API_PropertyGroup&, API_Guid*), (const));
    MOCK_METHOD(GSErrCode, ACAPI_Attribute_GetPropertyDefinition, (const API_Guid&, API_PropertyDefinition&), (const));
    MOCK_METHOD(GSErrCode, ACAPI_Attribute_CreatePropertyDefinition, (const API_PropertyDefinition&, API_Guid*), (const));
    MOCK_METHOD(GSErrCode, ACAPI_Element_GetPropertyValues, (const API_Guid&, API_PropertyValues&), (const));
    MOCK_METHOD(GSErrCode, ACAPI_Element_SetPropertyValues, (const API_Guid&, const API_PropertyValues&), (const));
    
    // Notification operations
    MOCK_METHOD(GSErrCode, ACAPI_Notification_CatchSelectionChange, (void (*)(void)), (const));
    
    // Memory operations
    MOCK_METHOD(void, ACAPI_DisposeElemMemoHdls, (API_ElementMemo*), (const));
    
    // Write report
    MOCK_METHOD(void, ACAPI_WriteReport, (const char*, bool, const char*), (const));
};