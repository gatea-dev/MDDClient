
echo off

:: ###########################
:: ## Requires unix tools
:: ###########################

:: ### 1) Binaries

cls
del /f /s /q bin64

:: ### 2) MDDClient

FOR %%X in (libmddWire librtEdge) DO (
   echo Clean %%X
   Call :CleanStuff %%X
)
goto Done


:: #############################
:: ## Helper Functions
:: #############################
:CleanStuff
   cd %1
   del /f /s /q Release64 lib64 bin64
   if exist CLI (
      cd CLI
      del /f /s /q Release64 lib64 bin64
      cd ..
   )
   cd ../
   EXIT /B 0
:Done
