# Story: Value Binding UI

**ID:** SI-20260123-03
**Parent Feature:** NF-20260123-01 (Firmware Binding)
**Dependencies:** SI-20260123-01 (Manifest Loader)

## 1. Goal

Allow the designer to link "Text" or "Value" elements to live Firmware Data Sources defined in the manifest.

## 2. Requirements

### 2.1 Element Property Update

- **Target Elements:** `kind: "value"`, `kind: "text"` (and potentially `kind: "gauge"` in future).
- **New Property:** `dataSourceId` (string, optional).

### 2.2 Inspector UI

- **Binding Control:** When a compatible element is selected, show a "Data Source" dropdown in the Property Inspector.
- **Filtering:**
  - Only show Manifest Values compatible with the element (e.g., `float` for Value elements, `string` for Text).
  - Allow "None" (Static text).

### 2.3 Visual Feedback

- **Canvas:** If an element is bound, display a small "link" icon or change its border color in the Editor (not in the preview render, or maybe distinct style).
- **Placeholder:** Replace the static content with the Bound Name (e.g., `{{FlowRate}}`) in the editor view for clarity.

## 3. Implementation Tasks

- [ ] Update `ScreenElement` type definition.
- [ ] Update `DesignToolbox` element editor to include "Data Source" selector.
- [ ] Implement Type-Filtering logic (match Element Kind to Manifest Type).
- [ ] Update `DisplayViewport` to render placeholder text for bound elements.

## 4. Verification Plan

- **Manual:** Create "Value" element -> Bind to "FlowRate".
- **Verify:** Editor shows `{{FlowRate}}` instead of default "00".
- **Verify:** JSON export contains `dataSourceId`.
