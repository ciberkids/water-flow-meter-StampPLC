# Agent Behavior: Planning Mode Requirements

- **Strict Planning:** Always enter "Planning Mode" and generate an `implementation_plan.md` before modifying any files if the task involves more than two files.
- **Architectural Changes:** Any task involving database schema updates, new API endpoints, or dependency changes REQUIRES a plan first.
- **Confirmation:** Do not execute terminal commands or file writes until the user has explicitly approved the plan artifact.
