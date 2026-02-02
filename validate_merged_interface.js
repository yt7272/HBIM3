// Validation script for merged HTML interface
const fs = require('fs');
const path = require('path');

// Read the HTML file
const htmlPath = path.join(__dirname, 'MergedPlugin', 'RFIX', 'Selection_Test.html');
const htmlContent = fs.readFileSync(htmlPath, 'utf8');

// Check required elements
const checks = [
  { name: 'ComponentInfo tab button', pattern: /<button id="componentinfo-tab" class="tab-button active">ComponentInfo(?:<span[^>]*>.*?<\/span>)?<\/button>/ },
  { name: 'IFC Properties tab button', pattern: /<button id="ifc-properties-tab" class="tab-button">IFC Properties<\/button>/ },
  { name: 'ComponentInfo content active', pattern: /<div id="componentinfo-content" class="tab-content active">/ },
  { name: 'IFC Properties content', pattern: /<div id="ifc-properties-content" class="tab-content">/ },
  { name: 'Component ID input', pattern: /<input type="text" id="component-id" class="form-input"/ },
  { name: 'Photo description input', pattern: /<input type="text" id="photo-description" class="form-input"/ },
  { name: 'Select photos button', pattern: /<button id="select-photos-btn" class="secondary" disabled>选择照片<\/button>/ },
  { name: 'View photos button', pattern: /<button id="view-photos-btn" class="secondary" disabled>查看照片<\/button>/ },
  { name: 'Edit info button', pattern: /<button id="edit-info-btn">编辑信息<\/button>/ },
  { name: 'View IFC properties button', pattern: /<button id="view-ifc-properties-btn">查看构件的IFC属性<\/button>/ },
  { name: 'Version display', pattern: /<div id="summary-version" class="summary-value">/ },
  { name: 'Tab switching JavaScript', pattern: /function switchTab\(tabId\)/ },
  { name: 'Industrial dark theme CSS', pattern: /--bg-dark: #1a1d23;/ }
];

let allPassed = true;
console.log('Validating merged HTML interface...\n');

checks.forEach(check => {
  const passed = check.pattern.test(htmlContent);
  console.log(`✓ ${check.name}: ${passed ? 'PASSED' : 'FAILED'}`);
  if (!passed) {
    allPassed = false;
  }
});

console.log(`\nOverall result: ${allPassed ? 'ALL TESTS PASSED' : 'SOME TESTS FAILED'}`);

if (allPassed) {
  console.log('\n✅ The merged HTML interface meets all requirements:');
  console.log('  - Industrial dark theme from IFCPropertyReader preserved');
  console.log('  - Tabbed navigation with ComponentInfo and IFC Properties tabs');
  console.log('  - ComponentInfo tab is active by default');
  // ... other requirements
} else {
  console.log('\n❌ Some requirements are not met. Please check the failed tests above.');
  process.exit(1);
}