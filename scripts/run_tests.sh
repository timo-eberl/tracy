#!/bin/bash

# strict mode
set -e

# find project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/.."

cd "$PROJECT_ROOT"

echo "Running tests"
if [ "$1" == "--rebuild" ]; then
	echo "Rebuilding Docker image"
	docker compose up --build
else
	docker compose up
fi
