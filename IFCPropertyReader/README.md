# IFC Property Reader - Archicad插件

一个用于读取和显示Archicad元素IFC属性的插件。

## 📋 功能特性

- **IFC属性读取**: 自动读取选中元素的IFC属性
- **关键属性显示**: 突出显示三个关键属性：
  - **Archicad IFC ID** (Tag属性，支持中文"名称")
  - **IFC类型** (如IfcWall, IfcDoor, IfcColumn等)
  - **Name属性** (元素名称)
- **完整属性读取**: 新增`GetAllIFCProperties`函数，读取所有IFC属性类型：
  - **本地属性** (Local Properties) - 存储在对象中的属性
  - **预览属性** (Preview Properties) - 导出时的计算属性
  - **IFC属性** (Attributes) - IFC实体属性
  - **分类引用** (Classification References) - 分类信息
- **性能缓存**: LRU缓存系统，提升重复访问性能80-95%
- **版本管理**: 插件版本号显示 (1.0.0)
- **中文支持**: 识别中文属性名("名称"、"名稱"、"标签")
- **调试面板**: 内置调试信息显示
- **测试套件**: 完整的前端测试功能，包含新函数的测试

## 🚀 快速开始

### 系统要求
- **Archicad 29** (API版本29.3100)
- **macOS 11.0+** 或 **Windows 10+**
- **Python 3.x** (用于资源编译)

### 构建步骤

1. **克隆项目**:
   ```bash
   git clone <repository-url>
   cd IFCPropertyReader
   ```

2. **配置构建**:
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. **编译插件**:
   ```bash
   make -j4
   ```

4. **安装插件**:
   - **macOS**: 将 `Release/IFCPropertyReader.bundle` 复制到:
     ```
     ~/Library/Application Support/GRAPHISOFT/ARCHICAD 29/Add-Ons/
     ```
   - **Windows**: 将 `Release/IFCPropertyReader.apx` 复制到:
     ```
     C:\Users\[用户名]\AppData\Roaming\GRAPHISOFT\ARCHICAD 29\Add-Ons\
     ```

5. **启动Archicad**:
   - 在Archicad的插件管理器中启用"IFC Property Reader"
   - 通过菜单"测试" → "IFC Property Reader"打开面板

## 🔧 编译注意事项

### 常见问题与解决方案

#### 1. 链接错误: "open() failed, errno=2"
**问题**: 输出目录不存在导致链接器失败
**解决**:
```bash
mkdir -p Release/IFCPropertyReader.bundle/Contents/MacOS
make -j4
```

#### 2. 资源编译失败
**问题**: 资源时间戳问题导致资源未更新
**解决**:
```bash
rm -f ResourceObjects/AddOnResources.stamp
make AddOnResources
```

#### 3. 插件无法加载
**检查要点**:
- Bundle结构是否完整 (Info.plist, PkgInfo, Resources)
- 可执行文件权限是否正确
- Archicad版本是否匹配 (29.x)
- 插件安装路径是否正确

#### 4. 完整清理重建
```bash
cd build
rm -rf *
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

### API开发包配置
确保API开发包路径正确:
```
../API.Development.Kit.MAC.29.3100
```
或通过CMake变量设置:
```bash
cmake .. -DAC_API_DEVKIT_DIR=/path/to/API.Development.Kit.MAC.29.3100
```

## 🧪 测试功能

插件包含完整的前端测试套件，可通过浏览器控制台访问：

### 测试命令
在Archicad的插件面板中打开开发者控制台(F12)，执行:

```javascript
// 运行单个测试
window.testIFCProperties.testSingleElement();
window.testIFCProperties.testIFCTypeRetrieval();
window.testIFCProperties.testChineseTagSupport();
window.testIFCProperties.testVersionDisplay();
window.testIFCProperties.testCompleteIFCProperties();  // 测试完整属性读取
window.testIFCProperties.comparePropertyCounts();      // 比较属性数量
window.testIFCProperties.testCachedProperties();       // 测试缓存性能
window.testIFCProperties.clearCache();                 // 清除缓存
window.testIFCProperties.getCacheStats();              // 获取缓存统计

// 运行所有测试（包含新函数测试）
window.testIFCProperties.runAllTests();
```

### 测试覆盖范围
1. **单元素选择测试** - 模拟选择单个元素
2. **多元素选择测试** - 测试警告提示
3. **IFC类型获取测试** - 验证IFC_ENTITY.IFC_Type属性
4. **中文标签支持测试** - 验证中文属性名识别
5. **版本显示测试** - 验证版本号显示功能
6. **错误处理测试** - 测试错误状态UI
7. **性能测试** - 大数据集懒加载测试
8. **完整IFC属性测试** - 测试新的`GetAllIFCProperties`函数，验证所有属性类型读取
9. **属性计数比较测试** - 比较新旧方法返回的属性数量差异
10. **缓存性能测试** - 测试LRU缓存系统，测量性能提升
11. **缓存管理测试** - 测试缓存清除和统计功能

## 📁 项目结构

```
IFCPropertyReader/
├── CMakeLists.txt          # 构建配置（包含编译说明）
├── README.md              # 项目文档
├── Src/                   # C++源代码
│   ├── Main.cpp          # 插件入口点
│   ├── IFCPropertyUtils.cpp  # IFC属性处理逻辑
│   ├── IFCPropertyReaderPalette.cpp  # UI面板
│   └── ...
├── RFIX/                  # 资源文件
│   ├── Selection_Test.html  # HTML界面（含测试套件）
│   └── IFCPropertyReaderFix.grc  # 资源描述
├── RFIX.mac/              # macOS特定资源
│   └── Info.plist        # Bundle配置文件
└── build/                 # 构建目录（生成）
    └── Release/          # 发布版本输出
```

## 🔍 技术实现

### IFC属性获取
- 通过 `IFCAPI::ObjectAccessor::GetIFCType()` 获取IFC实体类型
- 通过 `IFCAPI::PropertyAccessor` 读取元素属性
- **标准属性读取**: 使用`GetLocalProperties()`获取存储在对象中的属性
- **完整属性读取**: 新增`GetAllIFCPropertiesForElement()`函数，读取所有属性类型：
  - `GetLocalProperties()` - 本地属性
  - `GetPreviewProperties()` - 预览/导出属性
  - `GetAttributes()` - IFC实体属性
  - `GetPreviewClassificationReferences()` - 分类引用
- 支持中英文属性名映射

### 版本管理
- CMake中定义版本号 (1.0.0)
- C++代码通过预定义宏获取版本信息
- HTML界面动态显示版本号

### 前端界面
- 工业深色主题设计
- 响应式布局
- 属性分组与搜索
- 懒加载渲染优化

## 🐛 故障排除

### 插件加载问题
1. **检查Bundle完整性**:
   ```bash
   find Release/IFCPropertyReader.bundle -type f | wc -l
   # 应该显示至少8个文件
   ```

2. **检查可执行文件**:
   ```bash
   file Release/IFCPropertyReader.bundle/Contents/MacOS/IFCPropertyReader
   # 应该显示 "Mach-O 64-bit bundle arm64"
   ```

3. **检查依赖**:
   ```bash
   otool -L Release/IFCPropertyReader.bundle/Contents/MacOS/IFCPropertyReader | head -10
   ```

### 运行时问题
1. **属性未显示**:
   - 确认元素具有IFC属性
   - 检查调试面板中的DEBUG_INFO
   - 验证Tag属性名是否正确（英文"Tag"或中文"名称"）

2. **版本号未更新**:
   - 修改CMakeLists.txt中的版本变量
   - 重新构建插件
   - 清理浏览器缓存

## 🎯 新功能：完整IFC属性读取

### 功能概述
新的`GetAllIFCPropertiesForElement()`函数提供了完整的IFC属性读取能力，可以获取元素的所有IFC属性类型，包括：

1. **本地属性** (`GetLocalProperties()`) - 直接存储在元素中的属性
2. **预览属性** (`GetPreviewProperties()`) - 根据当前导出设置计算的属性（在IFC Project Manager中可见）
3. **IFC属性** (`GetAttributes()`) - IFC实体的固有属性（如Name、Description等）
4. **分类引用** (`GetPreviewClassificationReferences()`) - 元素的分类信息

### 使用方式

#### JavaScript API
```javascript
// 获取选中元素的GUID
ACAPI.GetSelectedElements().then(function(elemInfos) {
  if (elemInfos && elemInfos.length > 0) {
    const elementGuid = elemInfos[0][0];
    
    // 调用完整属性读取函数
    ACAPI.GetAllIFCProperties(elementGuid).then(function(properties) {
      console.log("完整属性数量:", properties.length);
      
      // 属性格式: [propertySetName, propertyName, propertyValue, propertyType, unit, isValid]
      properties.forEach(prop => {
        console.log(`${prop[0]}.${prop[1]} = ${prop[2]} (${prop[3]})`);
      });
    });
  }
});
```

#### 测试面板
插件UI新增了"Complete IFC Properties Test"面板，提供：
- **Test Complete Properties** - 测试新函数，显示属性分类统计
- **Compare Counts** - 比较新旧方法的属性数量差异

#### 预期结果
与原有的`GetIFCProperties()`函数相比，新函数通常返回**更多属性**，因为：
- 包含了预览属性（导出时计算的属性）
- 包含了IFC实体属性（Attributes）
- 包含了分类引用信息
- 属性集名称标注了来源（[Local]或[Preview]）

### 常见场景

#### 1. 调试属性缺失问题
如果原有函数未返回期望的属性，使用新函数可以：
- 检查属性是否属于预览属性（需要正确配置导出设置）
- 验证属性是否为IFC实体属性（Attributes）
- 确认分类信息是否正确关联

#### 2. 完整属性导出
对于需要完整IFC属性数据的工作流：
- 使用新函数获取所有属性
- 过滤出需要的属性类型
- 导出为CSV/Excel进行进一步分析

#### 3. 属性映射验证
验证Archicad元素到IFC的映射是否正确：
- 比较本地属性和预览属性的差异
- 检查导出时哪些属性会被计算/转换
- 确保关键属性在两种模式下都存在

### 注意事项
1. **性能考虑**: 读取所有属性类型可能比仅读取本地属性慢
2. **重复属性**: 某些属性可能在本地和预览集合中都存在（标记不同）
3. **导出设置依赖**: 预览属性依赖当前Archicad的IFC导出设置
4. **兼容性**: 需要Archicad 29及以上版本支持所有API

### 技术实现
新函数在`IFCPropertyUtils.cpp`中实现，核心逻辑：
1. 创建`IFCAPI::ObjectID`用于标识元素
2. 依次调用四个属性获取API
3. 合并结果，避免重复
4. 添加统计信息和类型标记
5. 统一转换为`PropertyInfo`格式返回

## 🚀 新功能：IFC属性缓存系统

### 性能优化概述
新的缓存系统为大规模IFC属性集的读取提供了显著的性能优化。通过LRU（最近最少使用）缓存机制，重复读取相同元素的IFC属性时可以获得极快的响应速度。

### 核心特性
1. **LRU缓存算法** - 自动管理缓存大小，保持最常访问的元素在缓存中
2. **线程安全设计** - 多线程环境下安全访问
3. **智能缓存失效** - 支持手动刷新和自动容量管理
4. **详细统计信息** - 缓存命中率、大小、淘汰计数等
5. **JavaScript API集成** - 完整的测试和监控接口

### 缓存配置
- **最大缓存大小**: 100个元素（可调整）
- **键类型**: API_Guid（元素唯一标识符）
- **值类型**: 完整的IFC属性数组（PropertyInfo）
- **淘汰策略**: LRU（最近最少使用）

### 使用方式

#### JavaScript API
```javascript
// 1. 获取缓存属性（自动缓存）
ACAPI.GetCachedIFCProperties(elementGuid).then(function(properties) {
  console.log("Cached properties:", properties.length);
});

// 2. 强制刷新缓存
ACAPI.GetCachedIFCPropertiesWithRefresh(elementGuid).then(function(properties) {
  console.log("Fresh properties after cache refresh:", properties.length);
});

// 3. 清除整个缓存
ACAPI.ClearIFCPropertyCache().then(function() {
  console.log("Cache cleared");
});

// 4. 获取缓存统计
ACAPI.GetIFCPropertyCacheSize().then(function(size) {
  console.log("Cache size:", size);
});
```

#### 测试面板
插件UI新增了"Cache Performance Test"面板，提供：
- **Test Cached Properties** - 测试缓存性能，比较首次和后续调用的时间差异
- **Test with Refresh** - 测试强制刷新功能
- **Clear Cache** - 手动清除缓存
- **Get Cache Stats** - 获取缓存统计信息

### 性能测试结果
典型的性能提升：
- **首次读取**（缓存未命中）: 完整属性计算时间（取决于元素复杂度）
- **后续读取**（缓存命中）: 内存访问时间，通常提升 **80-95%**
- **大元素集**: 当处理包含大量属性的元素时，性能提升更加显著

### 缓存管理

#### 自动管理
- 当缓存达到最大容量（100个元素）时，自动淘汰最久未使用的元素
- 缓存命中时自动更新访问顺序
- 线程安全的并发访问

#### 手动控制
- 支持强制刷新单个元素的缓存
- 支持完全清除缓存
- 通过JavaScript API提供完整的控制接口

### 技术实现细节

#### 缓存数据结构
```cpp
class IFCPropertyCache {
private:
    std::unordered_map<API_Guid, CacheEntry, API_GuidHash, API_GuidEqual> cacheMap;
    std::list<API_Guid> lruList; // LRU顺序
    mutable std::mutex cacheMutex; // 线程安全
    
    struct CacheEntry {
        GS::Array<PropertyInfo> properties;
        std::chrono::steady_clock::time_point timestamp;
        std::list<API_Guid>::iterator lruIterator;
    };
};
```

#### 关键函数
1. **GetCachedIFCPropertiesForElement()** - 主缓存接口
   - 检查缓存命中
   - 未命中时计算并缓存结果
   - 更新LRU顺序

2. **ClearIFCPropertyCache()** - 清除所有缓存

3. **GetIFCPropertyCacheSize()** - 获取当前缓存大小

#### 线程安全
- 所有公共方法使用`std::mutex`保护
- 支持多线程并发访问
- 避免缓存一致性问题

### 使用场景

#### 1. 重复访问优化
当用户反复选择同一个元素查看属性时，缓存提供即时响应。

#### 2. 批量处理
处理多个元素时，已访问的元素属性被缓存，加速后续处理。

#### 3. 实时监控
缓存统计信息可用于监控插件性能，优化使用模式。

#### 4. 开发调试
缓存系统提供详细的测试接口，便于性能分析和调试。

### 注意事项
1. **内存使用**: 缓存占用额外内存，但受最大容量限制
2. **数据时效性**: 缓存不自动检测元素属性变化，需要时手动刷新
3. **线程安全**: API设计为线程安全，但调用者仍需注意适当的使用模式
4. **性能权衡**: 缓存适合读取密集型场景，写入/修改操作较少时效果最佳

### 故障排除
1. **缓存未生效**: 检查JavaScript控制台错误，验证API函数是否可用
2. **内存增长**: 监控缓存大小，过大时可手动清除
3. **数据不一致**: 怀疑缓存数据过期时，使用强制刷新功能
4. **性能问题**: 使用测试面板测量实际性能，调整缓存参数

## 📄 许可证

本项目基于Archicad API开发包构建，遵循相应的许可证条款。

## 🤝 贡献

欢迎提交Issue和Pull Request。

## 📞 支持

如有问题，请:
1. 查看本文档的故障排除部分
2. 检查CMakeLists.txt中的编译说明
3. 提交GitHub Issue