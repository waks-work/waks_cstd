#!/bin/bash
echo "🚀 Setting up waks_cstd..."

# C waks_cstd project.  
if [ -f "CMakeLists.txt" ] || [ -f "Makefile" ]; then
    echo "🔧 C project detected"
    sudo pacman -S --needed base-devel cmake gcc clang 2>/dev/null || true
    mkdir -p build
    echo "✅ C build system ready"
fi

# Setup git hooks
cp .github/hooks/* .git/hooks/ 2>/dev/null || true
chmod +x .git/hooks/* 2>/dev/null || true

echo "✅ Project setup complete for waks_cstd development!"
