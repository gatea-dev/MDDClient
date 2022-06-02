
echo off

rem ###########################
rem ## Requires git and VS2017
rem ###########################

rem ### 1) Environment

cls
set VS=VS2017.64
set BLD_TYPE=Release
call win64\Clean.bat
if not exist bin64 (
   mkdir bin64
)

rem ### 2) MDDClient

FOR %%X in (libmddWire librtEdge) DO (
   echo Build %%X
   Call :BuildStuff64 %%X
   FOR %%Y in (Subscribe Publish LVCTest) DO (
      if exist %%X\cpp\%%Y (
         Call :BuildStuff2 %%X %%Y
         cp -v %%X\bin64\%%Y\%%Y.exe .\bin64
      )
   )
   if exist %%X\CLI\lib64 (
      cp -v %%X\CLI\lib64\*.dll .\bin64
   )
   if exist %%X\py\bin64 (
      cp -v %%X\py\bin64\*.pyd ..\bin64
   )
)
goto Done


:: #############################
:: ## Helper Functions
:: #############################
:BuildStuff2
   cd %1/%VS%
   set MK=%1%
   set M2=%2%
   devenv /build %BLD_TYPE% %MK%.sln /project %M2%
   cd ..
   if exist dox\%MK%.dox (
      %DOXYGEN% dox\%MK%.dox
      mv doc\* ..\..\doc
   )
   cd ..
   EXIT /B 0

:BuildStuff64
   cd %1\%VS%
   del /f /s /q ..Release64
   set MK=%1%
   devenv /build %BLD_TYPE% %MK%64.sln /project %MK%64
   IF EXIST %MK%MD64.vcxproj (
      echo %MK%MD64
      del /f /s /q ..\Release64
      devenv /build %BLD_TYPE% %MK%MD64.sln /project %MK%MD64
   )
   cd ..
   if exist CLI\%VS% (
      cd CLI\%VS%
      devenv /build %BLD_TYPE% %MK%CLI64.sln /project %MK%CLI64
      cd ..\..
   )
   if exist py\%VS% (
      cd py\%VS%
      devenv /build Release MDDirect.sln /project MDDirect27
      devenv /build Release MDDirect.sln /project MDDirect39
      cd ..\..
   )
   cd ..
   EXIT /B 0

:Done
