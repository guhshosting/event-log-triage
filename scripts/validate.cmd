@echo off
rem Run in an x64 MSVC developer command prompt. No events are generated.
setlocal
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
if errorlevel 1 exit /b 1
cmake --build build-debug
if errorlevel 1 exit /b 1
ctest --test-dir build-debug --output-on-failure
if errorlevel 1 exit /b 1
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DTRIAGE_ANALYZE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
if errorlevel 1 exit /b 1
cmake --build build-release
if errorlevel 1 exit /b 1
ctest --test-dir build-release --output-on-failure
if errorlevel 1 exit /b 1
cmake -S . -B build-independent -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build-independent
if errorlevel 1 exit /b 1
ctest --test-dir build-independent --output-on-failure
if errorlevel 1 exit /b 1
cmake --install build-independent --prefix install
if errorlevel 1 exit /b 1
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTRIAGE_ASAN=ON
if errorlevel 1 exit /b 1
cmake --build build-asan
if errorlevel 1 exit /b 1
rem The MSVC ASan runtime must be beside executables or on the loader search path.
copy /y "%VCToolsInstallDir%bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll" build-asan\ >nul
if errorlevel 1 exit /b 1
ctest --test-dir build-asan --output-on-failure
if errorlevel 1 exit /b 1
node scripts\validate-json.cjs build-release
if errorlevel 1 exit /b 1
dumpbin /imports build-release\event-log-triage.exe > artifacts\imports.txt
if errorlevel 1 exit /b 1
node scripts\audit.cjs
if errorlevel 1 exit /b 1
cppcheck --enable=warning,performance,portability --std=c++17 --platform=win64 --error-exitcode=1 --inline-suppr -Isrc src tests
if errorlevel 1 exit /b 1
clang-tidy -p build-debug src/acquisition.cpp src/cli.cpp src/correlation.cpp src/grouping.cpp src/help.cpp src/human_formatter.cpp src/json_formatter.cpp src/main.cpp src/message_formatter.cpp src/query.cpp src/unicode_time.cpp src/xml_parser.cpp tests/tests.cpp tests/nonadmin.cpp --extra-arg=/EHsc
if errorlevel 1 exit /b 1
echo Validation passed.
