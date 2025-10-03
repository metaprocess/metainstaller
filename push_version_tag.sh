#!/bin/bash

# Script to push a git tag with the version from .env.version

# Check if .env.version file exists
if [ ! -f ".env.version" ]; then
    echo "Error: .env.version file not found!"
    exit 1
fi

# Extract VERSION value from .env.version
VERSION=$(grep -E "^VERSION=" .env.version | cut -d'=' -f2)

# Check if VERSION is set
if [ -z "$VERSION" ]; then
    echo "Error: VERSION not found in .env.version file!"
    exit 1
fi

# Trim any whitespace from the version
VERSION=$(echo "$VERSION" | xargs)

echo "Found version: $VERSION"

# Create git tag
git tag "$VERSION"

# Check if tag creation was successful
if [ $? -eq 0 ]; then
    echo "Git tag '$VERSION' created successfully."
else
    echo "Error: Failed to create git tag '$VERSION'."
    exit 1
fi

# Push the tag to remote
git push origin "$VERSION"

# Check if push was successful
if [ $? -eq 0 ]; then
    echo "Git tag '$VERSION' pushed to remote successfully."
else
    echo "Error: Failed to push git tag '$VERSION' to remote."
    exit 1
fi

echo "Operation completed successfully."
