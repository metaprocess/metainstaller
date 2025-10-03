# Use Node.js 18 as base image
FROM node:18.20.0-alpine

RUN apk add --no-cache \
      bash \
      gzip \
      rsync \
      && rm -rf /var/cache/apk/*

# Set working directory
WORKDIR /build

# Copy package files
COPY package*.json .

# Install dependencies
RUN npm install


# # Copy source code
# COPY . .

# # Expose port for development
# EXPOSE 5173

# # Build command
# RUN npm run build

# # Default command for development
# CMD ["npm", "run", "dev"]
