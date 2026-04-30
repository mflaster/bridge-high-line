# Contributing and Maintenance Guide

Welcome! We appreciate your interest in contributing to the Bridge Workspace.
To maintain the quality and clarity of our codebase and documentation, please
adhere to the following guidelines.

## Documentation Standards

- **Modular Approach:** Keep documentation separated by topic (Setup, Operations,
  Architecture). Do not create monolithic files.
- **Format:** All documentation must be written in standard Markdown.
- **Linking:** Ensure that internal links between Markdown files remain
  functional when renaming or moving files.
- **Command Explanations:** When adding new operations or `make` targets, always
  document *why* the command is used, not just *how* to type it.

## Contribution Workflow

1. **Create a Branch:** Start your work on a new branch named after the feature
   or fix.
2. **Make Changes:** Update the code and relevant documentation.
3. **Build and Test:** Ensure that `make build` and `make test` complete
   successfully without errors.
4. **Update Documentation:** If your changes affect the architecture, setup, or
   operations, update the corresponding files in the `docs/` directory.
5. **Submit a Pull Request:** Open a PR and request a review from a maintainer.

Thank you for helping us keep the Bridge Workspace well-documented and
maintainable!
