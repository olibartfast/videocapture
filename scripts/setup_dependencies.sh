#!/bin/bash

# Unified setup script for VideoCapture library dependencies
# This script installs video processing dependencies

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
ENABLE_GSTREAMER=false
DEPENDENCY_ROOT="$HOME/dependencies"
FORCE=false
VERBOSE=false

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --gstreamer              Enable GStreamer support"
    echo "  -r, --root PATH          Set dependency installation root (default: $HOME/dependencies)"
    echo "  -f, --force              Force reinstallation of dependencies"
    echo "  -v, --verbose            Enable verbose output"
    echo "  -h, --help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --gstreamer"
    echo "  $0 --gstreamer --root /opt/dependencies"
    echo "  $0 --force"
}

# Function to parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --gstreamer)
                ENABLE_GSTREAMER=true
                shift
                ;;
            -r|--root)
                DEPENDENCY_ROOT="$2"
                shift 2
                ;;
            -f|--force)
                FORCE=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

# Function to check if running in Docker
is_docker() {
    [[ -f /.dockerenv ]]
}

# Function to detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [[ -f /etc/os-release ]]; then
            . /etc/os-release
            echo "$ID"
        else
            echo "linux"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    else
        echo "unknown"
    fi
}

# Function to install system dependencies
install_system_dependencies() {
    local os=$(detect_os)
    print_status "Installing system dependencies for $os..."
    
    case $os in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                git \
                wget \
                curl \
                pkg-config \
                libopencv-dev \
                libopencv-contrib-dev
            ;;
        centos|rhel|fedora)
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y \
                cmake \
                git \
                wget \
                curl \
                pkg-config \
                opencv-devel \
                opencv-contrib-devel
            ;;
        *)
            print_warning "Unsupported OS: $os. Please install dependencies manually."
            ;;
    esac
}

# Function to setup GStreamer
setup_gstreamer() {
    if [[ "$ENABLE_GSTREAMER" != "true" ]]; then
        return 0
    fi
    
    local version="1.20.0"
    print_status "Setting up GStreamer $version..."
    
    local os=$(detect_os)
    case $os in
        ubuntu|debian)
            sudo apt-get update
            sudo apt-get install -y \
                libgstreamer1.0-dev \
                libgstreamer-plugins-base1.0-dev \
                libgstreamer-plugins-bad1.0-dev \
                gstreamer1.0-plugins-base \
                gstreamer1.0-plugins-good \
                gstreamer1.0-plugins-bad \
                gstreamer1.0-plugins-ugly \
                gstreamer1.0-libav \
                gstreamer1.0-tools \
                gstreamer1.0-x \
                gstreamer1.0-alsa \
                gstreamer1.0-gl \
                gstreamer1.0-gtk3 \
                gstreamer1.0-qt5 \
                gstreamer1.0-pulseaudio
            ;;
        centos|rhel|fedora)
            sudo yum install -y \
                gstreamer1-devel \
                gstreamer1-plugins-base-devel \
                gstreamer1-plugins-good \
                gstreamer1-plugins-bad-free \
                gstreamer1-plugins-bad-free-devel \
                gstreamer1-plugins-ugly-free \
                gstreamer1-plugins-ugly-free-devel
            ;;
        *)
            print_warning "GStreamer installation not supported on $os. Please install manually."
            return 1
            ;;
    esac
    
    print_success "GStreamer installed"
}

# Function to validate OpenCV installation
validate_opencv() {
    print_status "Validating OpenCV installation..."
    
    if command -v pkg-config &> /dev/null; then
        if pkg-config --exists opencv4; then
            local version=$(pkg-config --modversion opencv4)
            print_success "OpenCV $version found via pkg-config"
        elif pkg-config --exists opencv; then
            local version=$(pkg-config --modversion opencv)
            print_success "OpenCV $version found via pkg-config"
        else
            print_warning "OpenCV not found via pkg-config. Checking system installation..."
        fi
    fi
    
    # Check for OpenCV headers
    if [[ -f "/usr/include/opencv4/opencv2/opencv.hpp" ]]; then
        print_success "OpenCV headers found at /usr/include/opencv4"
    elif [[ -f "/usr/include/opencv2/opencv.hpp" ]]; then
        print_success "OpenCV headers found at /usr/include/opencv2"
    else
        print_error "OpenCV headers not found. Please install OpenCV development packages."
        exit 1
    fi
}

# Function to validate GStreamer installation
validate_gstreamer() {
    if [[ "$ENABLE_GSTREAMER" != "true" ]]; then
        return 0
    fi
    
    print_status "Validating GStreamer installation..."
    
    if command -v gst-launch-1.0 &> /dev/null; then
        local version=$(gst-launch-1.0 --version | head -n1 | cut -d' ' -f3)
        print_success "GStreamer $version found"
    else
        print_error "GStreamer not found. Please install GStreamer development packages."
        exit 1
    fi
    
    # Check for GStreamer development files
    if [[ -f "/usr/include/gstreamer-1.0/gst/gst.h" ]]; then
        print_success "GStreamer development headers found"
    else
        print_error "GStreamer development headers not found. Please install GStreamer development packages."
        exit 1
    fi
}

# Function to create environment setup script
create_env_setup() {
    local env_file="$DEPENDENCY_ROOT/setup_videocapture_env.sh"
    
    print_status "Creating environment setup script..."
    
    mkdir -p "$DEPENDENCY_ROOT"
    
    cat > "$env_file" << EOF
#!/bin/bash
# Environment setup script for VideoCapture dependencies
# Generated by setup_dependencies.sh

export DEPENDENCY_ROOT="$DEPENDENCY_ROOT"

# OpenCV paths (if installed in custom location)
if [[ -d "\$DEPENDENCY_ROOT/opencv" ]]; then
    export OpenCV_DIR="\$DEPENDENCY_ROOT/opencv"
    export LD_LIBRARY_PATH="\$DEPENDENCY_ROOT/opencv/lib:\$LD_LIBRARY_PATH"
fi

# GStreamer paths (if installed in custom location)
if [[ -d "\$DEPENDENCY_ROOT/gstreamer" ]]; then
    export GSTREAMER_ROOT_DIR="\$DEPENDENCY_ROOT/gstreamer"
    export LD_LIBRARY_PATH="\$DEPENDENCY_ROOT/gstreamer/lib:\$LD_LIBRARY_PATH"
fi

echo "VideoCapture environment variables set"
EOF
    
    chmod +x "$env_file"
    print_success "Environment setup script created: $env_file"
}

# Main function
main() {
    print_status "VideoCapture Dependency Setup Script"
    print_status "===================================="
    
    # Parse command line arguments
    parse_args "$@"
    
    print_status "GStreamer enabled: $ENABLE_GSTREAMER"
    print_status "Dependency root: $DEPENDENCY_ROOT"
    print_status "Force reinstall: $FORCE"
    
    # Install system dependencies
    install_system_dependencies
    
    # Setup GStreamer if requested
    setup_gstreamer
    
    # Validate installations
    validate_opencv
    validate_gstreamer
    
    # Create environment setup script
    create_env_setup
    
    print_success "Setup completed successfully!"
    print_status "To use the dependencies, source the environment setup script:"
    print_status "  source $DEPENDENCY_ROOT/setup_videocapture_env.sh"
}

# Run main function with all arguments
main "$@" 