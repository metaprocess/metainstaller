#!/bin/bash

# Test script for FileManager REST endpoints

echo "Testing FileManager REST endpoints..."

# Start the server in the background
echo "Starting server..."
./MetaInstaller --test=filemanager &

# Give the server time to start
sleep 3

# Test the list directory endpoint
echo "Testing list directory endpoint..."
curl -X GET http://localhost:8080/api/file/list/

# Test the get directory tree endpoint
echo "Testing get directory tree endpoint..."
curl -X GET http://localhost:8080/api/file/tree/

# Test the get file endpoint (with a non-existent file)
echo "Testing get file endpoint with non-existent file..."
curl -X GET http://localhost:8080/api/file/get/nonexistent.txt

# Test the delete file endpoint (with a non-existent file)
echo "Testing delete file endpoint with non-existent file..."
curl -X DELETE http://localhost:8080/api/file/delete/nonexistent.txt

# Kill the server
echo "Stopping server..."
pkill -f MetaInstaller

echo "Test completed."
