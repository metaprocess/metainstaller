#!/bin/bash
set -axe
# Build and start services from docker-compose
source .env.baseimage

# IMAGE_NAME="$IMAGE_OS"
docker compose -f "docker-compose-create-baseimage.yml" --env-file ".env.baseimage" up  --build 

# Step 2: Authenticate to Docker Hub

# echo "$PASSWORD" | docker login ${REGISTRY_HOST} --username $USERNAME --password-stdin

# Tag and push
# docker push $REGISTRY_REPO_URL/$IMAGE_OS:$IMAGE_TAG
# docker tag $REGISTRY_REPO_URL/$IMAGE_OS:$IMAGE_TAG $REGISTRY_REPO_URL/$IMAGE_OS:latest
# docker push $REGISTRY_REPO_URL/$IMAGE_OS:latest

# docker push $REGISTRY_REPO_URL/$IMAGE_OS_WEB:$IMAGE_TAG_WEB
# docker tag $REGISTRY_REPO_URL/$IMAGE_OS_WEB:$IMAGE_TAG_WEB $REGISTRY_REPO_URL/$IMAGE_OS_WEB:latest
# docker push $REGISTRY_REPO_URL/$IMAGE_OS_WEB:latest

# Cleanup credentials
unset PASSWORD
