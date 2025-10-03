# Alpine builder image for producing a statically linked MetaInstaller binary
# This image is intended to be used with docker-compose.build.yml which mounts
# the project source and builds into the host's build_docker/ directory.

FROM alpine:3.22

# Toolchain + static libs for fully static linking
#     echo "https://dl-cdn.alpinelinux.org/alpine/edge/main" >> /etc/apk/repositories; \
#     echo "https://dl-cdn.alpinelinux.org/alpine/edge/community" >> /etc/apk/repositories; \
#     echo "https://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories; \
RUN apk add --no-cache \
      build-base \
      cmake \
      ninja \
      linux-headers \
      pkgconfig \
      git \
      libstdc++-dev \
      bash \
      && rm -rf /var/cache/apk/*
      # openssl-dev \
      # openssl-libs-static \
      # zlib-dev \
      # zlib-static \
      # boost-dev \
      # boost-static \

# Work in a mounted workspace (from compose)
WORKDIR /workspace

# Default command is a no-op hint; compose will override with the build steps
CMD ["sh", "-lc", "echo 'Builder ready. Use docker compose to run the build.'"]
