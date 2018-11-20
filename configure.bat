@echo off

if not exist "premake5.exe" (
  powershell -Command "(New-Object Net.WebClient).DownloadFile('https://github.com/premake/premake-core/releases/download/v5.0.0-alpha13/premake-5.0.0-alpha13-windows.zip', 'premake.zip')"

  powershell -Command "Expand-Archive -Path %cd%\premake.zip -DestinationPath %cd% -Force;"
  del /F/Q premake.zip
)

premake5.exe --file=premake.lua vs2017