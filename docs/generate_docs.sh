#!/bin/bash

# Script to generate Doxygen documentation for Vista
# This script should be run from the project root directory

echo "Generating Vista documentation with Doxygen..."

# Change to docs directory
cd "$(dirname "$0")"

# Run Doxygen
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo "Documentation generated successfully!"
    echo "Open docs/html/index.html in your browser to view the documentation."
else
    echo "Error generating documentation!"
    exit 1
fi
