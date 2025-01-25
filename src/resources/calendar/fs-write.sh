#!/bin/bash

TARGET_DIR="./holidays"

itctl fs mkdir $TARGET_DIR

for file in "$TARGET_DIR"/*; do
  # ファイルならコマンドを実行
  if [ -f "$file" ]; then
    name=$(basename "$file")
    echo "Processing $name"
    itctl fs write "$file" "holidays/$name"
  fi
done
