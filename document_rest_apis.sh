#!/bin/bash

# Document REST API outputs
OUTPUT_FILE="REST_API_Responses.md"
BASE_URL="http://127.0.0.1:14040"

# Create or clear the output file
echo "# Docker Manager REST API Responses" > $OUTPUT_FILE
echo "" >> $OUTPUT_FILE

# Function to document an endpoint
document_endpoint() {
    local endpoint=$1
    local description=$2
    
    echo "## $description" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "**Endpoint:** \`$endpoint\`" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "**Response:**" >> $OUTPUT_FILE
    
    # Special handling for /api/docs which returns HTML
    if [ "$endpoint" = "/api/docs" ]; then
        echo '```html' >> $OUTPUT_FILE
        curl -s --max-time 30 "$BASE_URL$endpoint" >> $OUTPUT_FILE
        echo '```' >> $OUTPUT_FILE
    else
        echo '```json' >> $OUTPUT_FILE
        curl -s --max-time 30 "$BASE_URL$endpoint" | jq '.' >> $OUTPUT_FILE 2>/dev/null || curl -s --max-time 30 "$BASE_URL$endpoint" >> $OUTPUT_FILE
        echo '```' >> $OUTPUT_FILE
    fi
    
    echo "" >> $OUTPUT_FILE
    echo "---" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
}

# Function to document a GET endpoint
document_get_endpoint() {
    local endpoint=$1
    local description=$2
    
    echo "## $description" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "**Endpoint:** \`GET $endpoint\`" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "**Response:**" >> $OUTPUT_FILE
    
    # Special handling for /api/docs which returns HTML
    if [ "$endpoint" = "/api/docs" ]; then
        echo '```html' >> $OUTPUT_FILE
        curl -s --max-time 30 "$BASE_URL$endpoint" >> $OUTPUT_FILE
        echo '```' >> $OUTPUT_FILE
    else
        echo '```json' >> $OUTPUT_FILE
        curl -s --max-time 30 "$BASE_URL$endpoint" | jq '.' >> $OUTPUT_FILE 2>/dev/null || curl -s --max-time 30 "$BASE_URL$endpoint" >> $OUTPUT_FILE
        echo '```' >> $OUTPUT_FILE
    fi
    
    echo "" >> $OUTPUT_FILE
    echo "---" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
}

# Function to document a POST endpoint
document_post_endpoint() {
    local endpoint=$1
    local description=$2
    local data=$3
    
    echo "## $description" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "**Endpoint:** \`POST $endpoint\`" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    if [ -n "$data" ]; then
        echo "**Request Body:**" >> $OUTPUT_FILE
        echo '```json' >> $OUTPUT_FILE
        echo "$data" >> $OUTPUT_FILE
        echo '```' >> $OUTPUT_FILE
        echo "" >> $OUTPUT_FILE
    fi
    echo "**Response:**" >> $OUTPUT_FILE
    echo '```json' >> $OUTPUT_FILE
    if [ -n "$data" ]; then
        curl -s --max-time 30 -X POST -H "Content-Type: application/json" -d "$data" "$BASE_URL$endpoint" | jq '.' >> $OUTPUT_FILE 2>/dev/null || curl -s --max-time 30 -X POST -H "Content-Type: application/json" -d "$data" "$BASE_URL$endpoint" >> $OUTPUT_FILE
    else
        curl -s --max-time 30 -X POST "$BASE_URL$endpoint" | jq '.' >> $OUTPUT_FILE 2>/dev/null || curl -s --max-time 30 -X POST "$BASE_URL$endpoint" >> $OUTPUT_FILE
    fi
    echo '```' >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "---" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
}

# Function to document a DELETE endpoint
document_delete_endpoint() {
    local endpoint=$1
    local description=$2
    
    echo "## $description" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "**Endpoint:** \`DELETE $endpoint\`" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "**Response:**" >> $OUTPUT_FILE
    echo '```json' >> $OUTPUT_FILE
    curl -s --max-time 30 -X DELETE "$BASE_URL$endpoint" | jq '.' >> $OUTPUT_FILE 2>/dev/null || curl -s --max-time 30 -X DELETE "$BASE_URL$endpoint" >> $OUTPUT_FILE
    echo '```' >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
    echo "---" >> $OUTPUT_FILE
    echo "" >> $OUTPUT_FILE
}

# Document all GET endpoints
document_get_endpoint "/api/docker/info" "Get Docker Information"
document_get_endpoint "/api/docker-compose/info" "Get Docker Compose Information"
document_get_endpoint "/api/docker/service/status" "Get Docker Service Status"
document_get_endpoint "/api/docker/containers?all=true" "List All Containers (Including Stopped)"
document_get_endpoint "/api/docker/containers?all=false" "List Running Containers Only"
document_get_endpoint "/api/docker/images" "List Docker Images"
document_get_endpoint "/api/docker/system/info" "Get Docker System Information"
document_get_endpoint "/api/docker/system/df" "Get Docker Disk Usage"
document_get_endpoint "/api/docker/installation/progress" "Get Installation Progress"
document_get_endpoint "/api/docker/containers/detailed?all=true" "Get Detailed Container Information (All)"
document_get_endpoint "/api/docker/containers/detailed?all=false" "Get Detailed Container Information (Running Only)"
document_get_endpoint "/api/docker/compose/projects" "List Docker Compose Projects"
document_get_endpoint "/api/docker/system/integration-status" "Get System Integration Status"
document_get_endpoint "/api/docs" "Get Documentation"

# Document POST endpoints
document_post_endpoint "/api/test/sudo" "Test and Set Sudo Password" '{"password": "test_password"}'
document_post_endpoint "/api/docker/install" "Install Docker" ""
document_post_endpoint "/api/docker/service/start" "Start Docker Service" ""
document_post_endpoint "/api/docker/service/stop" "Stop Docker Service" ""
document_post_endpoint "/api/docker/service/restart" "Restart Docker Service" ""
document_post_endpoint "/api/docker/service/enable" "Enable Docker Service" ""
document_post_endpoint "/api/docker/service/disable" "Disable Docker Service" ""
document_post_endpoint "/api/docker/system/prune" "Cleanup System" ""
document_post_endpoint "/api/docker/images/pull" "Pull Image" '{"image": "hello-world:latest"}'

# Document DELETE endpoints
document_delete_endpoint "/api/docker/uninstall" "Uninstall Docker"

echo "API documentation completed in $OUTPUT_FILE"
