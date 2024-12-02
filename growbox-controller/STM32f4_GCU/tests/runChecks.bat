@echo off
REM ==============================================
REM Batch-Skript zum Kompilieren und Ausführen von test_sha1.c und test_base64.c
REM ==============================================

REM --- Verzeichnisdefinitionen ---
set "TEST_DIR=D:\git\GrowBox_NG\growbox-controller\STM32f4_GCU\tests"
set "SRC_DIR=D:\git\GrowBox_NG\growbox-controller\STM32f4_GCU\Core\Src"
set "INC_DIR=D:\git\GrowBox_NG\growbox-controller\STM32f4_GCU\Core\Inc"

REM --- Compiler und Flags ---
set "COMPILER=gcc"
set "CFLAGS=-I"%INC_DIR%" -Wall -Wextra -O2"

REM --- Test 1: test_sha1.c ---
set "SOURCES_SHA1=%TEST_DIR%\test_sha1.c %SRC_DIR%\sha1.c %SRC_DIR%\base64.c"
set "OUTPUT_SHA1=%TEST_DIR%\test_sha1.exe"

echo ==============================================
echo Bauvorgang für test_sha1.exe
echo Compiler: %COMPILER%
echo Flags: %CFLAGS%
echo Quellen: %SOURCES_SHA1%
echo Ausgabe: %OUTPUT_SHA1%
echo ==============================================

%COMPILER% %CFLAGS% %SOURCES_SHA1% -o %OUTPUT_SHA1%
IF %ERRORLEVEL% NEQ 0 (
    echo Fehler: Kompilierung von test_sha1.c fehlgeschlagen.
    pause
    exit /b %ERRORLEVEL%
)

echo Kompilierung von test_sha1.exe erfolgreich.

REM --- Ausführen von test_sha1.exe ---
echo ==============================================
echo Ausführen von test_sha1.exe
echo ==============================================
%OUTPUT_SHA1%
IF %ERRORLEVEL% NEQ 0 (
    echo Warnung: test_sha1.exe ist mit Fehlercode %ERRORLEVEL% beendet worden.
)
echo.

REM --- Test 2: test_base64.c ---
set "SOURCES_BASE64=%TEST_DIR%\test_base64.c %SRC_DIR%\base64.c"
set "OUTPUT_BASE64=%TEST_DIR%\test_base64.exe"

echo ==============================================
echo Bauvorgang für test_base64.exe
echo Compiler: %COMPILER%
echo Flags: %CFLAGS%
echo Quellen: %SOURCES_BASE64%
echo Ausgabe: %OUTPUT_BASE64%
echo ==============================================

%COMPILER% %CFLAGS% %SOURCES_BASE64% -o %OUTPUT_BASE64%
IF %ERRORLEVEL% NEQ 0 (
    echo Fehler: Kompilierung von test_base64.c fehlgeschlagen.
    pause
    exit /b %ERRORLEVEL%
)

echo Kompilierung von test_base64.exe erfolgreich.

REM --- Ausführen von test_base64.exe ---
echo ==============================================
echo Ausführen von test_base64.exe
echo ==============================================
%OUTPUT_BASE64%
IF %ERRORLEVEL% NEQ 0 (
    echo Warnung: test_base64.exe ist mit Fehlercode %ERRORLEVEL% beendet worden.
)

REM --- Pausieren, um die Ausgabe zu sehen ---
pause
