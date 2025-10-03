# MetaInstaller

MetaInstaller is a comprehensive container management system that enables secure packaging, distribution, and deployment of Docker Compose applications as encrypted 7z archives. It provides a complete REST API and web interface for managing containerized projects from archive analysis through deployment and lifecycle management.

## Features

- **Encrypted Project Archives**: Package Docker Compose projects as password-protected 7z archives with full header encryption
- **Complete Project Lifecycle Management**: Load, unload, start, stop, restart, and remove projects
- **Archive Analysis**: Verify archive integrity and inspect contents without extraction
- **Docker Integration**: Seamless Docker Compose integration with image loading and service management
- **Real-time Monitoring**: WebSocket-based log streaming and progress updates
- **File Browser**: Web-based file explorer with detailed file information and content viewing
- **Embedded Web Browser**: Lightweight Midori browser with custom profile launched automatically
- **X11/Wayland Detection**: Comprehensive display environment detection before browser launch
- **Lightweight Resource Embedding**: Efficient binary resource embedding using CMake objcopy (vs Qt's memory-heavy approach)
- **Cross-platform**: Static C++ binary with embedded dependencies for easy deployment
- **SELinux Support**: Built-in SELinux management for enhanced security

## Architecture

MetaInstaller consists of two main components:

### Backend (C++/Crow)
- **ProjectManager**: Handles project lifecycle management and archive operations
- **DockerManager**: Manages Docker operations and Compose integration  
- **FileManager**: Provides file system access and web-based file browsing
- **BrowserManager**: Launches embedded Midori browser with custom profile for web interface
- **SELinuxManager**: Manages SELinux contexts and policies
- **X Display Detector**: Comprehensive X11/Wayland environment detection before launching browser
- **Resource Embedder**: Lightweight binary resource embedding using CMake objcopy (vs Qt's memory-heavy approach)

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

### Building Base Docker Images
Before building MetaInstaller from source, you may need to build the required base Docker images. These images provide the build environments for both the web frontend and C++ backend components.

The base images are defined in `docker-compose-create-baseimage.yml` and configured through `.env.baseimage`:

- **C++ Builder Image**: `metainstaller-builder:1.0.0` - Based on Alpine Linux 3.22 with all C++ build dependencies
- **Web Builder Image**: `metainstaller-baseimage-web:1.0.0` - Based on Node.js 18 with web build tools

To build these base images locally:

```bash
# Build base Docker images
make baseimage
```

This command:
1. Sources the `.env.baseimage` configuration file
2. Uses Docker Compose with `docker-compose-create-baseimage.yml` to build two services:
   - `base_os`: Builds the C++ backend build environment from the root `Dockerfile`
   - `web_build`: Builds the web frontend build environment from `frontend/Dockerfile`
3. Tags the resulting images according to the configuration in `.env.baseimage`

The base images are typically only needed when:
- Setting up a new development environment
- Modifying the build environment requirements
- Working offline without access to the pre-built images on the registry

Note: The build process is configured to use local images by default, so building base images is not required for regular development unless you've made changes to the Dockerfiles or need to update dependencies.

### Build from Source
```bash
# Clone the repository
git clone https://github.com/metaprocess/metainstaller.git
cd metainstaller

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

### Embedded Browser Architecture
MetaInstaller uses a lightweight, efficient approach to embed the web interface directly into the C++ binary:

**Resource Embedding Mechanism:**
- Uses CMake's `objcopy` utility to embed binary resources directly into the executable
- Resources are stored as ELF sections in the binary, avoiding Qt's memory-heavy resource system
- Embedded resources include: Midori browser (11.5.2), React web frontend, custom browser profile, and documentation
- The `cmake/EmbedResources.cmake` script automatically converts resource files to object files and links them into the final binary

**Browser Launch Process:**
1. **X Environment Detection**: Before launching the browser, MetaInstaller performs comprehensive X11/Wayland detection:
   - Checks `DISPLAY` and `WAYLAND_DISPLAY` environment variables
   - Scans for X11 socket files and lock files in `/tmp`
   - Detects running X11/Wayland processes
   - Performs socket connectivity tests to verify display availability
   - Falls back gracefully if no display environment is available

2. **Resource Extraction**: At runtime, embedded resources are extracted to `/tmp/metainstaller_browser/`:
   - `midori-11.5.2.7z` - Lightweight Midori web browser
   - `web.7z` - React frontend application
   - `profile_midori.7z` - Custom browser profile with network offline status disabled
   - `comprehensive_docs.html` - Complete API documentation
   - `404_fantasy.html` - Custom 404 error page

3. **Browser Launch**: Midori is launched with the custom profile pointing to the local web interface:
   ```bash
   /tmp/metainstaller_browser/midori/midori \
     --profile /tmp/metainstaller_browser/profile_midori \
     http://127.0.0.1:14040
   ```

**Advantages over Qt:**
- **Memory Efficiency**: No Qt runtime overhead, smaller binary size
- **Faster Compilation**: Avoids Qt's complex build system and MOC processing
- **Static Linking**: Fully static binary with all dependencies embedded
- **Lightweight**: Midori browser uses significantly less memory than Qt WebEngine

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
- `GET /api/projects/{name}/services` - Get project services
- `GET /api/projects/{name}/logs` - Get project logs
- `GET /api/projects/{name}/status` - Get project status
- `POST /api/projects/{name}/start` - Start project
- `POST /api/projects/{name}/stop` - Stop project
- `POST /api/projects/{name}/restart` - Restart project
- `POST /api/projects/{name}/unload` - Unload project
- `DELETE /api/projects/{name}/remove` - Remove project
- `POST /api/projects/create-archive` - Create project archive
- `POST /api/settings/browsing-directory` - Save browsing directory
- `GET /api/settings/browsing-directory` - Get browsing directory

#### File Management
- `GET /api/file/list/{path}` - List directory contents (simple)
- `POST /api/file/list-detailed` - List directory contents (detailed)
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
- `POST /api/docker/service/start` - Start Docker service
- `POST /api/docker/service/stop` - Stop Docker service
- `POST /api/docker/service/restart` - Restart Docker service
- `POST /api/docker/service/enable` - Enable Docker service
- `POST /api/docker/service/disable` - Disable Docker service
- `POST /api/docker/system/prune` - Cleanup Docker system
- `POST /api/docker/images/pull` - Pull Docker image
- `POST /api/docker/images/load` - Load Docker image
- `POST /api/docker/images/save` - Save Docker image
- `POST /api/docker/images/tag` - Tag Docker image
- `POST /api/docker/images/build` - Build Docker image
- `DELETE /api/docker/containers/{containerId}` - Delete container
- `DELETE /api/docker/images/{imageId}` - Delete image
- `POST /api/docker/containers/{containerId}/start` - Start container
- `POST /api/docker/containers/{containerId}/stop` - Stop container
- `POST /api/docker/containers/{containerId}/restart` - Restart container
- `POST /api/docker/containers/{containerId}/pause` - Pause container
- `POST /api/docker/containers/{containerId}/unpause` - Unpause container
- `GET /api/docker/containers/{containerId}/logs` - Get container logs
- `POST /api/docker/compose/projects` - Create Docker Compose project
- `DELETE /api/docker/compose/projects/{projectName}` - Delete Docker Compose project
- `POST /api/docker/compose/projects/{projectName}/up` - Start Docker Compose project
- `POST /api/docker/compose/projects/{projectName}/down` - Stop Docker Compose project
- `POST /api/docker/compose/projects/{projectName}/restart` - Restart Docker Compose project
- `GET /api/docker/compose/projects/{projectName}/services` - Get Docker Compose project services
- `POST /api/docker/compose/projects/{projectName}/services/{serviceName}/{action}` - Manage Docker Compose service
- `POST /api/test/sudo` - Test and set sudo password
- `DELETE /api/docker/uninstall` - Uninstall Docker

#### System Information
- `GET /api/version` - Get MetaInstaller version
- `GET /api/docs` - Get comprehensive API documentation (HTML)
- `GET /test` - Health check endpoint

#### SELinux Management
- `GET /api/selinux/info` - Get SELinux information
- `GET /api/selinux/status` - Get SELinux status
- `POST /api/selinux/disable` - Disable SELinux
- `POST /api/selinux/enable` - Enable SELinux
- `POST /api/selinux/enforcing` - Set SELinux to enforcing mode
- `POST /api/selinux/permissive` - Set SELinux to permissive mode
- `POST /api/selinux/restore-context` - Restore SELinux context
- `POST /api/selinux/apply-context` - Apply SELinux context

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
