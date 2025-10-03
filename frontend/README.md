# Docker Manager Web Interface

A comprehensive React TypeScript web application for managing Docker containers, images, services, and Docker Compose projects.

## Features

- **Dashboard**: Overview of Docker system status and resources
- **Installation Management**: Install/uninstall Docker with progress monitoring
- **Service Management**: Start, stop, restart, and configure Docker service
- **Container Management**: Full container lifecycle management (start, stop, pause, remove, logs)
- **Image Management**: Pull, build, tag, save, load, and remove Docker images
- **Docker Compose**: Load and manage multi-container applications
- **System Information**: Detailed system status and resource monitoring
- **Real-time Logs**: WebSocket-based live log streaming with filtering
- **Responsive UI**: Modern, lightweight interface built with Tailwind CSS

## Technology Stack

- **Frontend Framework**: React 18 with TypeScript
- **Routing**: React Router DOM v6
- **State Management**: TanStack Query (React Query) for server state
- **HTTP Client**: Axios for REST API communication
- **WebSocket**: Native WebSocket API for real-time log streaming
- **Styling**: Tailwind CSS with custom UI components
- **Build Tool**: Vite for fast development and building
- **Icons**: Lucide React for consistent iconography

## Getting Started

### Prerequisites

- Node.js 16+ and npm/yarn
- Docker REST API server running (typically on port 14040)

### Installation

1. Install dependencies:
```bash
cd frontend
npm install
```

2. Configure environment:
```bash
cp .env .env.local
# Edit .env.local with your API server details
```

3. Start development server:
```bash
npm run dev
```

The application will be available at `http://localhost:3000`

### Building for Production

```bash
npm run build
npm run preview
```

## Configuration

### Development vs Production

The application uses different URL strategies for development and production to avoid CORS issues:

**Development Mode (with Vite proxy):**
- API calls use relative URLs (e.g., `/api/docker/info`)
- Vite proxy forwards requests to the Docker REST server
- WebSocket connections are automatically proxied
- No CORS configuration needed

**Production Mode:**
- Uses absolute URLs from environment variables
- Requires proper CORS configuration on the backend
- Or deploy frontend and backend on the same domain

### Environment Variables

- `VITE_API_BASE_URL`: Base URL for the Docker REST API (leave empty for development proxy)
- `VITE_WS_BASE_URL`: Base URL for WebSocket connections (leave empty for development proxy)

### Proxy Configuration

The Vite development server is configured with proxy rules:
```javascript
proxy: {
  '/api': {
    target: 'http://localhost:14040',
    changeOrigin: true,
    secure: false,
  },
  '/ws': {
    target: 'ws://localhost:14040',
    ws: true,
    changeOrigin: true,
  }
}
```

### API Integration

The application integrates with the Docker Manager REST API and supports all documented endpoints:

- Docker installation and information
- Service management
- Container lifecycle operations
- Image registry operations
- System information and cleanup
- Docker Compose project management
- Real-time log streaming via WebSocket

### WebSocket Log Streaming

The log viewer component connects to the WebSocket server for real-time log streaming:

- Container-specific log streaming
- Search and filtering capabilities
- Configurable log retention
- Export functionality
- Automatic reconnection

## Project Structure

```
frontend/
├── src/
│   ├── components/          # React components
│   │   ├── ui/             # Reusable UI components
│   │   ├── docker/         # Docker-specific components
│   │   ├── containers/     # Container management
│   │   ├── images/         # Image management
│   │   ├── compose/        # Docker Compose
│   │   ├── system/         # System information
│   │   └── logs/           # Log viewer
│   ├── hooks/              # Custom React hooks
│   ├── services/           # API and WebSocket services
│   ├── types/              # TypeScript type definitions
│   └── utils/              # Utility functions
├── public/                 # Static assets
└── ...config files
```

## Available Scripts

- `npm run dev`: Start development server with hot reload
- `npm run build`: Build production bundle
- `npm run preview`: Preview production build locally
- `npm run type-check`: Run TypeScript type checking

## API Documentation

The application consumes the Docker Manager REST API. Refer to the API documentation files:
- `FRONTEND_API_DOCS.md`: Frontend-specific API documentation
- `REST_API_Documentation.md`: Complete API reference

## UI Components

The application uses a lightweight component library built with Tailwind CSS:

- **Button**: Various styles and states
- **Card**: Content containers with headers and footers
- **Table**: Data display with sorting and filtering
- **Input**: Form inputs with validation states
- **Badge**: Status indicators and labels
- **Alert**: Success, error, and warning messages

All components are responsive and follow modern design principles.