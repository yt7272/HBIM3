// HBIM Plugin Debug Collector
// 将此代码复制到 ArchiCAD 浏览器控制台 (F12) 运行
// 自动收集调试信息并生成诊断报告

(async function() {
    console.log('🔍 HBIM Plugin Debug Collector v1.0');
    console.log('====================================');
    
    // 1. 基本信息
    console.log('\n📋 1. 基本信息');
    console.log('- 时间:', new Date().toLocaleString());
    console.log('- 用户代理:', navigator.userAgent);
    console.log('- 页面URL:', window.location.href);
    
    // 2. ACAPI 对象检查
    console.log('\n📋 2. ACAPI 对象检查');
    const hasACAPI = typeof ACAPI !== 'undefined';
    console.log('- ACAPI 可用:', hasACAPI);
    
    if (hasACAPI) {
        const acapiFunctions = Object.keys(ACAPI).sort();
        console.log('- ACAPI 函数数量:', acapiFunctions.length);
        console.log('- ACAPI 函数列表:', acapiFunctions.join(', '));
        
        // 测试关键函数
        try {
            const version = await ACAPI.GetPluginVersion();
            console.log('- 插件版本:', version);
        } catch (err) {
            console.log('- GetPluginVersion 错误:', err.message);
        }
        
        try {
            const elements = await ACAPI.GetSelectedElements();
            console.log('- 当前选中构件:', elements ? elements.length : 0, '个');
            if (elements && elements.length > 0) {
                console.log('  - 第一个构件 GUID:', elements[0]);
            }
        } catch (err) {
            console.log('- GetSelectedElements 错误:', err.message);
        }
    }
    
    // 3. 全局变量检查
    console.log('\n📋 3. 全局变量检查');
    const globals = [
        'currentElementGuid',
        'currentGlobalId',
        'currentPhotoPaths',
        'currentFolderName',
        'hasUnsavedChanges',
        'isEditMode',
        'DEBUG'
    ];
    
    globals.forEach(varName => {
        const value = window[varName];
        console.log(`- ${varName}:`, value);
    });
    
    // 4. 函数检查
    console.log('\n📋 4. 函数检查');
    const functions = [
        'hasComponentInfo',
        'enterEditMode',
        'exitEditMode',
        'updateUIState',
        'UpdateSelectedElements'
    ];
    
    functions.forEach(funcName => {
        const exists = typeof window[funcName] === 'function';
        console.log(`- ${funcName}:`, exists ? '✅ 存在' : '❌ 不存在');
    });
    
    // 5. UI状态检查
    console.log('\n📋 5. UI状态检查');
    if (typeof hasComponentInfo === 'function') {
        console.log('- hasComponentInfo():', hasComponentInfo());
    }
    
    // 检查 body 类
    const bodyClasses = document.body.className.split(' ').filter(c => c.startsWith('state-'));
    console.log('- body 状态类:', bodyClasses.length > 0 ? bodyClasses.join(', ') : '无状态类');
    
    // 检查 UI 状态指示器
    const stateIndicator = document.getElementById('ui-state-text');
    console.log('- UI状态指示器:', stateIndicator ? stateIndicator.textContent : '未找到元素');
    
    // 6. 元素可见性检查
    console.log('\n📋 6. 元素可见性检查');
    const elementsToCheck = [
        { id: 'no-selection-message', name: '未选择构件消息' },
        { id: 'no-info-message', name: '无信息消息' },
        { id: 'has-info-content', name: '有信息内容' },
        { id: 'add-info-btn', name: '添加信息按钮' },
        { id: 'edit-info-btn', name: '编辑信息按钮' },
        { id: 'save-info-btn', name: '保存按钮' },
        { id: 'cancel-info-btn', name: '取消按钮' }
    ];
    
    elementsToCheck.forEach(item => {
        const el = document.getElementById(item.id);
        const exists = !!el;
        const visible = exists ? 
            (el.offsetParent !== null || el.style.display !== 'none') : false;
        const display = exists ? window.getComputedStyle(el).display : 'N/A';
        
        console.log(`- ${item.name} (${item.id}):`, 
            exists ? (visible ? '✅ 可见' : '⚠️ 存在但不可见') : '❌ 不存在',
            `[display: ${display}]`
        );
    });
    
    // 7. 错误检查
    console.log('\n📋 7. 错误检查');
    console.log('- 控制台错误数量: 请手动检查红色错误信息');
    
    // 8. 生成诊断总结
    console.log('\n📋 8. 诊断总结');
    const issues = [];
    
    if (!hasACAPI) issues.push('ACAPI 对象不可用');
    if (typeof hasComponentInfo !== 'function') issues.push('hasComponentInfo 函数不存在');
    if (bodyClasses.length === 0) issues.push('body 无状态类');
    
    if (issues.length === 0) {
        console.log('✅ 未发现明显问题');
        console.log('建议: 检查构件选择后UI状态变化，查看 [HBIM Debug] 日志');
    } else {
        console.log('⚠️ 发现问题:', issues.length, '个');
        issues.forEach((issue, i) => console.log(`  ${i+1}. ${issue}`));
        console.log('建议: 重新加载插件或重启 ArchiCAD');
    }
    
    console.log('\n====================================');
    console.log('📤 请复制此输出并提交给开发人员');
    console.log('====================================\n');
})();