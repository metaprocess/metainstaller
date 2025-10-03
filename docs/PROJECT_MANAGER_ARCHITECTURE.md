# ProjectManager Architecture Documentation

## Overview

The ProjectManager is a comprehensive system for managing Docker Compose projects packaged as encrypted 7z archives. It provides a complete workflow from archive analysis to project deployment and management.

## Architecture Components

### 1. Core Classes

#### ProjectManager
- **Purpose**: Main orchestrator for project lifecycle management
- **Location**: `src/ProjectManager.h`, `src/ProjectManager.cpp`
- **Dependencies**: DockerManager, ProcessManager, Utils

#### ProjectInfo
- **Purpose**: Stores metadata about loaded projects
- **Fields**:
  - `name`: Project identifier
  - `archive_path`: Original archive location
  - `extracted_path`: Where project files are extracted
  - `compose_file_path`: Path to docker-compose.yml
  - `required_images`: List of Docker images needed
  - `dependent_files`: Additional files referenced by compose
  - `is_loaded`, `is_running`: Status flags
  - `created_time`, `last_modified`: Timestamps

#### ProjectArchiveInfo
- **Purpose**: Analysis results of 7z archives
- **Fields**:
  - `archive_path`: Archive file location
  - `is_encrypted`: Whether archive requires password
  - `integrity_verified`: Archive integrity status
  - `contained_files`: List of files in archive
  - `docker_images`: Docker image tar files found
  - `error_message`: Any errors encountered

### 2. Project Lifecycle

#### Phase 1: Archive Analysis
1. **File Existence Check**: Verify archive exists
2. **Integrity Validation**: Test archive with 7z
3. **Content Listing**: Extract file list without extraction
4. **Encryption Detection**: Check if password required
5. **Compose File Detection**: Verify docker-compose.yml exists
6. **Image Detection**: Find .tar/.tar.gz Docker images

#### Phase 2: Project Loading
1. **Archive Extraction**: Extract to project directory
2. **Compose Validation**: Validate docker-compose.yml structure
3. **Image Loading**: Load Docker images from tar files
4. **Dependency Analysis**: Parse compose file for dependencies
5. **Registration**: Register with DockerManager
6. **Status Update**: Mark project as ready

#### Phase 3: Project Management
1. **Service Control**: Start/stop/restart services
2. **Status Monitoring**: Track running state
3. **Log Access**: Retrieve service logs
4. **Resource Cleanup**: Clean removal of projects

### 3. REST API Endpoints

#### Project Analysis
- `POST /api/projects/analyze`
  - **Purpose**: Analyze archive without extraction
  - **Input**: `{"archive_path": "path", "password": "optional"}`
  - **Output**: Archive info with file list and validation status

#### Project Management
- `POST /api/projects/load`
  - **Purpose**: Load project from archive
  - **Input**: `{"archive_path": "path", "project_name": "name", "password": "optional"}`
  
- `GET /api/projects`
  - **Purpose**: List all loaded projects
  
- `GET /api/projects/{name}`
  - **Purpose**: Get detailed project information

#### Project Control
- `POST /api/projects/{name}/start`
  - **Purpose**: Start project services
  
- `POST /api/projects/{name}/stop`
  - **Purpose**: Stop project services
  
- `POST /api/projects/{name}/restart`
  - **Purpose**: Restart project services

#### Project Cleanup
- `POST /api/projects/{name}/unload`
  - **Purpose**: Unload project (keep files)
  
- `DELETE /api/projects/{name}/remove`
  - **Purpose**: Remove project (optionally delete files)
  - **Input**: `{"remove_files": true/false}`

#### Utility Endpoints
- `GET /api/projects/{name}/services`
  - **Purpose**: List project services
  
- `GET /api/projects/{name}/logs?service=name`
  - **Purpose**: Get project/service logs
  
- `GET /api/projects/{name}/status`
  - **Purpose**: Get current project status

#### Archive Creation
- `POST /api/projects/create-archive`
  - **Purpose**: Create encrypted archive from project directory
  - **Input**: `{"project_path": "path", "archive_path": "output", "password": "optional"}`

### 4. WebSocket Integration

#### Log Broadcasting (`/ws/logs`)
- Real-time operation logs
- JSON format with operation, message, level, timestamp
- Shared with DockerManager for unified logging

#### Progress Broadcasting (`/ws/progress`)
- Real-time progress updates during operations
- Includes status, percentage, current operation, errors
- Used for long-running operations like extraction and loading

### 5. Security Features

#### Archive Encryption
- Full 7z encryption support with password protection
- Header encryption (`-mhe=on`) for maximum security
- Integrity validation before extraction

#### Input Validation
- Project name validation (alphanumeric, dash, underscore)
- Path sanitization and validation
- Archive integrity checks

#### Error Handling
- Comprehensive exception handling
- Detailed error reporting through WebSocket
- Graceful cleanup on failures

### 6. File System Management

#### Directory Structure
```
/tmp/metainstaller_projects/
├── project1/
│   ├── docker-compose.yml
│   ├── image1.tar
│   ├── image2.tar
│   └── ... (extracted files)
├── project2/
│   └── ...
└── ...

/tmp/metainstaller_temp/
├── 7zzs (extracted 7z executable)
└── ... (temporary files)
```

#### Cleanup Strategy
- Automatic cleanup on project removal
- Temporary file management
- Docker image cleanup integration

### 7. Integration Points

#### DockerManager Integration
- Leverages existing Docker operations
- Compose project registration
- Image loading and management
- Service lifecycle control

#### ProcessManager Integration
- 7z executable management
- Archive operations (extract, test, list, create)
- Process output capture and logging

#### Utils Integration
- Path manipulation utilities
- Temporary directory creation
- 7z executable extraction

### 8. Error Handling and Logging

#### Comprehensive Logging
- Operation-level logging with context
- Error categorization (info, warning, error)
- WebSocket broadcasting for real-time monitoring

#### Progress Tracking
- Detailed progress reporting for long operations
- Percentage completion tracking
- Current operation status
- Error details with context

### 9. Configuration

#### Default Settings
- Projects directory: `/tmp/metainstaller_projects`
- Temporary directory: `/tmp/metainstaller_temp`
- Configurable through `setProjectsDirectory()`

#### Environment Integration
- Uses existing EnvConfig system
- Inherits Docker configuration
- WebSocket port configuration

### 10. Usage Examples

#### Basic Workflow
```cpp
// Initialize
ProjectManager pm(&dockerManager);
pm.setWebSocketConnections(&logs, &progress);

// Analyze archive
auto info = pm.analyzeArchive("/path/to/project.7z", "password");

// Load project
bool success = pm.loadProject("/path/to/project.7z", "myproject", "password");

// Start services
pm.startProject("myproject");

// Monitor status
auto status = pm.getProjectStatus("myproject");

// Cleanup
pm.removeProject("myproject", true);
```

#### Archive Creation
```cpp
// Create archive from existing project
pm.createProjectArchive("/path/to/project", "/path/to/output.7z", "password");
```

## Implementation Notes

### C++17 Compatibility
- Uses `std::get<>()` instead of structured bindings
- Compatible string operations instead of `ends_with()`
- Proper namespace usage for filesystem operations

### Performance Considerations
- Asynchronous operations with progress callbacks
- Efficient file system operations
- Memory-conscious archive handling

### Extensibility
- Modular design for easy feature addition
- Plugin-ready architecture
- Configurable validation rules

## Testing

The system includes comprehensive testing capabilities:
- Archive integrity testing
- Project loading validation
- Service management verification
- Error condition handling

## Security Considerations

- Password-protected archives
- Input validation and sanitization
- Secure temporary file handling
- Process isolation for 7z operations
