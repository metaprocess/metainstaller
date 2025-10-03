#!/bin/bash

# Test script for file browsing functionality

echo "Testing File Browsing Functionality"
echo "==================================="

# Test server availability
echo "Checking if server is running..."
curl -s http://localhost:8080/api/file/list/ >/dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: Server is not running or not accessible on port 8080"
    echo "Please start the server before running this test"
    exit 1
fi

echo "Server is accessible"

# Test basic directory listing
echo
echo "Testing basic directory listing..."
curl -s http://localhost:8080/api/file/list/ | jq '.'

# Test detailed directory listing
echo
echo "Testing detailed directory listing..."
curl -s http://localhost:8080/api/file/list-detailed/ | jq '.'

# Test directory tree
echo
echo "Testing directory tree..."
curl -s http://localhost:8080/api/file/tree/ | jq '.'

# Test with a specific directory (if it exists)
echo
echo "Testing with 'src' directory..."
curl -s http://localhost:8080/api/file/list-detailed/src | jq '.'

echo
echo "Test completed successfully!"
