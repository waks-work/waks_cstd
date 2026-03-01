#!/bin/bash
echo "🏗️ Building cross-language project..."

# C/C++ FFI projects
if [ -f "CMakeLists.txt" ]; then
    # Build C components first
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    cd ..
    
    echo "✅ Cross-language build complete!"
    exit 0
fi

echo "ℹ️  No cross-language build configuration detected"
