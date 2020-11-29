
echo off

rem ###########################
rem ## Requires git and VS2017
rem ###########################

rem ### 1) Environment

cls
set VS=VS2017.32
set BLD_TYPE=Release
call win32\Clean.bat
if not exist bin32 (
   mkdir bin32
)

rem ### 2) MDDClient

FOR %%X in (libmddWire librtEdge) DO (
   echo Build %%X
   Call :BuildStuff32 %%X
)
cd ..

:: #############################
:: ## Helper Functions
:: #############################
:BuildStuff
   cd %1/%VS%
   del /f /s /q ..\Release ..\lib ..\bin
   set MK=%1%
   devenv /build %BLD_TYPE% %MK%.sln /project %MK%
   cd ..\..
   EXIT /B 0

:BuildStuff32
   cd %1\%VS%
   del /f /s /q ..\Release ..\lib ..\bin
   set MK=%1%
   devenv /build %BLD_TYPE% %MK%32.sln /project %MK%32
   cd ..\..
   EXIT /B 0

:Done
