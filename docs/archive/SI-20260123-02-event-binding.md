# Story: Event Binding UI

**ID:** SI-20260123-02
**Parent Feature:** NF-20260123-01 (Firmware Binding)
**Dependencies:** SI-20260123-01 (Manifest Loader)

## 1. Goal

Allow the designer to map "Physical Button Presses" on a specific screen to "Firmware Actions" and subsequent "Screen Transitions".

## 2. Requirements

### 2.1 Event Panel

- **Location:** New Sidebar Panel or Tab in `DesignToolbox`.
- **Context:** Active depending on Selected Screen.

### 2.2 Interaction Model

- **List of Inputs:** Default to M5Stack standard (Btn A, Btn B, Btn C, Encoder Push, Encoder Rotate).
- **Configuration Row per Input:**
  - **Trigger**: (e.g. "Button A Click").
  - **Action**: Dropdown list of `ManifestAction`s (loaded from Story 01).
  - **Navigation** (Optional): Dropdown list of existing Screens to transition to after action.

### 2.3 Data Structure Update

- Update `ScreenDefinition` type to include:

    ```typescript
    events: {
      trigger: string; // "btn_a", "btn_b", etc.
      actionId?: string; // from Manifest
      nextScreenId?: string; // from Screen List
    }[]
    ```

## 3. Implementation Tasks

- [ ] Update `types.ts` `ScreenDefinition` to support `events`.
- [ ] Create `EventBindingPanel` component.
- [ ] Implement Dropdowns populated by `ManifestContext` and `ScreenList`.
- [ ] Update `App.tsx` to save these bindings to the dataset.
- [ ] Visual: Show a small indicator on the Screen thumbnail if it has custom bindings.

## 4. Verification Plan

- **Manual:** Select Screen -> Bind "Button A" to "Save" -> Set Next Screen "Home".
- **Verify:** JSON export contains the event mapping correctly.
