#!/bin/bash

PrintHelp() {
    echo "Usage:"
    echo "    ./build-linux.sh compile [build_system] [configuration] [num_cores]"
    echo "    ./build-linux.sh run [build_system] [configuration] [num_cores]"
    echo ""
    echo "Example:"
    echo "    ./build-linux.sh compile gmake2 Release 8"
    echo "    ./build-linux.sh run gmake2 Debug 4"
    echo ""
    echo "Parameters:"
    echo "    build_system: The build system to use (default: gmake2)"
    echo "    configuration: The build configuration (default: Debug)"
    echo "    num_cores: The number of cores to use for building (default: 4)"
    echo ""
    echo "Available build systems:"
    vendor/premake5-linux --help | awk '/ACTIONS/{flag=1; next} flag'
}

Compile() {
    vendor/premake5-linux "$buildSystem"
    cd build/"$buildSystem"
    make CONFIG="$configuration" -j"$numCores"
    cd -
}

Run() {
    Compile

    executable="./build/$buildSystem/bin/x86_64/$configuration/voxels"
    if [ -f "$executable" ]; then
        echo "Running $executable"
        "$executable"
    else
        echo "Executable not found: $executable"
    fi
}

Clean() {
    echo "Deleting build directories..."
    rm -rf build
    echo "Clean complete."
}

Done() {
    exit 0
}

if [ -z "$1" ]; then
    PrintHelp
    Done
fi

action="$1"
buildSystem=${2:-gmake2}
configuration=${3:-Debug}
numCores=${4:-4}

case $action in
    "compile")
        Compile
        ;;
    "run")
        Run
        ;;
    "clean")
        Clean
        ;;
    *)
        vendor/premake5-linux "$action"
        ;;
esac