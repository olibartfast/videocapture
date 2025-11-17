# CI/CD Pipeline for VideoCapture

This document describes the continuous integration and continuous deployment (CI/CD) pipelines configured for the VideoCapture project.

## Workflows

### 1. CI Workflow (`.github/workflows/ci.yml`)

**Triggers:**
- Push to `master`, `main`, or `develop` branches
- Pull requests to `master`, `main`, or `develop` branches
- Manual workflow dispatch

**Jobs:**

#### Build and Test
- **Matrix Strategy:** Tests across multiple OS versions (Ubuntu 22.04, 24.04) and backend configurations (OpenCV, GStreamer, FFmpeg, all)
- **Steps:**
  - Install dependencies based on the backend configuration
  - Configure CMake with appropriate flags
  - Build the project
  - Run unit tests via CTest
  - Upload build logs if tests fail

#### Code Quality
- Runs on Ubuntu 22.04
- Performs code formatting checks with `clang-format`
- Runs static analysis with `cppcheck`
- Uploads analysis reports as artifacts

#### Build with Sanitizers
- **Matrix Strategy:** Tests with Address, Undefined Behavior, and Thread sanitizers
- Uses Clang compiler for sanitizer support
- Helps detect memory leaks, undefined behavior, and threading issues

#### Cross-Platform
- **Matrix Strategy:** Tests on macOS (Intel and Apple Silicon)
- Ensures compatibility across different platforms
- Uses Homebrew for dependency management on macOS

### 2. Release Workflow (`.github/workflows/release.yml`)

**Triggers:**
- Push of version tags (e.g., `v1.0.0`)
- Manual workflow dispatch with tag input

**Jobs:**

#### Create Release
- Extracts version information from tag
- Creates a GitHub release with changelog

#### Build Release
- **Matrix Strategy:** Builds for Ubuntu 22.04, 24.04, macOS 13, and latest macOS with OpenCV and full backends
- Packages library, headers, and binaries
- Uploads release artifacts as compressed archives
- Attaches artifacts to the GitHub release

### 3. Coverage Workflow (`.github/workflows/coverage.yml`)

**Triggers:**
- Push to `master`, `main`, or `develop` branches
- Pull requests to `master`, `main`, or `develop` branches
- Manual workflow dispatch

**Steps:**
- Builds project with all backends enabled
- Compiles with coverage flags (`--coverage`, `-fprofile-arcs`, `-ftest-coverage`)
- Runs all tests
- Generates coverage reports using `lcov`
- Uploads reports to Codecov
- Generates HTML coverage reports as artifacts

### 4. Docker Workflow (`.github/workflows/docker.yml`)

**Triggers:**
- Push to `master` or `main` branches
- Push of version tags
- Pull requests to `master` or `main` branches
- Manual workflow dispatch

**Variants:**
- **opencv:** Minimal image with OpenCV support only
- **full:** Complete image with OpenCV, GStreamer, and FFmpeg support

**Steps:**
- Creates appropriate Dockerfile for each variant
- Builds Docker images using BuildKit
- Pushes images to GitHub Container Registry (ghcr.io)
- Tags images appropriately (branch name, version, latest)
- Tests built images

### 5. Dependency Check Workflow (`.github/workflows/dependency-check.yml`)

**Triggers:**
- Weekly schedule (Mondays at 00:00 UTC)
- Manual workflow dispatch

**Steps:**
- Checks latest available versions of OpenCV, GStreamer, FFmpeg, and CMake
- Compares with versions in `versions.env`
- Creates GitHub issue if updates are available
- Helps maintain up-to-date dependencies

### 6. Documentation Workflow (`.github/workflows/docs.yml`)

**Triggers:**
- Push to `master` or `main` branches (when documentation files change)
- Pull requests affecting documentation
- Manual workflow dispatch

**Jobs:**

#### Build Docs
- Generates API documentation using Doxygen
- Checks for broken links in Markdown files
- Uploads documentation as artifacts

#### Deploy Docs
- Deploys generated documentation to GitHub Pages
- Only runs on push to main/master branches

## Configuration Files

### `.editorconfig`
Defines consistent coding styles across different editors and IDEs:
- Unix-style line endings (LF)
- UTF-8 encoding
- 4 spaces for C++ and CMake files
- 2 spaces for YAML and JSON files

### `.clang-format`
Code formatting rules based on Google style with customizations:
- 4 space indentation
- 100 character column limit
- Attach braces style
- C++17 standard

### `.github/markdown-link-check-config.json`
Configuration for markdown link checking:
- Ignores localhost URLs
- Ignores GitHub commit URLs
- Accepts various HTTP status codes

## Badges

Add these badges to your README.md:

```markdown
![CI](https://github.com/olibartfast/videocapture/workflows/CI/badge.svg)
![Coverage](https://github.com/olibartfast/videocapture/workflows/Code%20Coverage/badge.svg)
[![codecov](https://codecov.io/gh/olibartfast/videocapture/branch/master/graph/badge.svg)](https://codecov.io/gh/olibartfast/videocapture)
![Docker](https://github.com/olibartfast/videocapture/workflows/Docker/badge.svg)
```

## Usage

### Running Tests Locally

```bash
# Configure with tests enabled
cmake -B build -DBUILD_TESTS=ON

# Build
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

### Building Docker Images Locally

```bash
# OpenCV variant
docker build -t videocapture:opencv -f .github/workflows/Dockerfile.opencv .

# Full variant
docker build -t videocapture:full -f .github/workflows/Dockerfile.full .
```

### Creating a Release

1. Create and push a version tag:
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

2. The release workflow will automatically:
   - Create a GitHub release
   - Build artifacts for all platforms
   - Upload release packages

### Manual Workflow Dispatch

All workflows can be manually triggered from the GitHub Actions tab:
1. Go to the Actions tab
2. Select the workflow
3. Click "Run workflow"
4. Fill in any required inputs
5. Click "Run workflow"

## Secrets Required

- `GITHUB_TOKEN` - Automatically provided by GitHub Actions
- `CODECOV_TOKEN` - Optional, for Codecov integration (if using private repo)

## Maintenance

### Adding New Dependencies

1. Update `versions.env` with minimum version requirements
2. Update dependency installation steps in all relevant workflows
3. Update `docs/DEPENDENCY_MANAGEMENT.md`

### Adding New Tests

Tests are automatically discovered by CTest. Simply add new test files to the `tests/` directory and update `tests/CMakeLists.txt`.

### Modifying Build Matrix

Edit the `matrix` section in the workflow files to add/remove:
- Operating systems
- Backend configurations
- Compiler versions
- Build types
