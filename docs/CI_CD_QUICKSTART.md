# Quick Start Guide: CI/CD Pipeline

This guide helps you get started with the CI/CD pipeline that has been set up for the VideoCapture project.

## Overview

The CI/CD pipeline consists of 7 GitHub Actions workflows that automate building, testing, and releasing your project:

1. **CI** - Main continuous integration workflow
2. **Release** - Automated releases
3. **Coverage** - Code coverage analysis
4. **Docker** - Container image building
5. **Documentation** - API docs generation
6. **Dependency Check** - Weekly dependency monitoring
7. **Format Check** - Code formatting validation

## First Time Setup

### 1. Enable GitHub Actions

GitHub Actions should be enabled automatically for your repository. Verify by:
- Go to your repository on GitHub
- Click on the "Actions" tab
- You should see all workflows listed

### 2. Configure Branch Protection (Optional but Recommended)

Protect your main branch by requiring CI to pass:
1. Go to Settings → Branches
2. Add a branch protection rule for `master` or `main`
3. Enable "Require status checks to pass before merging"
4. Select the CI workflow checks

### 3. Set Up Codecov (Optional)

For code coverage reporting:
1. Go to [codecov.io](https://codecov.io)
2. Sign up with your GitHub account
3. Add your repository
4. Copy the token (only needed for private repos)
5. Add it as a secret in your repo: Settings → Secrets → New repository secret
   - Name: `CODECOV_TOKEN`
   - Value: (your token)

### 4. Enable GitHub Pages (Optional)

To publish API documentation:
1. Go to Settings → Pages
2. Source: "Deploy from a branch"
3. Branch: `gh-pages` / `(root)`
4. Save

Documentation will be available at: `https://olibartfast.github.io/videocapture/`

## Using the Workflows

### Running CI on Pull Requests

CI automatically runs when you:
- Create a pull request
- Push commits to a pull request
- Push to `master`, `main`, or `develop` branches

**What it does:**
- Builds with all backend combinations
- Runs all unit tests
- Tests on Ubuntu 22.04 and 24.04
- Tests on macOS
- Runs sanitizer checks
- Performs code quality analysis

### Creating a Release

**Automated Release:**
1. Create and push a version tag:
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

2. The release workflow will:
   - Create a GitHub release
   - Build for all platforms
   - Upload release artifacts

**Manual Release:**
1. Go to Actions → Release → Run workflow
2. Enter the tag name (e.g., `v1.0.0`)
3. Click "Run workflow"

### Checking Code Coverage

Coverage runs automatically on every push to main branches:
1. Go to Actions → Code Coverage
2. Click on the latest run
3. Download the coverage report artifact
4. Or view on Codecov (if configured)

### Building Docker Images

Docker images are built automatically on:
- Pushes to `master`/`main`
- Pull requests
- Version tags

**Pull a pre-built image:**
```bash
docker pull ghcr.io/olibartfast/videocapture:latest-opencv
docker pull ghcr.io/olibartfast/videocapture:latest-full
```

**Run the container:**
```bash
docker run -it ghcr.io/olibartfast/videocapture:latest-full
```

### Manually Triggering Workflows

All workflows can be manually triggered:
1. Go to the Actions tab
2. Select the workflow
3. Click "Run workflow"
4. Select branch and fill in any inputs
5. Click "Run workflow"

## Local Development

### Running Tests Locally

```bash
# Configure with all backends
cmake -B build -DBUILD_TESTS=ON -DUSE_GSTREAMER=ON -DUSE_FFMPEG=ON

# Build
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

### Formatting Code

Before committing, format your code:

```bash
# Format all C++ files
find src include tests app -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs clang-format -i

# Check formatting without modifying files
find src include tests app -name "*.cpp" -o -name "*.hpp" | while read file; do
  clang-format --dry-run -Werror "$file"
done
```

### Running Sanitizers Locally

```bash
# Address sanitizer
cmake -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address" \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan
cd build-asan && ctest --output-on-failure

# Undefined behavior sanitizer
cmake -B build-ubsan -DCMAKE_CXX_FLAGS="-fsanitize=undefined" \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build-ubsan
cd build-ubsan && ctest --output-on-failure
```

### Building Docker Images Locally

```bash
# Create a simple Dockerfile for testing
cat > Dockerfile <<EOF
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y build-essential cmake libopencv-dev
COPY . /app
WORKDIR /app
RUN cmake -B build && cmake --build build
EOF

# Build the image
docker build -t videocapture:test .

# Run it
docker run -it videocapture:test
```

## Workflow Status

Monitor workflow status:
- **GitHub UI**: Actions tab shows all runs
- **Badges**: README.md displays status badges
- **Notifications**: Configure in Settings → Notifications

## Troubleshooting

### CI Failing

1. **Check the logs**: Click on the failed job in the Actions tab
2. **Run locally**: Try to reproduce the issue on your machine
3. **Check dependencies**: Ensure all required packages are installed

### Format Check Failing

Run the formatter locally before pushing:
```bash
clang-format -i path/to/modified/file.cpp
```

### Build Failing on Specific Backend

Test that backend locally:
```bash
cmake -B build -DUSE_GSTREAMER=ON  # or USE_FFMPEG=ON
cmake --build build
```

### Docker Build Failing

1. Check Dockerfile syntax
2. Verify all dependencies are available
3. Test locally with `docker build`

## Common Tasks

### Adding a New Test

1. Create test file in `tests/` directory
2. Update `tests/CMakeLists.txt`
3. Push changes - CI will automatically discover and run new tests

### Updating Dependencies

1. Update `versions.env`
2. Update workflow files if needed
3. Test locally
4. Create PR - CI will validate changes

### Changing Build Configuration

1. Edit relevant workflow file in `.github/workflows/`
2. Test changes on a feature branch
3. Review workflow run results
4. Merge when passing

## Best Practices

1. **Always run tests locally** before pushing
2. **Format code** before committing
3. **Write descriptive commit messages**
4. **Keep PRs focused** - one feature/fix per PR
5. **Update documentation** when adding features
6. **Add tests** for new functionality
7. **Check CI status** before merging PRs

## Getting Help

- **CI Documentation**: See [docs/CI_CD.md](CI_CD.md)
- **GitHub Actions Docs**: https://docs.github.com/actions
- **Issues**: Open an issue using the templates in `.github/ISSUE_TEMPLATE/`

## What's Next?

- [ ] Review workflow configurations
- [ ] Set up branch protection
- [ ] Configure Codecov (if desired)
- [ ] Enable GitHub Pages for docs
- [ ] Customize workflows for your needs
- [ ] Add project-specific badges to README
