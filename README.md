# HexViewer

A small Win32 hex reader.

## Use

- Run the app and it opens `HexViewer.cpp` by default.
- Type a file path in the window title area and press Enter to open it.
- Press Escape to cancel the current path.
- Use the arrow keys on the vertical scroll bar to move through the file.
- Press Ctrl + `+` or Ctrl + `-` to change how many bytes are shown per line.

## Notes

- The app reads the whole file into memory.
- Paths are limited to 260 characters.
