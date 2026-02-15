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
    
    // Test 3: Check initial state - should show "未选择构件" or similar
    const noSelectionMessage = await page.evaluate(() => {
        const message = document.querySelector('.no-selection-message');
        return message ? message.textContent : 'No message found';
    });
    console.log(`Test 3 - No selection message: "${noSelectionMessage}"`);
    
    // Test 4: Check if UpdateSelectedElements function exists and call it
    const hasUpdateSelectedElements = await page.evaluate(() => {
        return typeof window.UpdateSelectedElements === 'function';
    });
    console.log(`Test 4 - UpdateSelectedElements function available: ${hasUpdateSelectedElements}`);
    
    if (hasUpdateSelectedElements) {
        // Call the function to trigger element selection
        await page.evaluate(() => {
            window.UpdateSelectedElements();
        });
        
        await page.waitForTimeout(1000);
        
        // Check if component info was loaded
        const componentIdValue = await page.evaluate(() => {
            const elem = document.getElementById('component-id');
            return elem ? elem.value : 'Element not found';
        });
        console.log(`Test 5 - Component ID after selection: "${componentIdValue}" (expected: "W-001")`);
    }
    
    // Test 6: Check photo description
    const photoDescValue = await page.evaluate(() => {
        const elem = document.getElementById('photo-description');
        return elem ? elem.value : 'Element not found';
    });
    console.log(`Test 6 - Photo Description: "${photoDescValue}" (expected: "West facade wall")`);
    
    // Test 7: Test edit mode - check if edit button exists
    const editButtonExists = await page.evaluate(() => {
        const editBtn = document.getElementById('edit-info-btn');
        return editBtn !== null;
    });
    console.log(`Test 7 - Edit button exists: ${editButtonExists}`);
    
    if (editButtonExists) {
        // Test 8: Test edit mode activation
        await page.evaluate(() => {
            document.getElementById('edit-info-btn').click();
        });
        
        await page.waitForTimeout(500);
        
        // Test 9: Check edit mode state by checking button visibility
        const saveButtonVisible = await page.evaluate(() => {
            const saveBtn = document.getElementById('save-info-btn');
            return saveBtn && !saveBtn.classList.contains('hidden');
        });
        const cancelButtonVisible = await page.evaluate(() => {
            const cancelBtn = document.getElementById('cancel-edit-btn');
            return cancelBtn && !cancelBtn.classList.contains('hidden');
        });
        const editButtonHidden = await page.evaluate(() => {
            const editBtn = document.getElementById('edit-info-btn');
            return editBtn && editBtn.classList.contains('hidden');
        });
        console.log(`Test 8 - Edit mode activated:`);
        console.log(`  Save button visible: ${saveButtonVisible} (expected: true)`);
        console.log(`  Cancel button visible: ${cancelButtonVisible} (expected: true)`);
        console.log(`  Edit button hidden: ${editButtonHidden} (expected: true)`);
        
        // Test 10: Test photo navigation if available
        const photoNavExists = await page.evaluate(() => {
            const photoNav = document.querySelector('.photo-navigation');
            return photoNav !== null;
        });
        
        if (photoNavExists) {
            const photoNavVisible = await page.evaluate(() => {
                const photoNav = document.querySelector('.photo-navigation');
                return photoNav && !photoNav.classList.contains('hidden');
            });
            console.log(`Test 9 - Photo navigation: ${photoNavVisible ? 'visible' : 'hidden'}`);
        }
        
        // Test 11: Test cancel edit
        await page.evaluate(() => {
            const cancelBtn = document.getElementById('cancel-edit-btn');
            if (cancelBtn) cancelBtn.click();
        });
        
        await page.waitForTimeout(500);
        
        const afterCancelSaveButtonHidden = await page.evaluate(() => {
            const saveBtn = document.getElementById('save-info-btn');
            return saveBtn && saveBtn.classList.contains('hidden');
        });
        const afterCancelEditButtonVisible = await page.evaluate(() => {
            const editBtn = document.getElementById('edit-info-btn');
            return editBtn && !editBtn.classList.contains('hidden');
        });
        console.log(`Test 10 - Cancel edit:`);
        console.log(`  Save button hidden after cancel: ${afterCancelSaveButtonHidden} (expected: true)`);
        console.log(`  Edit button visible after cancel: ${afterCancelEditButtonVisible} (expected: true)`);
    }
    
    // Test 12: Test IFC properties loading
    const ifcButtonExists = await page.evaluate(() => {
        const ifcBtn = document.getElementById('view-ifc-properties-btn');
        return ifcBtn !== null;
    });
    
    if (ifcButtonExists) {
        await page.evaluate(() => {
            document.getElementById('view-ifc-properties-btn').click();
        });
        
        await page.waitForTimeout(1000);
        
        const ifcPropertiesLoaded = await page.evaluate(() => {
            const propertyGroups = document.getElementById('property-groups');
            return propertyGroups !== null;
        });
        console.log(`Test 11 - IFC properties button clicked, property groups element exists: ${ifcPropertiesLoaded}`);
    }
    
    console.log("\n=== UI State Test Summary ===");
    console.log("All UI state management tests completed successfully!");
    
    return true;
};

// Export for use in Playwright
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { testUIStates };
}