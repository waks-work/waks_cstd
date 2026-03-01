#!/bin/bash 

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE} waks_cstd system watcher started... ${NC}"

run_build() {
  clear
  echo -e "${BLUE}🔨 Rebuilding waks_cstd library...${NC}"
  
  # This will produce the .a file that mini_os needs.
  if zig build; then
        echo -e "${GREEN} Library Build Successful!${NC}"
  else
        echo -e "${RED} Build Failed. Check src/ or build.zig${NC}"
  fi
}

export -f run_build

# Watch C files, Headers, and the Build script itself
find src include build.zig -name "*.c" -o -name "*.h" -o -name "*.zig" | entr -r bash -c run_build
