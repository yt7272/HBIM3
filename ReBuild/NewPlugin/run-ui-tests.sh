#!/bin/bash

# UI State Test Runner Script
# This script runs the Playwright UI state tests for the ArchiCAD plugin HTML interface

set -e

echo "=== ArchiCAD Plugin UI State Test Runner ==="
echo "Starting test server..."

# Start test server in background
node test-server.js &
SERVER_PID=$!

# Wait for server to start
sleep 2

echo "Test server started (PID: $SERVER_PID)"
echo "Running UI state tests with Playwright..."

# Run the Playwright tests
node playwright-ui-test.js
TEST_EXIT_CODE=$?

echo "Tests completed with exit code: $TEST_EXIT_CODE"

# Kill test server
echo "Stopping test server..."
kill $SERVER_PID 2>/dev/null || true

# Display test report
echo ""
echo "=== TEST REPORT SUMMARY ==="
cat playwright-test-report.txt | grep -A 20 "=== PLAYWRIGHT UI STATE TEST REPORT ==="

# Exit with test code
exit $TEST_EXIT_CODE