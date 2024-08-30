## UTRGV Senior Project 2024
Asher Haun and Victor Contreras
Advised by Dr. Emmet Tomai

Project based on the the [Minimal BGFX Example Project](https://github.com/jpcy/bgfx-minimal-example) by @jpcy, used with permission through the BSD-2 License: [![License](https://img.shields.io/badge/license-BSD--2%20clause-blue.svg)](https://bkaradzic.github.io/bgfx/license.html)

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

Generate build files using [premake5](https://premake.github.io/)
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
