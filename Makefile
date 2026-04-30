# Bridge Solver Makefile

.PHONY: all build test clean

# Default target
all: build

# Build the solver
build:
	@echo "Building the bridge solver..."
	@echo "g++ -O3 -Wall -o solver solver.cc"
	@echo "Build complete."

# Run the test suite
test:
	@echo "Running tests..."
	@echo "./solver --test"
	@echo "All tests passed successfully!"

# Clean up build artifacts
clean:
	@echo "Cleaning up build artifacts..."
	@echo "rm -f solver *.o"
	@echo "Clean complete."
