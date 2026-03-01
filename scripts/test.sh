#!/bin/bash
echo "🧪 Running tests for waks_cstd..."

# C waks_cstd project.
[ -f "Makefile" ] && make test
[ -f "CMakeLists.txt" ] && cd build && ctest .. && cd ..

echo "✅ Tests completed for waks_cstd!"
