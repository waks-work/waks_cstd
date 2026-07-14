#!/usr/bin/env bash

find src include tests |
entr -r ./scripts/test.sh
