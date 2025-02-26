# NZB Article Availability Checker

![License](https://img.shields.io/badge/license-GPLv3-blue.svg)

## Introduction

**NZB Article Availability Checker** is a C++ application designed to verify the availability of NZB articles across multiple NNTP (Network News Transfer Protocol) providers. It efficiently utilizes multiple connections to perform parallel processing, significantly reducing the time required to check large sets of NZB articles.

## Features

- **Parallel Processing:** Supports multiple concurrent connections per NNTP provider to expedite article availability checks.
- **Thread-Safe Queue:** Utilizes a thread-safe queue to manage article IDs efficiently across multiple threads.
- **Comprehensive Reporting:** Generates detailed reports highlighting total articles, available articles, and availability percentages for each server.
- **SSL Support:** Capable of handling both SSL and non-SSL NNTP servers.
- **Authentication:** Supports NNTP servers requiring authentication.
- **Command-Line Interface:** Easily specify the NZB file path via command-line arguments.

## Prerequisites

Before building and running the application, ensure you have the following installed on your system:

- **C++ Compiler:** Supports C++11 or later (e.g., `g++`, `clang++`, MSVC).
- **CMake:** Version 3.10 or later.
- **Boost Libraries:** Specifically, Boost.Asio and Boost.System.
- **OpenSSL:** For SSL/TLS support.
- **JsonCpp:** For JSON parsing.
- **pugixml:** For XML parsing.
- **pkg-config:** (Optional) For managing compiler and linker flags.

## Installation

### Using Package Managers

#### Ubuntu/Debian

1. **Update Package List:**

 ```sh
 sudo apt-get update
 sudo apt-get install build-essential cmake libboost-all-dev libssl-dev libjsoncpp-dev libpugixml-dev
 ```
### macOS

1. **Install Homebrew: (If not already installed):**

  ```sh
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  ```

2. **Install Dependencies:**

 ```sh
 brew install cmake boost openssl jsoncpp pugixml
 ```

### Windows

1. **Install vcpkg:**
   Follow the [vcpkg Quick Start guide](https://github.com/microsoft/vcpkg#quick-start) to set up vcpkg on your system.

2. **Install Dependencies via vcpkg:**
   ```sh
   vcpkg install boost-asio boost-system openssl jsoncpp pugixml
   vcpkg integrate install
   ```

   This command installs the required libraries and integrates vcpkg with Visual Studio for automatic discovery of installed packages.

## Building from source

1. Clone
2. cmake ..
3. make
