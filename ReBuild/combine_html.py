#!/usr/bin/env python3
import os
import sys

# Paths
project_root = os.path.dirname(os.path.abspath(__file__))
newplugin_dir = os.path.join(project_root, "NewPlugin")
rfix_dir = os.path.join(newplugin_dir, "RFIX")
drafts_dir = os.path.join(project_root, ".sisyphus/drafts")

# Input files
index_new = os.path.join(rfix_dir, "index_new.html")
draft_layout = os.path.join(drafts_dir, "prototype-layout.html")
original_backup = os.path.join(rfix_dir, "index.html.backup")
# Output file
output_file = os.path.join(rfix_dir, "index.html")

# Read index_new.html (head + opening body)
with open(index_new, "r", encoding="utf-8") as f:
    head_content = f.read()
print(f"Read {len(head_content)} chars from index_new.html")

# Read draft layout lines 473-558 (body content)
with open(draft_layout, "r", encoding="utf-8") as f:
    draft_lines = f.readlines()
# Find line numbers for body content (lines 473-558)
# Actually we need from line where <body class="state-no-selection"> appears (line 472?)
# Let's search for the line number of body tag
body_start = None
for i, line in enumerate(draft_lines):
    if '<body class="state-no-selection">' in line:
        body_start = i
        break
if body_start is None:
    print("Error: body tag not found in draft layout")
    sys.exit(1)
# Assume body content starts after body tag
body_content_start = body_start + 1
# Find closing </body> tag
body_end = None
for i in range(body_start, len(draft_lines)):
    if "</body>" in draft_lines[i]:
        body_end = i
        break
if body_end is None:
    print("Error: </body> not found in draft layout")
    sys.exit(1)
# Extract body content lines (excluding body tags)
body_content_lines = draft_lines[body_content_start:body_end]
print(f"Extracted {len(body_content_lines)} lines of body content")

# Read original backup to extract IFC properties section (lines 684-719)
with open(original_backup, "r", encoding="utf-8") as f:
    original_lines = f.readlines()
# Extract lines 684-719 (1-indexed)
ifc_section_lines = original_lines[683:719]  # slice end exclusive
print(f"Extracted {len(ifc_section_lines)} lines of IFC properties section")

# Now we need to insert IFC properties section into body content.
# Find group-3 closing div. We'll search for </div> that closes group-3.
# We'll do a simple insertion after the edit-mode-buttons div.
# Let's first combine body content into a single string.
body_content = "".join(body_content_lines)

# Insert IFC properties section after the edit-mode-buttons div.
# Find position of edit-mode-buttons div closing tag.
# We'll search for 'edit-mode-buttons' and then find the next '</div>' after that.
# For simplicity, we'll just append IFC properties section before the closing div of group-3.
# We'll locate the last '</div>' that is after group-3 opening.
# Let's do a simple approach: replace the closing </div> of group-3 with IFC section + </div>
# We'll find the line that contains '<!-- JavaScript桥接状态显示 -->' and insert before that.
# Actually body_content includes that line. Let's split lines.
body_lines = body_content.splitlines(keepends=True)
# Find index of line containing 'JavaScript桥接状态显示'
insert_index = None
for i, line in enumerate(body_lines):
    if "JavaScript桥接状态显示" in line:
        insert_index = i
        break
if insert_index is None:
    print("Error: Could not find bridge status line")
    sys.exit(1)

# Insert IFC properties section before that line
ifc_section = "".join(ifc_section_lines)
# Modify IFC section to add class ifc-properties-section wrapper.
# The original IFC properties content starts with <div id="ifc-properties-content" class="tab-content">
# We need to change that to <div class="ifc-properties-section"> and keep inner content.
# Let's do string replacement.
ifc_section = ifc_section.replace(
    'id="ifc-properties-content" class="tab-content"', 'class="ifc-properties-section"'
)
# Also remove any tab-related classes if present.
ifc_section = ifc_section.replace("tab-content", "")

# Insert
body_lines.insert(insert_index, "\n" + ifc_section + "\n")
body_content = "".join(body_lines)

# Now we need to read the full JavaScript from original backup (lines 721 to end)
# Find line where <script> starts (line 721)
script_start = None
for i, line in enumerate(original_lines):
    if '<script type="text/javascript">' in line:
        script_start = i
        break
if script_start is None:
    print("Error: script tag not found in original backup")
    sys.exit(1)
# Find closing </script> tag after script_start
script_end = None
for i in range(script_start, len(original_lines)):
    if "</script>" in original_lines[i]:
        script_end = i
        break
if script_end is None:
    print("Error: </script> not found")
    sys.exit(1)
# Extract script lines (including script tags)
script_lines = original_lines[script_start : script_end + 1]
script_content = "".join(script_lines)
print(f"Extracted {len(script_lines)} lines of script")

# Now we need to modify script content:
# 1. Remove tab switching functions (switchTab, tab event listeners)
# 2. Update references to version element (summary-version -> version-display)
# 3. Integrate three-state management: add updateUIState and hasComponentInfo functions, call updateUIState where appropriate.
# 4. Ensure event listeners target correct elements (some IDs may have changed).
# This is complex; we'll do basic replacements.

# Remove lines containing switchTab function definition and event listeners for tabs.
lines = script_content.splitlines(keepends=True)
filtered_lines = []
i = 0
while i < len(lines):
    line = lines[i]
    # Skip function switchTab definition and its body
    if "function switchTab" in line:
        # Skip until closing brace
        brace_count = 0
        while i < len(lines):
            l = lines[i]
            brace_count += l.count("{") - l.count("}")
            filtered_lines.append("")  # keep line count but empty
            i += 1
            if brace_count == 0:
                break
        continue
    # Skip event listeners for componentinfo-tab and ifc-properties-tab
    if "componentinfo-tab" in line or "ifc-properties-tab" in line:
        # Skip this line and possibly next line if it's a line continuation
        # For simplicity, skip the line
        i += 1
        continue
    filtered_lines.append(line)
    i += 1

script_content = "".join(filtered_lines)

# Replace summary-version with version-display
script_content = script_content.replace("summary-version", "version-display")

# Add three-state management functions at the beginning of script (after variable declarations)
# We'll insert after the let/const declarations.
# Find line where first function appears
lines = script_content.splitlines(keepends=True)
insert_pos = 0
for idx, line in enumerate(lines):
    if line.strip().startswith("function ") or line.strip().startswith("//"):
        insert_pos = idx
        break

# Define three-state functions
three_state_functions = """
// Three-state CSS class management
function updateUIState() {
    const body = document.body;
    if (!currentElementGuid) {
        // 状态1：未选择构件
        body.classList.remove('state-no-info', 'state-has-info');
        body.classList.add('state-no-selection');
    } else if (!hasComponentInfo()) {
        // 状态2：已选择构件但未填写信息
        body.classList.remove('state-no-selection', 'state-has-info');
        body.classList.add('state-no-info');
    } else {
        // 状态3：已选择构件有信息
        body.classList.remove('state-no-selection', 'state-no-info');
        body.classList.add('state-has-info');
    }
    updateUnsavedIndicator();
}

function hasComponentInfo() {
    const componentId = document.getElementById('component-id').value;
    const photoDescription = document.getElementById('photo-description').value;
    return componentId || photoDescription || currentPhotoPaths.length > 0;
}
"""

# Insert
lines.insert(insert_pos, three_state_functions)
script_content = "".join(lines)

# Now we need to call updateUIState at appropriate places:
# - After element selection changes (UpdateSelectedElements)
# - After edit mode changes
# - After saving component info
# We'll do simple search and replace later.

# For now, we'll just add a call to updateUIState inside UpdateSelectedElements after setting currentElementGuid.
# Let's find UpdateSelectedElements function.
# We'll do a more robust modification later.

# Combine everything: head_content + body_content + script_content + closing tags
# head_content already includes opening body tag. We need to insert body_content after opening body tag.
# Find position of '<body class="state-no-selection">' in head_content
body_tag_pos = head_content.find('<body class="state-no-selection">')
if body_tag_pos == -1:
    print("Error: body tag not found in head_content")
    sys.exit(1)
# Insert body_content after the body tag line (right after the tag)
# We'll split head_content into before and after body tag (including the tag)
before = head_content[:body_tag_pos]
after = head_content[body_tag_pos:]
# Insert newline and body content after the opening tag
new_content = (
    before + after + "\n" + body_content + "\n" + script_content + "\n</body>\n</html>"
)

# Write output
with open(output_file, "w", encoding="utf-8") as f:
    f.write(new_content)
print(f"Written combined HTML to {output_file}")
print("Done.")
