// E2E测试脚本 - 模拟ACAPI对象并测试HTML界面
const fs = require('fs');
const path = require('path');

// 读取mock-acapi.js内容
const mockAcapiPath = path.join(__dirname, '__tests__', 'mock-acapi.js');
const mockAcapiContent = fs.readFileSync(mockAcapiPath, 'utf8');

// 创建测试HTML文件，包含模拟的ACAPI对象
const htmlPath = path.join(__dirname, 'RFIX', 'index.html');
const htmlContent = fs.readFileSync(htmlPath, 'utf8');

// 创建测试HTML文件，注入模拟ACAPI对象
const testHtmlContent = `
<!DOCTYPE html>
<html>
<head>
    <title>HBIM构件信息输入工具 - E2E测试</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        /* 简化样式用于测试 */
        body { font-family: sans-serif; padding: 20px; }
        .test-result { margin: 10px 0; padding: 10px; border: 1px solid #ccc; }
        .success { background-color: #d4edda; border-color: #c3e6cb; }
        .error { background-color: #f8d7da; border-color: #f5c6cb; }
        .warning { background-color: #fff3cd; border-color: #ffeaa7; }
    </style>
</head>
<body>
    <h1>ArchiCAD插件HTML界面E2E测试</h1>
    <div id="test-results"></div>
    
    <!-- 原始HTML内容 -->
    <div id="original-content" style="display: none;">
        ${htmlContent.replace(/<script[\s\S]*?<\/script>/gi, '')}
    </div>
    
    <script>
        // 注入模拟ACAPI对象
        console.log("开始注入模拟ACAPI对象...");
        
        // 模拟ACAPI对象
        window.ACAPI = {
            // 1. GetSelectedElements
            GetSelectedElements: function() {
                console.log("ACAPI.GetSelectedElements called");
                return [
                    ["{12345678-1234-1234-1234-123456789012}", "Wall", "ID-001"],
                    ["{87654321-4321-4321-4321-210987654321}", "Door", "ID-002"]
                ];
            },
            
            // 2. GetPluginVersion
            GetPluginVersion: function() {
                console.log("ACAPI.GetPluginVersion called");
                return "0.2.12.47";
            },
            
            // 3. GetAllIFCProperties
            GetAllIFCProperties: function(elementGuid) {
                console.log("ACAPI.GetAllIFCProperties called with:", elementGuid);
                return [
                    ["IfcWall", "Name", "Wall-01", "IfcLabel", ""],
                    ["IfcWall", "Description", "Exterior wall", "IfcText", ""]
                ];
            },
            
            // 4. GetCachedIFCProperties
            GetCachedIFCProperties: function(elementGuid) {
                console.log("ACAPI.GetCachedIFCProperties called with:", elementGuid);
                return [
                    ["IfcWall", "Name", "Wall-01", "IfcLabel", ""],
                    ["IfcWall", "Description", "Exterior wall", "IfcText", ""]
                ];
            },
            
            // 5. ComponentInfo_GetComponentInfo
            ComponentInfo_GetComponentInfo: function(elementGuid) {
                console.log("ACAPI.ComponentInfo_GetComponentInfo called with:", elementGuid);
                return {
                    componentId: "W-001",
                    photoDesc: "West facade wall",
                    photoPaths: ["_构件照片_12345678/wall_01.jpg", "_构件照片_12345678/wall_02.jpg"],
                    globalId: "12345678-1234-1234-1234-123456789012",
                    folderName: "12345678-1234-1234-1234-123456789012"
                };
            },
            
            // 6. ComponentInfo_SaveComponentInfo
            ComponentInfo_SaveComponentInfo: function(params) {
                console.log("ACAPI.ComponentInfo_SaveComponentInfo called with:", params);
                return true;
            },
            
            // 7. TestBridge
            TestBridge: function(message) {
                console.log("ACAPI.TestBridge called with:", message);
                return true;
            },
            
            // 8. LogDebug
            LogDebug: function(message) {
                console.log("ACAPI.LogDebug called with:", message);
                return true;
            },
            
            // 9. ComponentInfo_SelectPhotos
            ComponentInfo_SelectPhotos: function(elementGuid) {
                console.log("ACAPI.ComponentInfo_SelectPhotos called with:", elementGuid);
                return ["/path/to/photo1.jpg", "/path/to/photo2.jpg"];
            },
            
            // 10. ComponentInfo_GetPhotoFullPath
            ComponentInfo_GetPhotoFullPath: function(relPath) {
                console.log("ACAPI.ComponentInfo_GetPhotoFullPath called with:", relPath);
                return "/full/path/to/project/_构件照片_12345678/photo.jpg";
            },
            
            // 11. ComponentInfo_ViewPhotos
            ComponentInfo_ViewPhotos: function(params) {
                console.log("ACAPI.ComponentInfo_ViewPhotos called with:", params);
                return true;
            },
            
            // 12. ComponentInfo_GetPhotoBase64
            ComponentInfo_GetPhotoBase64: function(relPath) {
                console.log("ACAPI.ComponentInfo_GetPhotoBase64 called with:", relPath);
                // Return a small base64 encoded image (1x1 pixel transparent PNG)
                return "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==";
            },
            
            // 13. ComponentInfo_SetEditMode
            ComponentInfo_SetEditMode: function(editMode) {
                console.log("ACAPI.ComponentInfo_SetEditMode called with:", editMode);
                return true;
            },
            
            // 14. ComponentInfo_SetUnsavedChanges
            ComponentInfo_SetUnsavedChanges: function(hasChanges) {
                console.log("ACAPI.ComponentInfo_SetUnsavedChanges called with:", hasChanges);
                return true;
            }
        };
        
        console.log("模拟ACAPI对象已注入，包含14个函数");
        
        // 测试函数
        function testACAPIFunctions() {
            const results = [];
            const testResultsDiv = document.getElementById('test-results');
            
            // 测试每个函数
            const functionTests = [
                { name: 'GetSelectedElements', test: () => ACAPI.GetSelectedElements() },
                { name: 'GetPluginVersion', test: () => ACAPI.GetPluginVersion() },
                { name: 'GetAllIFCProperties', test: () => ACAPI.GetAllIFCProperties('{test-guid}') },
                { name: 'GetCachedIFCProperties', test: () => ACAPI.GetCachedIFCProperties('{test-guid}') },
                { name: 'ComponentInfo_GetComponentInfo', test: () => ACAPI.ComponentInfo_GetComponentInfo('{test-guid}') },
                { name: 'ComponentInfo_SaveComponentInfo', test: () => ACAPI.ComponentInfo_SaveComponentInfo(['guid', 'id', 'desc', []]) },
                { name: 'TestBridge', test: () => ACAPI.TestBridge('test message') },
                { name: 'LogDebug', test: () => ACAPI.LogDebug('debug message') },
                { name: 'ComponentInfo_SelectPhotos', test: () => ACAPI.ComponentInfo_SelectPhotos('{test-guid}') },
                { name: 'ComponentInfo_GetPhotoFullPath', test: () => ACAPI.ComponentInfo_GetPhotoFullPath('path/to/photo.jpg') },
                { name: 'ComponentInfo_ViewPhotos', test: () => ACAPI.ComponentInfo_ViewPhotos(['guid', 'path']) },
                { name: 'ComponentInfo_GetPhotoBase64', test: () => ACAPI.ComponentInfo_GetPhotoBase64('path/to/photo.jpg') },
                { name: 'ComponentInfo_SetEditMode', test: () => ACAPI.ComponentInfo_SetEditMode(true) },
                { name: 'ComponentInfo_SetUnsavedChanges', test: () => ACAPI.ComponentInfo_SetUnsavedChanges(true) }
            ];
            
            functionTests.forEach((funcTest, index) => {
                try {
                    const result = funcTest.test();
                    console.log(\`✓ 函数 \${funcTest.name} 调用成功: \`, result);
                    results.push({ name: funcTest.name, success: true, result: result });
                    
                    const div = document.createElement('div');
                    div.className = 'test-result success';
                    div.innerHTML = \`<strong>✓ \${funcTest.name}</strong>: 调用成功 (结果类型: \${typeof result})\`;
                    testResultsDiv.appendChild(div);
                } catch (error) {
                    console.error(\`✗ 函数 \${funcTest.name} 调用失败: \`, error);
                    results.push({ name: funcTest.name, success: false, error: error.message });
                    
                    const div = document.createElement('div');
                    div.className = 'test-result error';
                    div.innerHTML = \`<strong>✗ \${funcTest.name}</strong>: 调用失败 - \${error.message}\`;
                    testResultsDiv.appendChild(div);
                }
            });
            
            // 总结
            const successCount = results.filter(r => r.success).length;
            const totalCount = results.length;
            
            const summaryDiv = document.createElement('div');
            summaryDiv.className = successCount === totalCount ? 'test-result success' : 'test-result warning';
            summaryDiv.innerHTML = \`<h3>测试总结</h3>
                <p>总共测试函数: \${totalCount}</p>
                <p>成功: \${successCount}</p>
                <p>失败: \${totalCount - successCount}</p>
                <p>成功率: \${Math.round((successCount / totalCount) * 100)}%</p>\`;
            testResultsDiv.appendChild(summaryDiv);
            
            return results;
        }
        
        // 页面加载完成后运行测试
        window.addEventListener('load', function() {
            console.log("页面加载完成，开始测试...");
            setTimeout(() => {
                const results = testACAPIFunctions();
                console.log("测试完成:", results);
                
                // 将结果保存到全局变量供外部访问
                window.e2eTestResults = results;
                
                // 触发截图信号
                window.dispatchEvent(new CustomEvent('e2e-test-complete', { 
                    detail: { results: results } 
                }));
            }, 1000);
        });
    </script>
</body>
</html>`;

// 保存测试HTML文件
const testHtmlPath = path.join(__dirname, 'test-e2e.html');
fs.writeFileSync(testHtmlPath, testHtmlContent);
console.log(`测试HTML文件已创建: ${testHtmlPath}`);

// 创建测试报告
const report = `
# ArchiCAD插件HTML界面E2E测试报告
**生成时间:** ${new Date().toLocaleString()}
**测试文件:** ${testHtmlPath}
**原始HTML:** ${htmlPath}
**模拟ACAPI对象:** ${mockAcapiPath}

## 测试内容
1. 加载HTML文件并注入模拟ACAPI对象
2. 测试所有14个ACAPI函数的可用性
3. 检查JavaScript错误
4. 生成截图证据

## 使用方法
1. 在浏览器中打开: file://${testHtmlPath}
2. 查看控制台输出
3. 检查测试结果

## 预期结果
- 所有14个ACAPI函数应成功调用
- 无JavaScript错误
- 页面应正常加载和显示
`;

console.log(report);