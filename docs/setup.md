# Setup Guide

This guide covers the necessary steps to configure your environment for the
Bridge Solver.

## Dependencies

To build and run the solver, you will need the following dependencies installed
on your system:

- **C++ Compiler:** GCC or Clang with C++17 support or higher.
- **Make:** GNU Make for running build and test commands.
- **Python 3.8+:** (Optional) For running supplementary analysis scripts.

## Environment Configuration

1. **Clone the repository:**

   ```bash
   git clone <repository_url> bridge-workspace
   cd bridge-workspace
   ```

2. **Verify installation:**
   Ensure `make` and your compiler are available in your system's PATH.

   ```bash
   make --version
   g++ --version
   ```

3. **Proceed to Operations:**
   Once your environment is set up, proceed to the
   [Operations Guide](operations.md) for instructions on building and testing
   the solver.
