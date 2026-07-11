@echo off
setlocal enabledelayedexpansion

IF "%~1" == "" GOTO PrintHelp
IF "%~1" == "compile" GOTO Compile
IF "%~1" == "run" GOTO Run
if "%~1" == "clean" GOTO Clean

bin\premake5.exe %1

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
    bin\premake5.exe --help | findstr /R /C:"^ clean" /C:"^ codelite" /C:"^ gmake" /C:"^ gmake2" /C:"^ vs2005" /C:"^ vs2008" /C:"^ vs2010" /C:"^ vs2012" /C:"^ vs2013" /C:"^ vs2015" /C:"^ vs2017" /C:"^ vs2019" /C:"^ vs2022" /C:"^ xcode4"

    GOTO Done

:Compile
    set buildSystem=%2
    if "%buildSystem%" == "" set buildSystem=vs2022

    set configuration=%3
    if "%configuration%" == "" set configuration=Debug

    set numCores=%4
    if "%numCores%" == "" set numCores=4

    bin\premake5.exe %buildSystem%

    if not defined DevEnvDir (
        set vsWhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
        set vsDevCmd=

        if exist !vsWhere! (
            for /f "usebackq tokens=*" %%i in (`!vsWhere! -latest -products * -requires Microsoft.Component.MSBuild -find Common7\Tools\VsDevCmd.bat`) do set vsDevCmd=%%i
        )

        if not defined vsDevCmd (
            echo Could not locate VsDevCmd.bat via vswhere. Falling back to common install paths.
            for %%p in (
                "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
                "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
                "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
                "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
            ) do (
                if exist %%p set vsDevCmd=%%~p
            )
        )

        if defined vsDevCmd (
            call "!vsDevCmd!" -arch=x64
        ) else (
            echo Could not find a Visual Studio 2022 installation with VsDevCmd.bat.
        )
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

    set executable=.\build\%buildSystem%\bin\x86_64\%configuration%\voxels.exe

    del "%executable%"

    call :Compile compile %buildSystem% %configuration% %numCores%

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