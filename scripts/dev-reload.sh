#!/bin/bash 

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE} waks_cstd system watcher has started... ${NC}"

run_build() {
  clear
  echo -e "${BLUE} Rebuilding waks_cstd... ${NC}"
  
  mkdir -p build 
  cd build

  if cmake .. && make; then
        echo -e "${GREEN}Build Successful! Executing Tests...${NC}"
        ./waks_test
  else
        echo -e "\033[0;31m Build Failed. Check the logs above.${NC}"
  fi
  cd ..
}

export -f run_build

# Use 'find' to track all .c and .h files, then pipe to 'entr'
# The '-r' flag restarts the process if new files are added
find src include -name "*.c" -o -name "*.h" | entr -r bash -c run_build
