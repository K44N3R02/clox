#!/bin/bash

# Configuration
INTERPRETER="./clox"               # Path to your standard interpreter executable
DEBUG_INTERPRETER="./clox-dbg"     # Path to your debug interpreter executable
TEST_ROOT_DIR="test"               # Root directory containing all test categories
RESULT_DIR="test-result"           # Directory to store test results (e.g., diffs and traces)

# --- End of Configuration ---

# Create the result directory if it doesn't exist
mkdir -p "$RESULT_DIR"

# Keep track of total results
total_tests_run=0
total_tests_passed=0

# Loop through each topic directory in the test root
for category_dir in "$TEST_ROOT_DIR"/*; do
    if [ -d "$category_dir" ]; then
        category_name=$(basename "$category_dir")
        echo "========================================"
        echo "Testing category: $category_name"

        category_tests_run=0
        category_tests_passed=0

        # Loop through each test file in the category directory
        for test_file in "$category_dir"/*.lox; do
            if [ -f "$test_file" ]; then
                test_base_name=$(basename "$test_file" .lox)

                # Check for corresponding .expected file
                expected_file="${test_file%.lox}.exp"
                if [ -f "$expected_file" ]; then

                    # Run the interpreter and capture its output
                    # Store the actual output in a temporary file for diffing
                    actual_output_file="$RESULT_DIR/${test_base_name}.out"
                    "$INTERPRETER" "$test_file" > "$actual_output_file" 2>&1

                    # Read the expected output
                    expected_output=$(cat "$expected_file")

                    # Compare the output
                    if diff -q "$actual_output_file" "$expected_file" > /dev/null; then
                        echo "  ✅ PASS: $test_base_name"
                        ((category_tests_passed++))
                        # Clean up actual output file if test passed
                        rm "$actual_output_file"
                    else
                        echo "  ❌ FAIL: $test_base_name"
                        echo "    - Category: $category_name"

                        # Generate and save the diff
                        diff_file="$RESULT_DIR/${test_base_name}.diff"
                        diff -u "$expected_file" "$actual_output_file" > "$diff_file"

                        # Run the debug interpreter and save the trace
                        trace_file="$RESULT_DIR/${test_base_name}.trace"
                        echo "    - Diff saved to '$diff_file'."
                        echo "    - Debug trace saved to '$trace_file'."

                        "$DEBUG_INTERPRETER" "$test_file" > "$trace_file" 2>&1
                    fi
                    ((category_tests_run++))
                else
                    echo "  ⚠️  SKIP: $test_base_name - Missing expected output file."
                fi
            fi
        done

        # Print category summary
        echo "----------------------------------------"
        echo "Category Summary: $category_name"
        echo "Passed $category_tests_passed out of $category_tests_run tests."
        echo ""

        # Add to total counts
        ((total_tests_run+=category_tests_run))
        ((total_tests_passed+=category_tests_passed))
    fi
done

# Print final summary
echo "========================================"
echo "FINAL TEST SUMMARY"
echo "Passed $total_tests_passed out of $total_tests_run total tests."
echo "========================================"
