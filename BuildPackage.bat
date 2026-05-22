@echo off
REM ============================================================================
REM PoF Build & Package Script
REM Usage: BuildPackage.bat [development|shipping] [--skip-cook] [--no-version-bump]
REM ============================================================================

setlocal EnableDelayedExpansion

REM --- Configuration ---
set ENGINE_DIR=C:\Program Files\Epic Games\UE_5.7
set PROJECT_DIR=%~dp0
set PROJECT_FILE=%PROJECT_DIR%PoF.uproject
set UAT="%ENGINE_DIR%\Engine\Build\BatchFiles\RunUAT.bat"
set UBT="%ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
set BUILD_DIR=%PROJECT_DIR%Builds
set INI_FILE=%PROJECT_DIR%Config\DefaultGame.ini

REM --- Parse arguments ---
set BUILD_CONFIG=Shipping
set SKIP_COOK=0
set NO_VERSION_BUMP=0

:parse_args
if "%~1"=="" goto :done_args
if /i "%~1"=="development" set BUILD_CONFIG=Development
if /i "%~1"=="shipping" set BUILD_CONFIG=Shipping
if /i "%~1"=="--skip-cook" set SKIP_COOK=1
if /i "%~1"=="--no-version-bump" set NO_VERSION_BUMP=1
shift
goto :parse_args
:done_args

echo ============================================
echo  PoF Build Pipeline - %BUILD_CONFIG%
echo ============================================
echo.

REM --- Step 1: Increment build number ---
if %NO_VERSION_BUMP%==0 (
    echo [1/5] Incrementing build number...
    powershell -Command ^
        "$ini = Get-Content '%INI_FILE%' -Raw; " ^
        "if ($ini -match 'BuildNumber=(\d+)') { " ^
        "  $old = [int]$Matches[1]; $new = $old + 1; " ^
        "  $ini = $ini -replace 'BuildNumber=\d+', \"BuildNumber=$new\"; " ^
        "  $ini = $ini -replace 'BuildDate=.*', \"BuildDate=$(Get-Date -Format 'yyyy-MM-dd HH:mm')\"; " ^
        "  Set-Content '%INI_FILE%' $ini -NoNewline; " ^
        "  Write-Host \"  Build number: $old -> $new\" " ^
        "} else { Write-Host '  WARNING: BuildNumber not found in INI' }"
) else (
    echo [1/5] Skipping version bump
)

REM --- Step 2: Compile ---
echo.
echo [2/5] Compiling %BUILD_CONFIG%...
%UBT% PoF Win64 %BUILD_CONFIG% "-Project=%PROJECT_FILE%" -WaitMutex
if %ERRORLEVEL% NEQ 0 (
    if %ERRORLEVEL% NEQ 9666 (
        echo ERROR: Compilation failed with code %ERRORLEVEL%
        exit /b 1
    )
    echo   UBA error 9666 - retrying without UBA (normal)
)

REM --- Step 3: Content validation (editor builds only) ---
echo.
echo [3/5] Running content validation...
"%ENGINE_DIR%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_FILE%" -run=ContentValidation -nopause -unattended -nosplash 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo   WARNING: Content validation reported issues (see log above)
)

REM --- Step 4: Cook and package ---
echo.
if %SKIP_COOK%==1 (
    echo [4/5] Skipping cook (--skip-cook)
) else (
    echo [4/5] Cooking and packaging for Win64 %BUILD_CONFIG%...
    call %UAT% BuildCookRun ^
        -project="%PROJECT_FILE%" ^
        -noP4 ^
        -platform=Win64 ^
        -clientconfig=%BUILD_CONFIG% ^
        -cook ^
        -build ^
        -stage ^
        -pak ^
        -compressed ^
        -archive ^
        -archivedirectory="%BUILD_DIR%" ^
        -prereqs ^
        -nodebuginfo ^
        -utf8output
    if !ERRORLEVEL! NEQ 0 (
        echo ERROR: BuildCookRun failed
        exit /b 1
    )
)

REM --- Step 5: Summary ---
echo.
echo ============================================
echo  Build Complete!
echo  Config:  %BUILD_CONFIG%
echo  Output:  %BUILD_DIR%
echo ============================================

endlocal
exit /b 0
