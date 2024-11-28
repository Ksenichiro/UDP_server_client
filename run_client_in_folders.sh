#!/bin/bash

# Path to the Client executable (update if needed)
CLIENT_EXECUTABLE="../build/Client"
CONFIG_FILE="../client_config.txt"
# Check if the Client executable exists
# if [ ! -f "$CLIENT_EXECUTABLE" ]; then
#   echo "Client executable not found at $CLIENT_EXECUTABLE"
#   exit 1
# fi

# Create 10 folders and run the Client executable
for i in {1..10}; do
  FOLDER_NAME="Folder_$i"
  
  # Create the folder
  mkdir -p "$FOLDER_NAME"
  
  # Navigate into the folder
  cd "$FOLDER_NAME" || exit

  cp "$CONFIG_FILE" "./"

  # Run the Client executable
  echo "Running Client from $FOLDER_NAME..."
  "$CLIENT_EXECUTABLE"  &
  
  # Navigate back to the original directory
  cd ..
done

echo "All tasks completed."
