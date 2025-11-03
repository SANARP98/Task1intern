# FreeRTOS POSIX Simulator - Docker Container

This directory contains a containerized environment for building and running the FreeRTOS POSIX simulator demo without needing to install dependencies on your host machine.

## Overview

The Docker container:
- Uses Ubuntu 22.04 as base image
- Installs build tools (GCC, CMake, Git)
- Clones the FreeRTOS kernel at runtime
- Injects your project source files into the kernel examples
- Builds the CMake POSIX example
- Executes the FreeRTOS demo automatically

## Directory Structure

```
container/
├── Dockerfile         # Container image definition
├── entrypoint.sh      # Main script that runs inside container
├── run.sh             # Host script to build and run container
└── README.md          # This file
```

## Quick Start

### Basic Usage

From the container directory, run:

```bash
./run.sh
```

This will:
1. Build the Docker image (if not already built)
2. Run the container with your project mounted
3. Execute the FreeRTOS demo
4. Display output in your terminal

### With Automated Tests

To run the automated test sequence:

```bash
RUN_AUTOMATED_TESTS=1 ./run.sh
```

## How It Works

### 1. Volume Mounting (Not Copying)

The container uses **volume mounting** instead of copying files during build:

```bash
# In run.sh
docker run --rm \
    -v "${PROJECT_ROOT}:/project:ro" \
    "${IMAGE_NAME}"
```

This means:
- Your source files (main.c, FreeRTOSConfig.h) remain on your host
- The container can read them at runtime from `/project`
- No files are copied into the image during build
- Changes to your source files don't require rebuilding the image

### 2. Runtime Workflow

When you run `./run.sh`, here's what happens:

**On Host (`run.sh`):**
1. Builds Docker image `freertos-posix-sim`
2. Starts container with project mounted at `/project`
3. Passes `RUN_AUTOMATED_TESTS` environment variable if set

**Inside Container (`entrypoint.sh`):**
1. Validates project structure exists
2. Clones FreeRTOS kernel from GitHub
3. Copies your `main.c` → kernel's example
4. Copies your `FreeRTOSConfig.h` → kernel's config
5. Runs CMake to configure build
6. Builds the project
7. Executes the demo binary
8. Container exits when demo completes

## Environment Variables

You can customize the build by setting these environment variables:

### Host Variables (for `run.sh`)

| Variable | Default | Description |
|----------|---------|-------------|
| `IMAGE_NAME` | `freertos-posix-sim` | Docker image name/tag |
| `RUN_AUTOMATED_TESTS` | `0` | Set to `1` to enable automated tests |

Example:
```bash
IMAGE_NAME=my-freertos-test RUN_AUTOMATED_TESTS=1 ./run.sh
```

### Container Variables (for `entrypoint.sh`)

These can be passed via `docker run -e`:

| Variable | Default | Description |
|----------|---------|-------------|
| `PROJECT_DIR` | `/project` | Mount point for project sources |
| `KERNEL_REPO` | `https://github.com/FreeRTOS/FreeRTOS-Kernel.git` | FreeRTOS kernel repository |
| `KERNEL_BRANCH` | `main` | Branch to clone |
| `BUILD_DIR` | `/workspace/build` | CMake build directory |

Example:
```bash
docker run --rm \
    -e KERNEL_BRANCH=V11.0.1 \
    -v "$(pwd)/..:/project:ro" \
    freertos-posix-sim
```

## Manual Docker Commands

If you prefer not to use `run.sh`, you can run Docker commands manually:

### Build the Image

```bash
cd container
docker build -t freertos-posix-sim .
```

### Run the Container

```bash
# From ProblemStatement1/container directory
docker run --rm \
    -v "$(pwd)/../..:/project:ro" \
    freertos-posix-sim
```

### Run with Automated Tests

```bash
docker run --rm \
    -e RUN_AUTOMATED_TESTS=1 \
    -v "$(pwd)/../..:/project:ro" \
    freertos-posix-sim
```

### Interactive Shell (for Debugging)

```bash
docker run --rm -it \
    -v "$(pwd)/../..:/project:ro" \
    --entrypoint /bin/bash \
    freertos-posix-sim
```

Inside the container, you can then manually run:
```bash
/entrypoint.sh
```

## Troubleshooting

### Error: "ProblemStatement1 sources not found"

**Cause:** The volume mount path is incorrect.

**Solution:** Ensure you're mounting the repository root (which contains ProblemStatement1/):

```bash
# Correct - from container/ directory
docker run --rm -v "$(pwd)/../..:/project:ro" freertos-posix-sim

# Incorrect - mounting wrong directory
docker run --rm -v "$(pwd):/project:ro" freertos-posix-sim
```

### Error: "Failed to clone FreeRTOS kernel"

**Cause:** No internet connection, GitHub is unreachable, or SSL certificate verification failed.

**Solutions:**

1. **SSL Certificate Issues** (most common):
   The Dockerfile now includes `ca-certificates` package. Rebuild the image:
   ```bash
   docker build --no-cache -t freertos-posix-sim .
   ./run.sh
   ```

2. **Behind a proxy**:
   Configure Docker to use your proxy:
   ```bash
   docker build --build-arg HTTP_PROXY=http://proxy:port \
                --build-arg HTTPS_PROXY=http://proxy:port \
                -t freertos-posix-sim .
   ```

3. **No internet / GitHub unreachable**:
   Clone kernel manually and mount it:
   ```bash
   cd container
   git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git
   docker run --rm \
       -v "$(pwd)/../..:/project:ro" \
       -v "$(pwd)/FreeRTOS-Kernel:/workspace/FreeRTOS-Kernel:ro" \
       freertos-posix-sim
   ```

   Then modify `entrypoint.sh` to skip cloning if kernel already exists:
   ```bash
   if [ ! -d "/workspace/FreeRTOS-Kernel/.git" ]; then
       log "Cloning FreeRTOS kernel..."
       # ... existing clone logic
   fi
   ```

### Error: "docker: command not found"

**Cause:** Docker is not installed.

**Solution:** Install Docker:
- **macOS:** Install [Docker Desktop](https://www.docker.com/products/docker-desktop/)
- **Linux:** `sudo apt install docker.io` (Ubuntu/Debian)
- **Windows:** Install [Docker Desktop](https://www.docker.com/products/docker-desktop/)

### Build is Slow

**Cause:** Docker is cloning the kernel every time.

**Optimization:** Cache the kernel clone by adding a volume:

```bash
docker run --rm \
    -v "$(pwd)/../..:/project:ro" \
    -v freertos-kernel-cache:/workspace/FreeRTOS-Kernel \
    freertos-posix-sim
```

### Want to Keep Build Artifacts

By default, build artifacts are deleted when the container exits. To keep them:

```bash
mkdir -p build
docker run --rm \
    -v "$(pwd)/../..:/project:ro" \
    -v "$(pwd)/build:/workspace/build" \
    freertos-posix-sim

# Binary will be in build/example
```

## Technical Details

### Why Use Volumes Instead of COPY?

**Advantages of volume mounting:**
1. **No rebuild needed** when you change source files
2. **Faster iteration** - just re-run the container
3. **Smaller image size** - project files not in image layers
4. **Read-only safety** - `:ro` flag prevents accidental changes
5. **Easy debugging** - mount different versions without rebuilding

**The COPY approach would be:**
```dockerfile
COPY ../main.c /workspace/main.c
COPY ../FreeRTOSConfig.h /workspace/FreeRTOSConfig.h
```

But this requires rebuilding the image every time you modify the code.

### Container Architecture

```
Host Machine
├── ProblemStatement1/
│   ├── main.c
│   ├── FreeRTOSConfig.h
│   └── container/
│       ├── Dockerfile          ─┐
│       ├── entrypoint.sh       ─┤ Build image
│       └── run.sh              ─┘ Run container
│
└── (run.sh executes) ──────────┐
                                 │
                                 ▼
                    ┌────────────────────────┐
                    │   Docker Container     │
                    │   (Ubuntu 22.04)       │
                    ├────────────────────────┤
                    │ /project/              │ ← Volume mounted
                    │   ProblemStatement1/   │   (read-only)
                    │     ├── main.c         │
                    │     └── FreeRTOSConfig │
                    ├────────────────────────┤
                    │ /workspace/            │
                    │   FreeRTOS-Kernel/     │ ← Cloned at runtime
                    │     ├── examples/      │
                    │     └── portable/      │
                    ├────────────────────────┤
                    │ /entrypoint.sh         │ ← Runs automatically
                    └────────────────────────┘
                                 │
                                 ▼
                    Copies files, builds, runs demo
                                 │
                                 ▼
                         Output to console
```

### Security Considerations

- Container runs with root privileges (default)
- Project mounted read-only (`:ro`) to prevent accidental modifications
- Container is removed after execution (`--rm` flag)
- No data persistence between runs (stateless)

For production use, consider:
```bash
docker run --rm \
    --user $(id -u):$(id -g) \     # Run as current user
    --read-only \                   # Read-only root filesystem
    --tmpfs /workspace \            # Writable workspace
    -v "$(pwd)/../..:/project:ro" \
    freertos-posix-sim
```

## Advanced Usage

### Custom Kernel Branch

Test with a specific FreeRTOS version:

```bash
docker run --rm \
    -e KERNEL_BRANCH=V10.5.1 \
    -v "$(pwd)/../..:/project:ro" \
    freertos-posix-sim
```

### Custom Kernel Fork

Use your own kernel fork:

```bash
docker run --rm \
    -e KERNEL_REPO=https://github.com/yourname/FreeRTOS-Kernel.git \
    -e KERNEL_BRANCH=custom-feature \
    -v "$(pwd)/../..:/project:ro" \
    freertos-posix-sim
```

### Build with Extra CMake Flags

Modify `entrypoint.sh` to add flags:

```bash
cmake -B "${BUILD_DIR}" -S /workspace/FreeRTOS-Kernel/examples/cmake_example \
    -DFREERTOS_PORT=GCC_POSIX \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS="-Wall -Wextra -g"
```

### Multi-Stage Builds (Future Optimization)

For a smaller runtime image, use multi-stage builds:

```dockerfile
# Build stage
FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y build-essential cmake git
# ... build steps ...

# Runtime stage (much smaller)
FROM ubuntu:22.04
COPY --from=builder /workspace/build/example /example
ENTRYPOINT ["/example"]
```

## Comparison with Local Build

| Aspect | Docker Container | Local Build |
|--------|------------------|-------------|
| **Setup Time** | 5 min (first build) | 15-30 min (install tools) |
| **Dependencies** | Only Docker needed | GCC, CMake, Git, FreeRTOS |
| **Isolation** | Fully isolated | Uses system libraries |
| **Reproducibility** | Perfect (same Ubuntu 22.04) | Varies by system |
| **Cleanup** | Automatic (container deleted) | Manual |
| **Performance** | Slight overhead | Native speed |
| **Best For** | Quick testing, CI/CD | Development |

## CI/CD Integration

Example GitHub Actions workflow:

```yaml
name: Test FreeRTOS Demo

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Run FreeRTOS tests
        run: |
          cd ProblemStatement1/container
          RUN_AUTOMATED_TESTS=1 ./run.sh
```

## References

- [Docker Documentation](https://docs.docker.com/)
- [FreeRTOS Kernel Repository](https://github.com/FreeRTOS/FreeRTOS-Kernel)
- [Docker Best Practices](https://docs.docker.com/develop/dev-best-practices/)

## Support

If you encounter issues:

1. Check the troubleshooting section above
2. Verify Docker is running: `docker ps`
3. Check Docker logs: `docker logs <container-id>`
4. Run in interactive mode for debugging
5. Review `entrypoint.sh` for logic flow

## License

Part of the LIPL Embedded Systems Intern Assessment.
