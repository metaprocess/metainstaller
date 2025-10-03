# File Browser for Web Applications

This document describes the enhanced file browsing functionality implemented in the FileManager module, specifically designed for web application file pickers.

## Overview

The FileManager module has been enhanced with detailed file information capabilities to support web-based file browsing applications. The new functionality provides richer file metadata that is essential for a user-friendly file picker interface.

## New Features

### Detailed File Information
The new `listDirectoryDetailed` function returns comprehensive information about files and directories:

- **Name**: File or directory name
- **Type**: Whether the item is a file or directory
- **Size**: File size in bytes (0 for directories)
- **Last Modified**: Timestamp of the last modification

### New API Endpoints

1. **GET `/api/file/list-detailed/<path>`**
   - Returns detailed information about files and directories
   - Response format:
     ```json
     {
       "success": true,
       "path": "current/path",
       "entries": [
         {
           "name": "filename.txt",
           "is_directory": false,
           "size": 1024,
           "last_modified": "2025-08-16 15:30:45"
         },
         {
           "name": "subdirectory",
           "is_directory": true,
           "size": 0,
           "last_modified": "2025-08-16 14:20:10"
         }
       ]
     }
     ```

2. **GET `/api/file/list/<path>`** (unchanged)
   - Maintains backward compatibility
   - Returns simple file names only

## Implementation Details

### FileInfo Structure
```cpp
struct FileInfo {
    std::string name;
    bool is_directory;
    size_t size;
    std::string last_modified;
};
```

### File Sorting
Files are sorted with directories appearing first, followed by files, both in alphabetical order.

### Time Formatting
Modification times are formatted as `YYYY-MM-DD HH:MM:SS` for consistent display across different clients.

## Web Application Integration

### File Picker Example
A sample HTML/JavaScript file picker is included in `frontend/file_picker.html` that demonstrates:

- Directory navigation
- File selection
- File content viewing
- Responsive design with file icons
- Human-readable file sizes
- Last modified timestamps

### API Usage
To integrate with your web application:

1. **List directory contents**:
   ```javascript
   fetch('/api/file/list-detailed/' + encodeURIComponent(path))
     .then(response => response.json())
     .then(data => {
       // Process data.entries array
     });
   ```

2. **Get file content**:
   ```javascript
   fetch('/api/file/get/' + encodeURIComponent(filePath))
     .then(response => response.json())
     .then(data => {
       // Access data.content
     });
   ```

## Testing

Run the test script to verify functionality:
```bash
./test_file_browsing.sh
```

This script tests:
- Server availability
- Basic directory listing
- Detailed directory listing
- Directory tree structure
- Specific directory access

## Security Considerations

- All file paths are resolved relative to the configured base directory
- Path traversal attempts (with `..`) are blocked
- Only files within the base directory can be accessed

## Future Enhancements

Possible future improvements:
- File type icons based on MIME types
- File search functionality
- File filtering by type or size
- Batch operations (copy, move, etc.)
