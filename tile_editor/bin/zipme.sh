#!/bin/bash

if [ -d "$2" ]; then
    echo "$2 is a directory"
else
    echo "$2 is not a directory"
fi

DEST=$(realpath "$2")

set -e

cd $1

# Get current datetime in format YYYYMMDD_HHMMSS
timestamp=$(date +"%Y%m%d_%H%M%S")

# Define output zip filename
zipname="source_$timestamp.zip"

# Find and zip all .cpp, .h, and .pro files in current directory
zip "$zipname" *.cpp *.h *.pro

# Optional: echo result
echo "Created archive: $zipname"

mv *.zip ${DEST}