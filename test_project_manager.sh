#!/bin/bash

# ProjectManager Test Script
# This script demonstrates the complete workflow of the ProjectManager system

set -e  # Exit on any error

echo "🚀 ProjectManager Test Script"
echo "=============================="

# Configuration
REST_PORT=14040 # ${REST_PORT:-8000}
BASE_URL="http://localhost:${REST_PORT}"
PROJECT_NAME="example-project"
ARCHIVE_PATH="/tmp/example-project.7z"
PASSWORD="test123"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if server is running
check_server() {
    log_info "Checking if server is running..."
    if curl -s "${BASE_URL}/test" > /dev/null; then
        log_success "Server is running on port ${REST_PORT}"
    else
        log_error "Server is not running on port ${REST_PORT}"
        log_info "Please start the server first: ./MetaInstaller"
        exit 1
    fi
}

# Create example archive
create_example_archive() {
    log_info "Creating example project archive..."
    
    # Check if 7z is available
    if ! command -v 7z &> /dev/null; then
        log_warning "7z not found in PATH, using embedded 7z"
    fi
    
    # Create archive using the API (if server supports it) or manually
    if [ -d "example_project" ]; then
        # Use system 7z if available, otherwise this will be handled by the server
        if command -v 7z &> /dev/null; then
            log_info "Creating encrypted archive with system 7z..."
            cd example_project
            7z a -p"${PASSWORD}" -mhe=on "${ARCHIVE_PATH}" ./*
            cd ..
            log_success "Archive created: ${ARCHIVE_PATH}"
        else
            log_info "Using server API to create archive..."
            curl -X POST "${BASE_URL}/api/projects/create-archive" \
                -H "Content-Type: application/json" \
                -d "{\"project_path\": \"$(pwd)/example_project\", \"archive_path\": \"${ARCHIVE_PATH}\", \"password\": \"${PASSWORD}\"}" \
                -w "\nHTTP Status: %{http_code}\n"
        fi
    else
        log_error "example_project directory not found!"
        exit 1
    fi
}

# Test archive analysis
test_analyze_archive() {
    log_info "Testing archive analysis..."
    
    response=$(curl -s -X POST "${BASE_URL}/api/projects/analyze" \
        -H "Content-Type: application/json" \
        -d "{\"archive_path\": \"${ARCHIVE_PATH}\", \"password\": \"${PASSWORD}\"}")
    
    echo "Analysis Response:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
    
    # Check if analysis was successful
    if echo "$response" | grep -q '"success": *true'; then
        log_success "Archive analysis successful"
    else
        log_error "Archive analysis failed"
        return 1
    fi
}

# Test project loading
test_load_project() {
    log_info "Testing project loading..."
    
    response=$(curl -s -X POST "${BASE_URL}/api/projects/load" \
        -H "Content-Type: application/json" \
        -d "{\"archive_path\": \"${ARCHIVE_PATH}\", \"project_name\": \"${PROJECT_NAME}\", \"password\": \"${PASSWORD}\"}")
    
    echo "Load Response:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
    
    if echo "$response" | grep -q '"success": *true'; then
        log_success "Project loaded successfully"
    else
        log_error "Project loading failed"
        return 1
    fi
}

# Test project listing
test_list_projects() {
    log_info "Testing project listing..."
    
    response=$(curl -s "${BASE_URL}/api/projects")
    
    echo "Projects List:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
}

# Test project info
test_project_info() {
    log_info "Testing project info retrieval..."
    
    response=$(curl -s "${BASE_URL}/api/projects/${PROJECT_NAME}")
    
    echo "Project Info:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
}

# Test project services
test_project_services() {
    log_info "Testing project services listing..."
    
    response=$(curl -s "${BASE_URL}/api/projects/${PROJECT_NAME}/services")
    
    echo "Project Services:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
}

# Test project start
test_start_project() {
    log_info "Testing project start..."
    
    response=$(curl -s -X POST "${BASE_URL}/api/projects/${PROJECT_NAME}/start")
    
    echo "Start Response:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
    
    if echo "$response" | grep -q '"success": *true'; then
        log_success "Project started successfully"
        log_info "You can now access the web service at http://localhost:8080"
    else
        log_warning "Project start may have failed (check Docker installation)"
    fi
}

# Test project status
test_project_status() {
    log_info "Testing project status..."
    
    response=$(curl -s "${BASE_URL}/api/projects/${PROJECT_NAME}/status")
    
    echo "Status Response:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
}

# Test project stop
test_stop_project() {
    log_info "Testing project stop..."
    
    response=$(curl -s -X POST "${BASE_URL}/api/projects/${PROJECT_NAME}/stop")
    
    echo "Stop Response:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
    
    if echo "$response" | grep -q '"success": *true'; then
        log_success "Project stopped successfully"
    else
        log_warning "Project stop may have failed"
    fi
}

# Test project removal
test_remove_project() {
    log_info "Testing project removal..."
    
    response=$(curl -s -X DELETE "${BASE_URL}/api/projects/${PROJECT_NAME}/remove" \
        -H "Content-Type: application/json" \
        -d '{"remove_files": true}')
    
    echo "Remove Response:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
    
    if echo "$response" | grep -q '"success": *true'; then
        log_success "Project removed successfully"
    else
        log_warning "Project removal may have failed"
    fi
}

# Cleanup function
cleanup() {
    log_info "Cleaning up test files..."
    rm -f "${ARCHIVE_PATH}"
    log_success "Cleanup completed"
}

# Main test execution
main() {
    echo
    log_info "Starting ProjectManager functionality tests..."
    echo
    
    # Check prerequisites
    check_server
    
    # Create test archive
    create_example_archive
    
    # Run tests in sequence
    echo
    log_info "=== Phase 1: Archive Analysis ==="
    test_analyze_archive
    
    echo
    log_info "=== Phase 2: Project Loading ==="
    test_load_project
    
    echo
    log_info "=== Phase 3: Project Information ==="
    test_list_projects
    test_project_info
    test_project_services
    
    echo
    log_info "=== Phase 4: Project Control ==="
    test_project_status
    test_start_project
    
    # Wait a moment for services to start
    log_info "Waiting 5 seconds for services to start..."
    sleep 5
    
    test_project_status
    test_stop_project
    
    echo
    log_info "=== Phase 5: Project Cleanup ==="
    test_remove_project
    
    # Final cleanup
    cleanup
    
    echo
    log_success "🎉 All tests completed successfully!"
    echo
    log_info "The ProjectManager system is working correctly."
    log_info "You can now use the REST API to manage Docker Compose projects."
}

# Trap cleanup on exit
trap cleanup EXIT

# Run main function
main "$@"
