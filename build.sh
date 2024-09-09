#!/bin/bash
# Determine the operating system and set the correct premake binary
if [ "$(uname)" = "Darwin" ]; then
    premakeBinary="bin/premake5-mac"
elif [ "$(uname)" = "Linux" ]; then
    premakeBinary="bin/premake5-linux"
else
    echo "Unsupported operating system"
    exit 1
fi

PrintHelp() {
    echo "Usage:"
    echo "    ./build.sh compile [build_system] [configuration] [num_cores]"
    echo "    ./build.sh run [build_system] [configuration] [num_cores]"
    echo ""
    echo "Example:"
    echo "    ./build.sh compile gmake2 Release 8"
    echo "    ./build.sh run gmake2 Debug 4"
    echo ""
    echo "Parameters:"
    echo "    build_system: The build system to use (default: gmake2)"
    echo "    configuration: The build configuration (default: Debug)"
    echo "    num_cores: The number of cores to use for building (default: 4)"
    echo ""
    echo "Available build systems:"
    $premakeBinary --help | awk '/ACTIONS/{flag=1; next} flag'
}

Compile() {
    $premakeBinary "$buildSystem" || exit 1
    cd build/"$buildSystem"
    make CONFIG="$configuration" -j"$numCores" || exit 1
    cd -
}

Run() {
    Compile
    # ./build/gmake2/bin/x86/Release/voxels
    ./build/"$buildSystem"/bin/x86/"$configuration"/voxels
}

# Default values
buildSystem=${2:-gmake2}
configuration=${3:-Release}
numCores=${4:-4}

# Main script logic
case "$1" in
    compile)
        Compile
        ;;
    run)
        Run
        ;;
    *)
        PrintHelp
        ;;
esac
