
echo off

:: ###########################
:: ## Requires unix tools
:: ###########################

:: ### 1) Binaries

cls
del /f /s /q bin32

:: ### 2) MDDClient

FOR %%X in (libmddWire librtEdge) DO (
   echo Clean %%X
   Call :CleanStuff %%X
)
cd ..

:: #############################
:: ## Helper Functions
:: #############################
:CleanStuff
   cd %1
   del /f /s /q Release lib
   cd ../
   EXIT /B 0
:Done
