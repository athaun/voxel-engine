## UTRGV Senior Project 2024
Asher Haun and Victor Contreras
Advised by Dr. Emmet Tomai

[Project board](https://github.com/orgs/UTRGV-CS-Projects/projects/2?query=sort%3Aupdated-desc+is%3Aopen)

### Requirements
- [Premake 5](https://premake.github.io/) (bundled with repository)
- Clang++, VS22 or other C++ compiler supported by Premake5
- Git

## Setup and running

### Build script usage:
Run the script without parameters for usage instructions.
```sh
# DOS
> .\build.bat
# Linux
$ ./build.sh
```

### Example compilation command
The `compile` command will generate a build solution in the `./build/[build_system]/` directory.
`[num_cores]` can be left blank, default: 4, or input as a number by you.
```sh
# DOS
> .\build.bat compile vs2022 Release [num_cores]
# Linux
$ ./build.sh compile gmake2 Release [num_cores]
```
If you would prefer to create a build solution and use an IDE, this will work, or you can use premake as normal.

Run the binary
The `compile` command will build an executable within the following directory structure:
```
./build/[build_system]/bin/[architecture]/[Release|Debug]/voxels[.exe]
```

### Compile and Run
```sh
# DOS
> .\build.bat run vs2022 Release [num_cores]
# Linux
$ ./build.sh run gmake2 Release [num_cores]
```

### Credits
Our premake5.lua script is based on the the [Minimal BGFX Example Project](https://github.com/jpcy/bgfx-minimal-example) by @jpcy, used with permission through the [BSD-2 License](https://img.shields.io/badge/license-BSD--2%20clause-blue.svg)](https://bkaradzic.github.io/bgfx/license.html).


