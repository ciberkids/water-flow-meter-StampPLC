# Story: SI-20251111-05-spike-firmware-manifest-pattern — Spike: Firmware Function Manifest Strategy

> **Naming Convention**  
> Store active story specs under `docs/missing implementation/` using the identifier format above, for example `SI-20250517-01-led-diagnostics.md`.  
> Reference the story from `docs/stories to implement/missing_implementation_stories.md` with the same ID so incremental progress stays traceable.

**Status:** pending  
**Author:** Matteo  
**Last Updated:** 2025-11-11  
**Linked Requirements:** docs/Requirements/feature addition/Display_Web_UI_Workspace_Improvements.md#open-questions--follow-up  
**Related Features:** Display Web UI workspace, Firmware exporter

---

## 1. Summary

Investigate design patterns for gathering UI/Modbus-exposed firmware functions into a JSON manifest. The spike must evaluate C++ techniques (e.g., registries, macros, constexpr descriptors) that enable automatic manifest generation, document trade-offs, and recommend the path forward before full implementation (SI-20251111-03).

---

## 2. Acceptance Criteria

- [ ] Survey at least two viable approaches (e.g., constexpr descriptor tables vs. macro-based registries) and document pros/cons.
- [ ] Produce a prototype that emits sample JSON matching the desired schema for a subset of `UiActionRegistry` entries.
- [ ] Recommend the preferred approach with rationale, risks, and migration steps.

---

## 3. Implementation Notes

- Focus on discoverability, minimal boilerplate for firmware developers, and compatibility with PlatformIO builds.
- Consider leveraging existing registries (e.g., `UiActionRegistry`, Modbus setter/getter tables) to avoid duplicating metadata.
- Prototype can live in a sandbox file; final production implementation deferred to SI-20251111-03.

---

## 4. Tasks & Checklist

- [ ] Collect current firmware action definitions and identify metadata gaps.
- [ ] Prototype approach A (constexpr descriptors) and capture emitted JSON.
- [ ] Prototype approach B (macro/registration helpers) and capture emitted JSON.
- [ ] Compare approaches (performance, maintainability, tooling effort) and write spike report.
- [ ] Share findings with stakeholders for sign-off.

---

## 5. Verification Plan

1. Validate prototype output against the draft JSON schema (Ajv or similar).
2. Ensure prototypes build/run inside the PlatformIO environment.
3. Conduct design review walkthrough of findings.

---

## 6. Backout / Fallback Strategy

- Spike is non-production; revert by discarding prototype files if necessary.
- Document assumptions so future revisions can revisit decisions.

---

## 7. Notes & Open Questions

- Output feeds directly into SI-20251111-03 and Simulation/Design stories.
- Capture any tooling requirements (e.g., custom CMake steps) uncovered during the spike.
