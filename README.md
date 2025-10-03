# MetaInstaller

MetaInstaller is a comprehensive container management system that enables secure packaging, distribution, and deployment of Docker Compose applications as encrypted 7z archives. It provides a complete REST API and web interface for managing containerized projects from archive analysis through deployment and lifecycle management.

## Features

- **Encrypted Project Archives**: Package Docker Compose projects as password-protected 7z archives with full header encryption
- **Complete Project Lifecycle Management**: Load, unload, start, stop, restart, and remove projects
- **Archive Analysis**: Verify archive integrity and inspect contents without extraction
- **Docker Integration**: Seamless Docker Compose integration with image loading and service management
- **Real-time Monitoring**: WebSocket-based log streaming and progress updates
- **File Browser**: Web-based file explorer with detailed file information and content viewing
- **Cross-platform**: Static C++ binary with embedded dependencies for easy deployment
- **SELinux Support**: Built-in SELinux management for enhanced security

## Architecture

MetaInstaller consists of two main components:

### Backend (C++/Crow)
- **ProjectManager**: Handles project lifecycle management and archive operations
- **DockerManager**: Manages Docker operations and Compose integration  
- **FileManager**: Provides file system access and web-based file browsing
- **BrowserManager**: Launches embedded browser for web interface
- **SELinuxManager**: Manages SELinux contexts and policies

### Frontend (React/TypeScript)
- React-based web interface for project management
- Real-time WebSocket integration for logs and progress
- File picker component with detailed file information
- Responsive design with modern UI components

## Installation

### Prerequisites
- Docker and Docker Compose installed
- Linux system (tested on distributions with systemd)
- Git (for cloning the repository)
- Network access to registry.metaprocess.ir (for base images)

### Build Process Overview
MetaInstaller uses a multi-stage Docker build process to create a fully static binary:

1. **Web Frontend Build**: Builds the React frontend in a Node.js container and packages it as a 7z archive
2. **C++ Backend Build**: Compiles the C++ backend in an Alpine Linux container with full static linking
3. **Final Package**: Combines both components into a single executable 7z archive

### Build from Source
```bash
# Clone the repository
git clone ssh://mojtabagit@127.0.0.1:22242/container_installer_rest
cd container_installer_rest

# Build the complete MetaInstaller package (recommended)
make

# Alternative: Build individual components
make web        # Build only the web frontend
make metainstaller  # Build only the C++ backend
make baseimage  # Build base Docker images (requires authentication)
```

### Build Configuration
The build process uses environment files for configuration:
- `.env.baseimage`: Contains Docker registry, image names, and tags
- `.env.version`: Contains version information (VERSION=1404.06.18)

### Quick Start
```bash
# Build the complete MetaInstaller package
make

# This creates:
# - MetaInstaller-${VERSION}: Static C++ binary with embedded web interface
# - MetaInstaller-${VERSION}.7z: Compressed package containing the binary

# Run with help
./MetaInstaller-${VERSION} --help

# Check version
./MetaInstaller-${VERSION} --version

# Run tests
./MetaInstaller-${VERSION} --test=all
```

### Docker Build Details
The build process uses two Docker Compose files:

- **docker-compose.yml**: Main build orchestration
  - `web_build` service: Builds React frontend using Node.js 18 container
  - `builder` service: Compiles C++ backend using Alpine 3.22 container with static linking

- **docker-compose-create-baseimage.yml**: Base image creation (for maintainers)
  - Builds custom Alpine and Node.js base images
  - Requires authentication to push to registry.metaprocess.ir

The C++ backend is compiled with:
- C++17 standard
- Full static linking (`-static -static-libstdc++ -static-libgcc`)
- Release build type
- Embedded web resources and 7z executable

## Usage

### REST API Endpoints

#### Project Management
- `POST /api/projects/analyze` - Analyze project archive
- `POST /api/projects/load` - Load project from archive
- `GET /api/projects` - List all loaded projects
- `GET /api/projects/{name}` - Get project details
- `POST /api/projects/{name}/start` - Start project
- `POST /api/projects/{name}/stop` - Stop project
- `POST /api/projects/{name}/restart` - Restart project
- `POST /api/projects/{name}/unload` - Unload project
- `DELETE /api/projects/{name}/remove` - Remove project

#### File Management
- `GET /api/file/list/{path}` - List directory contents (simple)
- `GET /api/file/list-detailed/{path}` - List directory contents (detailed)
- `GET /api/file/get/{path}` - Get file content

#### Docker Management
- `GET /api/docker/info` - Get Docker information
- `GET /api/docker-compose/info` - Get Docker Compose information
- `GET /api/docker/service/status` - Get Docker service status
- `GET /api/docker/containers?all=true/false` - List containers (all or running only)
- `GET /api/docker/containers/detailed?all=true/false` - Get detailed container information
- `GET /api/docker/images` - List Docker images
- `GET /api/docker/system/info` - Get Docker system information
- `GET /api/docker/system/df` - Get Docker disk usage
- `GET /api/docker/compose/projects` - List Docker Compose projects
- `GET /api/docker/system/integration-status` - Get system integration status
- `GET /api/docker/installation/progress` - Get Docker installation progress
- `POST /api/docker/install` - Install Docker
- `POST /api/docker/service/start|stop|restart|enable|disable` - Manage Docker service
- `POST /api/docker/system/prune` - Cleanup Docker system
- `POST /api/docker/images/pull` - Pull Docker image
- `POST /api/test/sudo` - Test and set sudo password
- `DELETE /api/docker/uninstall` - Uninstall Docker

#### System Information
- `GET /api/version` - Get MetaInstaller version
- `GET /api/docs` - Get comprehensive API documentation (HTML)
- `GET /test` - Health check endpoint

### WebSocket Endpoints
- `/ws/logs` - Real-time operation logs
- `/ws/progress` - Installation and operation progress updates

### Example Workflow

1. **Analyze an archive**:
   ```bash
   curl -X POST http://localhost:14040/api/projects/analyze \
     -H "Content-Type: application/json" \
     -d '{"archive_path": "/path/to/project.7z", "password": "secret"}'
   ```

2. **Load the project**:
   ```bash
   curl -X POST http://localhost:14040/api/projects/load \
     -H "Content-Type: application/json" \
     -d '{"archive_path": "/path/to/project.7z", "project_name": "myapp", "password": "secret"}'
   ```

3. **Start the project**:
   ```bash
   curl -X POST http://localhost:14040/api/projects/myapp/start
   ```

4. **Monitor logs via WebSocket**:
   ```javascript
   const ws = new WebSocket('ws://localhost:14040/ws/logs');
   ws.onmessage = (event) => {
     const log = JSON.parse(event.data);
     console.log(`${log.timestamp} [${log.level}] ${log.message}`);
   };
   ```

5. **Manage Docker (if needed)**:
   ```bash
   # Install Docker if not present
   curl -X POST http://localhost:14040/api/docker/install
   
   # Check Docker status
   curl http://localhost:14040/api/docker/service/status
   
   # Pull additional images
   curl -X POST http://localhost:14040/api/docker/images/pull \
     -H "Content-Type: application/json" \
     -d '{"image": "nginx:alpine"}'
   ```

## Project Structure

```
container_installer_rest/
├── src/                    # C++ source code
├── frontend/               # React web interface
│   ├── Dockerfile          # Node.js 18 build environment
│   └── package.json        # Frontend dependencies
├── docs/                   # Documentation
├── resources/              # Embedded resources (7z executable, HTML files)
├── asio/                   # ASIO library dependency
├── Crow/                   # Crow web framework dependency
├── sqlite/                 # SQLite database dependency
├── example_project/        # Example project template
│   ├── docker-compose.yml  # Example compose file with nginx + redis
│   ├── html/               # Web content for nginx
│   └── README.md           # Example project documentation
├── CMakeLists.txt          # C++ build configuration
├── Makefile                # Build automation
├── Dockerfile              # Alpine 3.22 C++ build environment
├── docker-compose.yml      # Main build orchestration
├── docker-compose-create-baseimage.yml  # Base image creation
├── create_baseimage.sh     # Base image build and push script
├── make_project.sh         # Run example project
├── document_rest_apis.sh   # Generate API documentation
├── push_version_tag.sh     # Push version git tag
├── .env                    # Runtime configuration (REST_PORT, LOG_LEVEL, SUDO_PASSWORD)
├── .env.baseimage          # Docker build configuration
├── .env.version            # Version configuration
├── test_*.sh               # Test scripts
└── README.md               # This file
```

## Configuration

MetaInstaller uses environment files for configuration:

### Runtime Configuration
- `.env.version` - Version information (currently VERSION=1404.06.18)
- `.env` - Runtime settings (automatically generated)
  - `REST_PORT=14040` - REST API server port
  - `LOG_LEVEL=INFO` - Logging level (DEBUG, INFO, WARN, ERROR)
  - `SUDO_PASSWORD` - Password for sudo operations (set via API)

### Build Configuration  
- `.env.baseimage` - Docker registry, image names, and tags for build process
  - `REGISTRY_REPO_URL`: Docker registry URL (registry.metaprocess.ir)
  - `IMAGE_OS`: Base OS image name (alpine:3.22 based)
  - `IMAGE_TAG`: Base OS image tag
  - `IMAGE_OS_WEB`: Web build image name (node:18.20.0-alpine based)
  - `IMAGE_TAG_WEB`: Web build image tag

## Security

- **Archive Encryption**: Full 7z encryption with password protection
- **Input Validation**: Comprehensive path and input validation
- **Path Traversal Protection**: Prevents directory traversal attacks
- **SELinux Integration**: Optional SELinux context management
- **Secure Temporary Files**: Isolated temporary directory handling

## Testing

Run comprehensive tests with:
```bash
# Run all tests
./MetaInstaller-${VERSION} --test=all

# Run specific tests
./MetaInstaller-${VERSION} --test=project_manager,file_manager

# Test scripts available
./test_project_manager.sh
./test_filemanager.sh
./test_file_browsing.sh
```

### Example Project
The `example_project/` directory contains a ready-to-use example with:
- **nginx:alpine** web server on port 9999
- **redis:alpine** cache server on port 6379
- Static HTML content served by nginx
- Pre-built Docker images (redis.tar, web.tar)

To test with the example project:
```bash
# Package the example project (requires 7z)
7z a -psecret example_project.7z example_project/

# Analyze and load using MetaInstaller API
curl -X POST http://localhost:14040/api/projects/analyze \
  -H "Content-Type: application/json" \
  -d '{"archive_path": "/path/to/example_project.7z", "password": "secret"}'
```

### Utility Scripts
- `make_project.sh` - Runs docker-compose for the example project
- `document_rest_apis.sh` - Generates comprehensive API documentation
- `push_version_tag.sh` - Creates and pushes git version tags

## Documentation

Detailed documentation is available in the `docs/` directory:
- `README_ProjectManager.md` - Project Manager REST API documentation
- `PROJECT_MANAGER_ARCHITECTURE.md` - Architecture overview
- `FILE_BROWSER.md` - File browser functionality

## Version

Current version: **1404.06.18**

## License

This project uses multiple open-source components with their respective licenses:
- Crow: MIT License
- ASIO: Boost Software License
- SQLite: Public Domain
- React: MIT License

See individual component directories for complete license information.

---

MetaInstaller provides a secure, comprehensive solution for distributing and managing containerized applications as encrypted archives, making it ideal for enterprise deployments, secure software distribution, and controlled environment management.
