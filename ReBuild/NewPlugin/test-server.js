const http = require('http');
const fs = require('fs');
const path = require('path');

const server = http.createServer((req, res) => {
    console.log(`请求: ${req.url}`);
    
    if (req.url === '/') {
        // 提供测试HTML文件
        const htmlPath = path.join(__dirname, 'test-e2e.html');
        fs.readFile(htmlPath, 'utf8', (err, data) => {
            if (err) {
                res.writeHead(500, { 'Content-Type': 'text/plain' });
                res.end('Error loading HTML file');
                return;
            }
            
            res.writeHead(200, { 'Content-Type': 'text/html' });
            res.end(data);
        });
    } else if (req.url === '/actual') {
        // 提供实际HTML文件（带模拟ACAPI）
        const htmlPath = path.join(__dirname, 'RFIX', 'index.html');
        fs.readFile(htmlPath, 'utf8', (err, data) => {
            if (err) {
                res.writeHead(500, { 'Content-Type': 'text/plain' });
                res.end('Error loading actual HTML file');
                return;
            }
            
            // 注入模拟ACAPI对象到实际HTML中
            const mockACAPI = `
                <script>
                    // 模拟ACAPI对象
                    window.ACAPI = {
                        GetSelectedElements: function() {
                            console.log("ACAPI.GetSelectedElements called");
                            return Promise.resolve([
                                ["{12345678-1234-1234-1234-123456789012}", "Wall", "ID-001"],
                                ["{87654321-4321-4321-4321-210987654321}", "Door", "ID-002"]
                            ]);
                        },
                        GetPluginVersion: function() {
                            console.log("ACAPI.GetPluginVersion called");
                            return Promise.resolve("0.2.12.47");
                        },
                        GetAllIFCProperties: function(elementGuid) {
                            console.log("ACAPI.GetAllIFCProperties called with:", elementGuid);
                            return Promise.resolve([
                                ["IfcWall", "Name", "Wall-01", "IfcLabel", "", true],
                                ["IfcWall", "Description", "Exterior wall", "IfcText", "", true]
                            ]);
                        },
                        GetCachedIFCProperties: function(elementGuid) {
                            console.log("ACAPI.GetCachedIFCProperties called with:", elementGuid);
                            return Promise.resolve([
                                ["IfcWall", "Name", "Wall-01", "IfcLabel", "", true],
                                ["IfcWall", "Description", "Exterior wall", "IfcText", "", true]
                            ]);
                        },
                        ComponentInfo_GetComponentInfo: function(elementGuid) {
                            console.log("ACAPI.ComponentInfo_GetComponentInfo called with:", elementGuid);
                            // Return as array format expected by JavaScript code
                            return Promise.resolve([
                                "W-001",  // componentId
                                "West facade wall",  // photoDesc
                                ["_构件照片_12345678/wall_01.jpg", "_构件照片_12345678/wall_02.jpg"],  // photoPaths
                                "12345678-1234-1234-1234-123456789012",  // globalId
                                "12345678-1234-1234-1234-123456789012"   // folderName
                            ]);
                        },
                        ComponentInfo_SaveComponentInfo: function(params) {
                            console.log("ACAPI.ComponentInfo_SaveComponentInfo called with:", params);
                            return Promise.resolve(true);
                        },
                        TestBridge: function(message) {
                            console.log("ACAPI.TestBridge called with:", message);
                            return Promise.resolve(true);
                        },
                        LogDebug: function(message) {
                            console.log("ACAPI.LogDebug called with:", message);
                            return Promise.resolve(true);
                        },
                        ComponentInfo_SelectPhotos: function(elementGuid) {
                            console.log("ACAPI.ComponentInfo_SelectPhotos called with:", elementGuid);
                            return Promise.resolve(["/path/to/photo1.jpg", "/path/to/photo2.jpg"]);
                        },
                        ComponentInfo_GetPhotoFullPath: function(relPath) {
                            console.log("ACAPI.ComponentInfo_GetPhotoFullPath called with:", relPath);
                            return Promise.resolve("/full/path/to/project/_构件照片_12345678/photo.jpg");
                        },
                        ComponentInfo_ViewPhotos: function(params) {
                            console.log("ACAPI.ComponentInfo_ViewPhotos called with:", params);
                            return Promise.resolve(true);
                        },
                        ComponentInfo_GetPhotoBase64: function(relPath) {
                            console.log("ACAPI.ComponentInfo_GetPhotoBase64 called with:", relPath);
                            // Return a small base64 encoded image (1x1 pixel transparent PNG)
                            return Promise.resolve("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==");
                        },
                        ComponentInfo_SetEditMode: function(editMode) {
                            console.log("ACAPI.ComponentInfo_SetEditMode called with:", editMode);
                            return Promise.resolve(true);
                        },
                        ComponentInfo_SetUnsavedChanges: function(hasChanges) {
                            console.log("ACAPI.ComponentInfo_SetUnsavedChanges called with:", hasChanges);
                            return Promise.resolve(true);
                        }
                    };
                    console.log("模拟ACAPI对象已注入到实际HTML界面");
                </script>
            `;
            
            // 在</body>标签前注入模拟ACAPI
            const modifiedData = data.replace('</body>', mockACAPI + '</body>');
            
            res.writeHead(200, { 'Content-Type': 'text/html' });
            res.end(modifiedData);
        });
    } else if (req.url === '/status') {
        // 状态检查端点
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ status: 'ok', time: new Date().toISOString() }));
    } else {
        res.writeHead(404, { 'Content-Type': 'text/plain' });
        res.end('Not Found');
    }
});

const PORT = 8080;
server.listen(PORT, () => {
    console.log(`测试服务器运行在 http://localhost:${PORT}`);
    console.log(`测试页面: http://localhost:${PORT}/`);
    console.log('按 Ctrl+C 停止服务器');
});

// 优雅关闭
process.on('SIGINT', () => {
    console.log('\n正在关闭服务器...');
    server.close(() => {
        console.log('服务器已关闭');
        process.exit(0);
    });
});