#!/bin/bash

# FFmpeg setup script for VideoCapture library
# This script installs FFmpeg development libraries

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
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

# Function to check if FFmpeg is already installed
check_ffmpeg_installed() {
    if pkg-config --exists libavformat libavcodec libavutil libswscale 2>/dev/null; then
        local version=$(pkg-config --modversion libavformat)
        print_success "FFmpeg is already installed (version: $version)"
        return 0
    fi
    return 1
}

# Function to install FFmpeg on Ubuntu/Debian
install_ffmpeg_ubuntu() {
    print_status "Installing FFmpeg on Ubuntu/Debian..."
    
    sudo apt-get update
    sudo apt-get install -y \
        libavformat-dev \
        libavcodec-dev \
        libavutil-dev \
        libswscale-dev \
        libavdevice-dev \
        libavfilter-dev \
        pkg-config
    
    print_success "FFmpeg installed successfully"
}

# Function to install FFmpeg on Fedora/RHEL/CentOS
install_ffmpeg_fedora() {
    print_status "Installing FFmpeg on Fedora/RHEL/CentOS..."
    
    sudo dnf install -y \
        ffmpeg-devel \
        pkgconfig
    
    print_success "FFmpeg installed successfully"
}

# Function to install FFmpeg on Arch Linux
install_ffmpeg_arch() {
    print_status "Installing FFmpeg on Arch Linux..."
    
    sudo pacman -S --needed --noconfirm \
        ffmpeg \
        pkgconf
    
    print_success "FFmpeg installed successfully"
}

# Function to install FFmpeg on macOS
install_ffmpeg_macos() {
    print_status "Installing FFmpeg on macOS..."
    
    if ! command -v brew &> /dev/null; then
        print_error "Homebrew is not installed. Please install Homebrew first:"
        print_error "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi
    
    brew install ffmpeg pkg-config
    
    print_success "FFmpeg installed successfully"
}

# Main installation logic
main() {
    print_status "Starting FFmpeg setup..."
    
    # Check if already installed
    if check_ffmpeg_installed && [[ "$1" != "--force" ]]; then
        print_status "Use --force to reinstall FFmpeg"
        exit 0
    fi
    
    # Detect OS and install
    OS=$(detect_os)
    print_status "Detected OS: $OS"
    
    case "$OS" in
        ubuntu|debian)
            install_ffmpeg_ubuntu
            ;;
        fedora|rhel|centos)
            install_ffmpeg_fedora
            ;;
        arch|manjaro)
            install_ffmpeg_arch
            ;;
        macos)
            install_ffmpeg_macos
            ;;
        *)
            print_error "Unsupported OS: $OS"
            print_error "Please install FFmpeg manually:"
            print_error "  - Ubuntu/Debian: sudo apt-get install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev"
            print_error "  - Fedora/RHEL: sudo dnf install ffmpeg-devel"
            print_error "  - Arch: sudo pacman -S ffmpeg"
            print_error "  - macOS: brew install ffmpeg"
            exit 1
            ;;
    esac
    
    # Verify installation
    if check_ffmpeg_installed; then
        print_success "FFmpeg setup completed successfully!"
    else
        print_error "FFmpeg installation verification failed"
        exit 1
    fi
}

# Parse arguments and run
main "$@"
