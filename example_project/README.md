# Example Docker Project

This is an example project that demonstrates the ProjectManager architecture for managing Docker Compose projects through encrypted 7z archives.

## Project Structure

```
example_project/
├── docker-compose.yml    # Main compose file
├── html/                 # Web content
│   └── index.html       # Example web page
└── README.md            # This file
```

## Services

- **nginx:alpine** - Web server serving static content on port 8080
- **redis:alpine** - Redis cache server on port 6379

## Usage

1. This project should be packaged into an encrypted 7z archive
2. The archive is loaded using the ProjectManager REST API
3. Docker images are automatically loaded from tar files (if present)
4. Services can be started/stopped through the API

## API Examples

### Analyze Archive
```bash
curl -X POST http://localhost:8000/api/projects/analyze \
  -H "Content-Type: application/json" \
  -d '{"archive_path": "/path/to/project.7z", "password": "secret"}'
```

### Load Project
```bash
curl -X POST http://localhost:8000/api/projects/load \
  -H "Content-Type: application/json" \
  -d '{"archive_path": "/path/to/project.7z", "project_name": "example", "password": "secret"}'
```

### Start Project
```bash
curl -X POST http://localhost:8000/api/projects/example/start
```

### Get Project Status
```bash
curl http://localhost:8000/api/projects/example/status
```

### List All Projects
```bash
curl http://localhost:8000/api/projects
