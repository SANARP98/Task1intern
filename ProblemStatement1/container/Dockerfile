FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential \
       cmake \
       git \
       ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && update-ca-certificates

WORKDIR /workspace

# Copy the entrypoint script
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Copy project source files
# The container expects the full project to be mounted at /project at runtime,
# but we could also copy it at build time as a fallback
# COPY ../main.c /workspace/main.c
# COPY ../FreeRTOSConfig.h /workspace/FreeRTOSConfig.h

ENTRYPOINT ["/entrypoint.sh"]
