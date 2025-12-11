#!/bin/bash
# Run all string module tests

cd "$(dirname "$0")/../../.."
PROJECT_ROOT="$(pwd)"

echo "Running string module tests..."
echo "=============================="

passed=0
failed=0

for test in src/string/t/*.co; do
    # Skip README
    if [[ "$test" == *"README"* ]]; then
        continue
    fi
    
    testname=$(basename "$test" .co)
    echo -n "Running $testname... "
    
    # Compile and run test
    if ./build/come build "$test" -o "/tmp/come_test_$$" 2>/dev/null; then
        if "/tmp/come_test_$$" 2>/dev/null; then
            echo "✓ PASS"
            ((passed++))
        else
            echo "✗ FAIL (runtime)"
            ((failed++))
        fi
        rm -f "/tmp/come_test_$$"
    else
        echo "✗ FAIL (compile)"
        ((failed++))
    fi
done

echo "=============================="
echo "Results: $passed passed, $failed failed"

if [ $failed -eq 0 ]; then
    exit 0
else
    exit 1
fi
