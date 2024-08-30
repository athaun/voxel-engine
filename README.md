## bgfx-minimal-example

[![License](https://img.shields.io/badge/license-BSD--2%20clause-blue.svg)](https://bkaradzic.github.io/bgfx/license.html)

Minimal [bgfx](https://github.com/bkaradzic/bgfx/) "hello world" example.

This doesn't use the [bgfx example framework](https://github.com/bkaradzic/bgfx/tree/master/examples/common). [GLFW](https://www.glfw.org/) is used for windowing. There are separate single and multithreaded examples.

[Premake 5](https://premake.github.io/) is used instead of [GENie](https://github.com/bkaradzic/GENie), so this also serves as an example of how to build [bgfx](https://github.com/bkaradzic/bgfx/), [bimg](https://github.com/bkaradzic/bimg/) and [bx](https://github.com/bkaradzic/bx/) with a different build system.

### Related links

[Using the bgfx library with C++ on Ubuntu](https://www.sandeepnambiar.com/getting-started-with-bgfx/) and the associated [repository](https://github.com/gamedolphin/bgfx-sample).

[Hello, bgfx!](https://dev.to/pperon/hello-bgfx-4dka)


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
