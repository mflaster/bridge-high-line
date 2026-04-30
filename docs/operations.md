# Operations Guide

This guide explains how to build, test, and operate the Bridge Solver using the
provided `Makefile`.

## Build and Test Instructions

We use `make` to automate common operational tasks. By adhering to these
commands, you ensure consistent builds and test executions.

### `make all`

- **Purpose:** This is the default target. It triggers the `build` target to
  compile the source code.
- **Why use it:** Running `make` or `make all` is the quickest way to compile
  the project from scratch without needing to specify a specific target.

### `make build`

- **Purpose:** Compiles the solver's source files into an executable binary.
- **Why use it:** Use this command after modifying the codebase to generate a
  fresh, executable version of the solver. It handles the specific compiler
  flags and linking steps automatically.

### `make test`

- **Purpose:** Executes the full suite of unit and integration tests.
- **Why use it:** Run this command to verify that your changes have not broken
  existing functionality. It provides a clean-slate verification of the solver's
  correctness.

### `make clean`

- **Purpose:** Removes compiled binaries, object files, and other build
  artifacts.
- **Why use it:** Use this command to reset your workspace. It is particularly
  useful before committing code or when you want to ensure a completely clean
  build.

## Example Workflow

A typical developer workflow looks like this:

1. Make changes to the source code.
2. Run `make build` to compile the changes.
3. Run `make test` to ensure all tests pass.
4. If issues occur, run `make clean` and rebuild to eliminate artifact caching
   issues.
