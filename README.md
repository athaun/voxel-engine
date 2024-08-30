## bgfx-minimal-example

[![License](https://img.shields.io/badge/license-BSD--2%20clause-blue.svg)](https://bkaradzic.github.io/bgfx/license.html)

Project started using the [Minimal BGFX Example Project](https://github.com/jpcy/bgfx-minimal-example) by @jpcy.

## Requirements
- [Premake 5](https://premake.github.io/)
- Clang++, VS22 or other C++ compiler supported by Premake5
- Git

## Setup and running
clone submodules
```
git submodule init
git submodule update
```

Generate makefiles using [premake5](https://premake.github.io/)
run `premake5 --help` for a list of target build systems.
```
premake5 gmake2
cd build/gmake2/
```

Build the binaries
```
make all
```

Run the binary
```
./bin/x86_64/Release/helloworld
```
# 202510-fall-2024-projects-voxels
