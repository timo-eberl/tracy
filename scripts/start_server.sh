#!/bin/bash

# strict mode
set -e

# find project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/.."

cd "$PROJECT_ROOT"

echo "Starting Server"
if [ "$1" == "--rebuild" ]; then
	echo "Rebuilding Docker image"
	docker compose up --build webapp
else
	docker compose up webapp
fi
