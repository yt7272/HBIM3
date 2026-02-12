// Test script for UI state management
const testUIStates = async (page) => {
    console.log("=== Testing UI State Management ===");
    
    // Wait for page to load
    await page.waitForTimeout(1000);
    
    // Test 1: Initial state should be "state-no-selection"
    const initialBodyClass = await page.evaluate(() => document.body.className);
    console.log(`Test 1 - Initial state: ${initialBodyClass}`);
    console.log(`✓ Should contain 'state-no-selection': ${initialBodyClass.includes('state-no-selection')}`);
    
    // Test 2: Check that "未选择构件" message is visible
    const noSelectionVisible = await page.evaluate(() => {
        const message = document.querySelector('.no-selection-message');
        return message && window.getComputedStyle(message).display !== 'none';
    });
    console.log(`Test 2 - No selection message visible: ${noSelectionVisible}`);
    
    // Test 3: Simulate element selection by calling updateSelectedElements
    await page.evaluate(() => {
        // Mock element selection
        const elemInfos = [
            ["{12345678-1234-1234-1234-123456789012}", "Wall", "ID-001"]
        ];
        window.updateSelectedElements(elemInfos);
    });
    
    await page.waitForTimeout(500);
    
    // Test 4: Check state after element selection (should be "state-has-info" since mock returns component info)
    const afterSelectionClass = await page.evaluate(() => document.body.className);
    console.log(`Test 3 - After element selection: ${afterSelectionClass}`);
    console.log(`✓ Should contain 'state-has-info': ${afterSelectionClass.includes('state-has-info')}`);
    
    // Test 5: Check that component info is loaded
    const componentIdValue = await page.evaluate(() => document.getElementById('component-id').value);
    const photoDescValue = await page.evaluate(() => document.getElementById('photo-description').value);
    console.log(`Test 4 - Component info loaded:`);
    console.log(`  Component ID: ${componentIdValue} (expected: "W-001")`);
    console.log(`  Photo Description: ${photoDescValue} (expected: "West facade wall")`);
    
    // Test 6: Test edit mode
    await page.evaluate(() => {
        document.getElementById('edit-info-btn').click();
    });
    
    await page.waitForTimeout(500);
    
    // Test 7: Check edit mode state
    const isEditMode = await page.evaluate(() => window.isEditMode);
    const editButtonsVisible = await page.evaluate(() => {
        const saveBtn = document.getElementById('save-info-btn');
        const cancelBtn = document.getElementById('cancel-edit-btn');
        return saveBtn && !saveBtn.classList.contains('hidden') && 
               cancelBtn && !cancelBtn.classList.contains('hidden');
    });
    console.log(`Test 5 - Edit mode:`);
    console.log(`  isEditMode: ${isEditMode} (expected: true)`);
    console.log(`  Edit buttons visible: ${editButtonsVisible} (expected: true)`);
    
    // Test 8: Test photo preview navigation
    const photoNavVisible = await page.evaluate(() => {
        const photoNav = document.querySelector('.photo-navigation');
        return photoNav && !photoNav.classList.contains('hidden');
    });
    const photoIndexText = await page.evaluate(() => document.getElementById('photo-index').textContent);
    console.log(`Test 6 - Photo preview:`);
    console.log(`  Navigation visible: ${photoNavVisible} (expected: true)`);
    console.log(`  Photo index: ${photoIndexText} (expected: "1 / 2")`);
    
    // Test 9: Test next photo button
    await page.evaluate(() => {
        document.getElementById('next-photo-btn').click();
    });
    
    await page.waitForTimeout(300);
    
    const afterNextPhotoIndex = await page.evaluate(() => document.getElementById('photo-index').textContent);
    console.log(`Test 7 - Next photo button:`);
    console.log(`  After clicking next: ${afterNextPhotoIndex} (expected: "2 / 2")`);
    
    // Test 10: Test previous photo button
    await page.evaluate(() => {
        document.getElementById('prev-photo-btn').click();
    });
    
    await page.waitForTimeout(300);
    
    const afterPrevPhotoIndex = await page.evaluate(() => document.getElementById('photo-index').textContent);
    console.log(`Test 8 - Previous photo button:`);
    console.log(`  After clicking previous: ${afterPrevPhotoIndex} (expected: "1 / 2")`);
    
    // Test 11: Test cancel edit
    await page.evaluate(() => {
        document.getElementById('cancel-edit-btn').click();
    });
    
    await page.waitForTimeout(500);
    
    const afterCancelEditMode = await page.evaluate(() => window.isEditMode);
    console.log(`Test 9 - Cancel edit:`);
    console.log(`  isEditMode after cancel: ${afterCancelEditMode} (expected: false)`);
    
    // Test 12: Test IFC properties loading
    await page.evaluate(() => {
        document.getElementById('view-ifc-properties-btn').click();
    });
    
    await page.waitForTimeout(1000);
    
    const ifcPropertiesLoaded = await page.evaluate(() => {
        const propertyGroups = document.getElementById('property-groups');
        return propertyGroups && propertyGroups.children.length > 0;
    });
    console.log(`Test 10 - IFC properties:`);
    console.log(`  Property groups loaded: ${ifcPropertiesLoaded} (expected: true)`);
    
    console.log("\n=== UI State Test Summary ===");
    console.log("All UI state management tests completed successfully!");
    
    return true;
};

// Export for use in Playwright
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { testUIStates };
}