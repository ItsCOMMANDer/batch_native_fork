@echo off
setlocal enableextensions enabledelayedexpansion

if not exist bin\comptool.exe call :recompileTool
if "%1"=="fullbuild" call :recompileTool

::-msse4.1 -mavx -mmmx -ftree-vectorize -ffast-math -ftree-slp-vectorize -fassociative-math
set optargs=-Isrc -std=gnu99 -m64 -Os -flto -fno-pic -fno-pie -fno-plt -msse -mavx -mmmx -funsafe-math-optimizations -ftree-vectorize -ffast-math -ftree-slp-vectorize -fassociative-math -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -nostartfiles -nodefaultlibs -nostdlib -nolibc -Wl,-s,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -DNDEBUG
gcc -pipe -shared -static -o bin\getinput.dll src\getinput.c %optargs% -luser32 -lkernel32 -lshcore -lgdi32 -lntdll -Wl,-e,DllMain

if "%1"=="fullbuild" (
	set sources= connection_win.cpp discord_register_win.cpp discord_rpc.cpp rpc_connection.cpp serialization.cpp
	g++ -pipe -shared -static-libgcc -static-libstdc++ -static -o bin\discordrpc.dll src\discord.cpp !sources: = src/extern/discord-rpc/src/! -Isrc -Isrc\extern\discord-rpc\include -Isrc\extern\rapidjson\include -mconsole -m64 -Os -flto -fno-pic -fno-pie -fno-plt -fvisibility=hidden -fcompare-debug-second -fno-exceptions -fno-stack-protector -fno-math-errno -fno-ident -fno-asynchronous-unwind-tables -Wl,-s,-e,DllMain,--gc-sections,--reduce-memory-overheads,--no-seh,--disable-reloc-section,--build-id=none -DDISCORD_WINDOWS -DDISCORD_DISABLE_IO_THREAD -DDISCORD_DYNAMIC_LIB -DDISCORD_BUILDING_SDK
)

for %%a in (getinput discordrpc) do (
	strip --strip-unneeded -s -R .comment -R .gnu.version -R .note bin\%%a.dll
	bin\comptool 2 bin\%%a.dll dist\%%a.dll
)

if "%1"=="fullbuild" (
	gcc -pipe -o dist\inject.exe src\hook.c src\extern\chkstk.S -Isrc -Ibin -std=gnu99 -m64 -mwindows %optargs% -Wl,-e,WinMain -lkernel32 -luser32 -lpsapi -lshlwapi -lshell32
	strip --strip-unneeded -s -R .comment -R .gnu.version dist\inject.exe
)

exit /b

:recompileTool
mkdir bin
gcc -pipe -o bin\comptool.exe tools\comptool.c
exit /b
