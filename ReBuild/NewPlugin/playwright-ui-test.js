#!/usr/bin/env node

/**
 * Playwright UI State Test Runner
 * 
 * This script runs the UI state tests using Playwright browser automation.
 * It loads the HTML interface with injected mock ACAPI and runs the testUIStates
 * function from test-ui-states.js.
 */

const { chromium } = require('playwright');
const path = require('path');
const fs = require('fs');

// Import the testUIStates function from test-ui-states.js
const { testUIStates } = require('./test-ui-states.js');

// Configuration
const TEST_URL = 'http://localhost:8080/actual';
const SCREENSHOT_DIR = path.join(__dirname, 'playwright-screenshots');
const REPORT_FILE = path.join(__dirname, 'playwright-test-report.txt');

// Ensure screenshot directory exists
if (!fs.existsSync(SCREENSHOT_DIR)) {
    fs.mkdirSync(SCREENSHOT_DIR, { recursive: true });
}

// Test results tracking
const testResults = {
    total: 0,
    passed: 0,
    failed: 0,
    errors: [],
    screenshots: []
};

async function takeScreenshot(page, name) {
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const screenshotPath = path.join(SCREENSHOT_DIR, `${name}-${timestamp}.png`);
    await page.screenshot({ path: screenshotPath, fullPage: true });
    testResults.screenshots.push({ name, path: screenshotPath });
    console.log(`  Screenshot saved: ${screenshotPath}`);
    return screenshotPath;
}

async function runUITests() {
    console.log('=== Playwright UI State Tests ===');
    console.log(`Test URL: ${TEST_URL}`);
    console.log(`Screenshot directory: ${SCREENSHOT_DIR}`);
    console.log('Starting browser...\n');

    let browser;
    let page;

    try {
        // Launch browser
        browser = await chromium.launch({ headless: false }); // Set to true for CI
        const context = await browser.newContext({
            viewport: { width: 1200, height: 800 }
        });
        page = await context.newPage();

        // Navigate to test page
        console.log(`Navigating to ${TEST_URL}...`);
        await page.goto(TEST_URL, { waitUntil: 'networkidle' });

        // Wait for page to load
        await page.waitForTimeout(2000);

        // Take initial screenshot
        console.log('\n1. Taking initial screenshot...');
        await takeScreenshot(page, 'initial-state');

        // Check if ACAPI is available
        const acapiAvailable = await page.evaluate(() => {
            return typeof window.ACAPI !== 'undefined';
        });

        if (!acapiAvailable) {
            throw new Error('ACAPI object not found on page. Mock injection may have failed.');
        }

        console.log(`✓ ACAPI object available: ${acapiAvailable}`);

        // Run the UI state tests
        console.log('\n2. Running UI state tests...');
        
        // Capture console logs from the page
        page.on('console', msg => {
            const msgText = msg.text();
            if (msgText.includes('Test') || msgText.includes('✓') || msgText.includes('✗')) {
                console.log(`  [Page Console] ${msgText}`);
            }
        });

        // Run the testUIStates function
        const testResult = await testUIStates(page);
        
        if (testResult === true) {
            console.log('\n✓ All UI state tests passed!');
            testResults.passed++;
        } else {
            console.log('\n✗ UI state tests returned false');
            testResults.failed++;
        }
        testResults.total++;

        // Take screenshots of key states
        console.log('\n3. Taking screenshots of key UI states...');
        
        // Screenshot after element selection - call UpdateSelectedElements to trigger loading
        await page.evaluate(() => {
            if (typeof window.UpdateSelectedElements === 'function') {
                window.UpdateSelectedElements();
            }
        });
        await page.waitForTimeout(1000);
        await takeScreenshot(page, 'after-element-selection');

        // Screenshot in edit mode
        await page.evaluate(() => {
            document.getElementById('edit-info-btn').click();
        });
        await page.waitForTimeout(500);
        await takeScreenshot(page, 'edit-mode-active');

        // Screenshot with photo navigation
        await page.waitForTimeout(300);
        await takeScreenshot(page, 'photo-navigation-visible');

        // Screenshot with IFC properties
        await page.evaluate(() => {
            document.getElementById('view-ifc-properties-btn').click();
        });
        await page.waitForTimeout(1000);
        await takeScreenshot(page, 'ifc-properties-loaded');

        // Return to initial state for final screenshot
        await page.evaluate(() => {
            // Cancel edit if active
            if (window.isEditMode) {
                document.getElementById('cancel-edit-btn').click();
            }
            // Clear selection
            document.body.className = 'state-no-selection';
        });
        await page.waitForTimeout(500);
        await takeScreenshot(page, 'final-state');

    } catch (error) {
        console.error('\n✗ Error during test execution:', error.message);
        testResults.errors.push(error.message);
        testResults.failed++;
        
        // Take error screenshot if page exists
        if (page) {
            try {
                await takeScreenshot(page, 'error-state');
            } catch (screenshotError) {
                console.error('  Could not take error screenshot:', screenshotError.message);
            }
        }
    } finally {
        // Close browser
        if (browser) {
            await browser.close();
            console.log('\nBrowser closed.');
        }
    }

    return testResults;
}

async function generateReport(results) {
    const report = `
=== PLAYWRIGHT UI STATE TEST REPORT ===
Generated: ${new Date().toISOString()}
Test URL: ${TEST_URL}

SUMMARY:
--------
Total Tests: ${results.total}
Passed: ${results.passed}
Failed: ${results.failed}
Success Rate: ${results.total > 0 ? Math.round((results.passed / results.total) * 100) : 0}%

SCREENSHOTS:
------------
${results.screenshots.map(s => `• ${s.name}: ${s.path}`).join('\n')}

ERRORS:
-------
${results.errors.length > 0 ? results.errors.map(e => `• ${e}`).join('\n') : 'No errors'}

DETAILED RESULTS:
-----------------
${results.total === 1 && results.passed === 1 ? '✓ All UI state tests completed successfully' : '✗ Some tests failed'}

RECOMMENDATIONS:
----------------
${results.failed === 0 ? 
    'All UI state tests passed. The HTML interface is working correctly with mock ACAPI.' :
    'Some tests failed. Check the errors above and review the HTML interface implementation.'}
`;

    // Write report to file
    fs.writeFileSync(REPORT_FILE, report);
    console.log(`\nReport saved to: ${REPORT_FILE}`);

    // Also output to console
    console.log(report);
}

// Main execution
async function main() {
    console.log('Starting Playwright UI state tests...\n');
    
    try {
        const results = await runUITests();
        await generateReport(results);
        
        // Exit with appropriate code
        process.exit(results.failed === 0 ? 0 : 1);
    } catch (error) {
        console.error('Fatal error in test runner:', error);
        process.exit(1);
    }
}

// Run if called directly
if (require.main === module) {
    main();
}

module.exports = { runUITests, generateReport };