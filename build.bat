@echo off

IF "%~1" == "" GOTO PrintHelp
IF "%~1" == "compile" GOTO Compile
IF "%~1" == "run" GOTO Run
if "%~1" == "clean" GOTO Clean

vendor\premake5.exe %1

:PrintHelp
    echo Usage:
    echo     .\build.bat compile [build_system] [configuration] [num_cores]
    echo     .\build.bat run [build_system] [configuration] [num_cores]
    echo Example: 
    echo     .\build.bat compile vs2022 Release 8
    echo     .\build.bat run vs2022 Debug 4
    echo Parameters:
    echo     build_system: The build system to use (default: vs2022)
    echo     configuration: The build configuration (default: Debug)
    echo     num_cores: The number of cores to use for building (default: 4)
    
    echo Available build systems:
    vendor\premake5.exe --help | findstr /R /C:"^ clean" /C:"^ codelite" /C:"^ gmake" /C:"^ gmake2" /C:"^ vs2005" /C:"^ vs2008" /C:"^ vs2010" /C:"^ vs2012" /C:"^ vs2013" /C:"^ vs2015" /C:"^ vs2017" /C:"^ vs2019" /C:"^ vs2022" /C:"^ xcode4"

    GOTO Done

:Compile
    set buildSystem=%2
    if "%buildSystem%" == "" set buildSystem=vs2022

    set configuration=%3
    if "%configuration%" == "" set configuration=Debug

    set numCores=%4
    if "%numCores%" == "" set numCores=4

    vendor\premake5.exe %buildSystem%

    if not defined DevEnvDir (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    )

    set solutionFile=build\%buildSystem%\voxels-senior-project.sln
    msbuild /t:Build /p:Configuration=%configuration% /p:Platform=x64 /m:%numCores% "%solutionFile%"
    GOTO Done

:Run
    set buildSystem=%2
    if "%buildSystem%" == "" set buildSystem=vs2022

    set configuration=%3
    if "%configuration%" == "" set configuration=Debug

    set numCores=%4
    if "%numCores%" == "" set numCores=4

    call :Compile compile %buildSystem% %configuration% %numCores%

    set executable=.\build\%buildSystem%\bin\x86_64\%configuration%\voxels.exe
    if exist "%executable%" (
        echo Running %executable%
        "%executable%"
    ) else (
        echo Executable not found: %executable%
    )
    GOTO Done

:Clean
    echo Deleting build directories...
    rmdir /s /q build
    echo Clean complete.
    GOTO Done

:Done