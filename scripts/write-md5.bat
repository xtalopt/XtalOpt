@echo OFF & setlocal EnableDelayedExpansion

REM Windows batch files take a lot of code to do simple things

REM Find the package name and set it in a variable
for /r %%i in (.\*) do (
  if /i [%i:~-10%]==[-win32.exe] (
    set @packagename=%%i
    goto :break
  )
)

:break

REM Calculate and write the md5
CertUtil -hashfile %packagename% MD5 > windows-xtalopt.md5

REM Remove the first and last line of the file
set row=
for /f "skip=1 delims=*" %%a in (windows-xtalopt.md5) do (
  if defined row echo.!row! >> tmpFile.txt
  set row=%%a
)
xcopy tmpFile.txt windows-xtalopt.md5 /y
del tmpFile.txt /f /q
