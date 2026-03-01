#!/bin/bash
echo "🔧 Starting C Development environment for waks_cstd..."

# C projects with hot reload
if [ -f "CMakeLists.txt" ] && [ -f "scripts/dev-reload.sh" ]; then
    echo "🔧 Starting C with hot reload..."
    ./scripts/dev-reload.sh
    exit 0
fi

echo "⚠️  No C Development server detected"
echo "💡 Supported: C with hot reload"
