#!/bin/bash

# Configuration
SKETCH="."
BOARD="esp32:esp32:esp32:PartitionScheme=min_spiffs"
BUILD_DIR="./build"
LIB_PATH="/run/media/eric/windu/Users/eric/Dropbox/make/arduino/libraries"

# Ensure directories exist for persistent speed gains
mkdir -p "$BUILD_DIR"

echo "--- Starting Ultra-Fast Compile ---"

# --jobs 0: Parallelizes across all CPU cores
# --build-path/--build-cache-path: Prevents recompiling the ESP32 core
# --skip-libraries-discovery: Skips the slow recursive search for libraries

arduino-cli compile --fqbn "$BOARD" \
  --jobs 0 \
  --libraries "$LIB_PATH" \
  --build-path "$BUILD_DIR" \
  --skip-libraries-discovery \
  "$SKETCH"

# Check if compile succeeded
if [ $? -eq 0 ]; then
    echo "--- Compile Success! ---"
    # Optional: arduino-cli upload -p /dev/ttyUSB0 --fqbn "$BOARD" --input-dir "$BUILD_DIR"
else
    echo "--- Compile Failed ---"
    echo "Tip: If a library is 'missing', try removing --skip-libraries-discovery once."
fi