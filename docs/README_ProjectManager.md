# ProjectManager REST API Documentation

## Overview

The ProjectManager component handles project lifecycle management for containerized applications. It provides functionality to load, unload, start, stop, and manage projects that are packaged as 7z archives containing docker-compose.yml files and Docker images.

## REST API Endpoints

### 1. Analyze Archive

Analyzes a 7z archive to check its contents and validity.

- **Method**: `POST`
- **Endpoint**: `/api/projects/analyze`
- **Description**: Analyzes a project archive to verify its integrity and identify contained files and Docker images.

#### Request Body
```json
{
  "archive_path": "string",  // Path to the 7z archive file
  "password": "string"       // Password for encrypted archives (optional)
}
```

#### Response Format
```json
{
  "success": boolean,
  "archive_path": "string",
  "is_encrypted": boolean,
  "integrity_verified": boolean,
  "contained_files": ["string"],
  "docker_images": ["string"],
  "error_message": "string"
}
```

#### Example Curl Command
```bash
curl -X POST http://localhost:14040/api/projects/analyze \
  -H "Content-Type: application/json" \
  -d '{
    "archive_path": "/path/to/project.7z",
    "password": "archive_password"
  }'
```

### 2. Load Project

Loads a project from a 7z archive.

- **Method**: `POST`
- **Endpoint**: `/api/projects/load`
- **Description**: Extracts and loads a project from an archive, including Docker images and compose files.

#### Request Body
```json
{
  "archive_path": "string",  // Path to the 7z archive file
  "project_name": "string",  // Name for the project (must be unique)
  "password": "string"       // Password for encrypted archives (optional)
}
```

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string"
}
```

#### Example Curl Command
```bash
curl -X POST http://localhost:14040/api/projects/load \
  -H "Content-Type: application/json" \
  -d '{
    "archive_path": "/path/to/project.7z",
    "project_name": "my_project",
    "password": "archive_password"
  }'
```

### 3. Unload Project

Unloads a project from memory.

- **Method**: `POST`
- **Endpoint**: `/api/projects/{projectName}/unload`
- **Description**: Removes a project from the ProjectManager without deleting files.

#### Path Parameters
- `projectName`: Name of the project to unload

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string"
}
```

#### Example Curl Command
```bash
curl -X POST http://localhost:14040/api/projects/my_project/unload
```

### 4. Remove Project

Removes a project completely.

- **Method**: `DELETE`
- **Endpoint**: `/api/projects/{projectName}/remove`
- **Description**: Removes a project and optionally deletes its files.

#### Path Parameters
- `projectName`: Name of the project to remove

#### Request Body
```json
{
  "remove_files": boolean  // Whether to delete project files from disk
}
```

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string"
}
```

#### Example Curl Command
```bash
curl -X DELETE http://localhost:14040/api/projects/my_project/remove \
  -H "Content-Type: application/json" \
  -d '{
    "remove_files": true
  }'
```

### 5. Start Project

Starts a loaded project using Docker Compose.

- **Method**: `POST`
- **Endpoint**: `/api/projects/{projectName}/start`
- **Description**: Starts all services defined in the project's docker-compose.yml file.

#### Path Parameters
- `projectName`: Name of the project to start

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string"
}
```

#### Example Curl Command
```bash
curl -X POST http://localhost:14040/api/projects/my_project/start
```

### 6. Stop Project

Stops a running project.

- **Method**: `POST`
- **Endpoint**: `/api/projects/{projectName}/stop`
- **Description**: Stops all services for a running project.

#### Path Parameters
- `projectName`: Name of the project to stop

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string"
}
```

#### Example Curl Command
```bash
curl -X POST http://localhost:14040/api/projects/my_project/stop
```

### 7. Restart Project

Restarts a project.

- **Method**: `POST`
- **Endpoint**: `/api/projects/{projectName}/restart`
- **Description**: Restarts all services for a project.

#### Path Parameters
- `projectName`: Name of the project to restart

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string"
}
```

#### Example Curl Command
```bash
curl -X POST http://localhost:14040/api/projects/my_project/restart
```

### 8. List Projects

Lists all loaded projects.

- **Method**: `GET`
- **Endpoint**: `/api/projects`
- **Description**: Returns information about all projects currently loaded in the ProjectManager.

#### Response Format
```json
{
  "success": boolean,
  "projects": [
    {
      "name": "string",
      "archive_path": "string",
      "extracted_path": "string",
      "compose_file_path": "string",
      "is_loaded": boolean,
      "status_message": "string",
      "created_time": "string",
      "last_modified": "string",
      "required_images": ["string"],
      "dependent_files": ["string"]
    }
  ]
}
```

#### Example Curl Command
```bash
curl -X GET http://localhost:14040/api/projects
```

### 9. Get Project Info

Gets detailed information about a specific project.

- **Method**: `GET`
- **Endpoint**: `/api/projects/{projectName}`
- **Description**: Returns detailed information about a specific project.

#### Path Parameters
- `projectName`: Name of the project to query

#### Response Format
```json
{
  "success": boolean,
  "project": {
    "name": "string",
    "archive_path": "string",
    "extracted_path": "string",
    "compose_file_path": "string",
    "is_loaded": boolean,
    "status_message": "string",
    "created_time": "string",
    "last_modified": "string",
    "required_images": ["string"],
    "dependent_files": ["string"]
  }
}
```

#### Example Curl Command
```bash
curl -X GET http://localhost:14040/api/projects/my_project
```

### 10. Get Project Services

Lists services defined in a project's docker-compose.yml file.

- **Method**: `GET`
- **Endpoint**: `/api/projects/{projectName}/services`
- **Description**: Returns a list of services defined in the project's compose file.

#### Path Parameters
- `projectName`: Name of the project

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string",
  "services": ["string"]
}
```

#### Example Curl Command
```bash
curl -X GET http://localhost:14040/api/projects/my_project/services
```

### 11. Get Project Logs

Retrieves logs for a project or specific service.

- **Method**: `GET`
- **Endpoint**: `/api/projects/{projectName}/logs`
- **Description**: Returns logs for a project or a specific service within the project.

#### Path Parameters
- `projectName`: Name of the project

#### Query Parameters
- `service`: Name of the specific service to get logs for (optional)

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string",
  "service_name": "string",
  "logs": "string"
}
```

#### Example Curl Command
```bash
curl -X GET "http://localhost:14040/api/projects/my_project/logs?service=web"
```

### 12. Create Project Archive

Creates a 7z archive from a project directory.

- **Method**: `POST`
- **Endpoint**: `/api/projects/create-archive`
- **Description**: Creates a 7z archive from a project directory, optionally with password protection.

#### Request Body
```json
{
  "project_path": "string",  // Path to the project directory
  "archive_path": "string",  // Path where the archive should be created
  "password": "string"       // Password to encrypt the archive (optional)
}
```

#### Response Format
```json
{
  "success": boolean,
  "archive_path": "string"
}
```

#### Example Curl Command
```bash
curl -X POST http://localhost:14040/api/projects/create-archive \
  -H "Content-Type: application/json" \
  -d '{
    "project_path": "/path/to/project",
    "archive_path": "/path/to/project.7z",
    "password": "archive_password"
  }'
```

### 13. Get Project Status

Gets the current status of a project.

- **Method**: `GET`
- **Endpoint**: `/api/projects/{projectName}/status`
- **Description**: Returns the current status of a project.

#### Path Parameters
- `projectName`: Name of the project

#### Response Format
```json
{
  "success": boolean,
  "project_name": "string",
  "status": "string"  // Possible values: not_loaded, extracting, loading_images, validating, ready, running, stopped, error
}
```

#### Example Curl Command
```bash
curl -X GET http://localhost:14040/api/projects/my_project/status
```

## Project Status Values

The ProjectManager uses the following status values to indicate the state of a project:

- `not_loaded`: Project is not loaded
- `extracting`: Project archive is being extracted
- `loading_images`: Docker images are being loaded
- `validating`: Project is being validated
- `ready`: Project is loaded and ready to start
- `running`: Project is currently running
- `stopped`: Project is stopped
- `error`: An error occurred with the project

## Error Handling

All endpoints return appropriate HTTP status codes:
- `200`: Success
- `400`: Bad request (invalid input)
- `404`: Project not found
- `500`: Internal server error

Error responses follow this format:
```json
{
  "success": false,
  "error": "Error message"
}
