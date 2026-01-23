# Story: Manifest Schema & Loader

**ID:** SI-20260123-01
**Parent Feature:** NF-20260123-01 (Firmware Binding)

## 1. Goal

Enable the UI Designer to load a manual `firmware_manifest.json` file. This file describes the available Firmware Actions and Values that can be bound to UI elements.

## 2. Requirements

### 2.1 JSON Schema Definition

Define a strict TypeScript Interface and JSON Schema for the manifest.

- **Types:**
  - `ManifestAction`: `{ id: string, name: string, description?: string, triggers?: string[] }`
  - `ManifestValue`: `{ id: string, name: string, type: 'int' | 'float' | 'string' | 'bool', readOnly: boolean }`
  - `Manifest`: `{ actions: ManifestAction[], values: ManifestValue[] }`

### 2.2 UI Loader Implementation

- **Component:** `ManifestLoader` (part of DesignToolbox or global settings).
- **Functionality:**
  - Button: "Load Firmware Manifest".
  - Action: Browser File Picker (or drag-and-drop).
  - Validation: Parse JSON and validate against Schema. Error if invalid.
- **State Management:** Store the loaded Manifest in `ScreenDataset` (or a parallel generic context `FirmwareContext`) so it persists while editing.

### 2.3 Persistence (Optional for MVP)

- If possible, save the loaded manifest content alongside the project json, or just require reloading on refresh (MVP: Require Reload).

## 3. Implementation Tasks

- [ ] Define `Manifest.ts` types in `src/types/`.
- [ ] Create `loadManifest()` utility with Zod/AJV validation.
- [ ] Add "Load Manifest" button to `DesignToolbox` header.
- [ ] Store manifest in React State (`useFirmwareManifest` hook/context).
- [ ] Verify loading a sample `manifest.json` works and populates the store.

## 4. Verification Plan

- **Manual:** Create a valid `manifest.json`. Load it. Verify state contains actions/values.
- **Error Handling:** Load a malformed JSON. Verify User-Friendly Error Message.
