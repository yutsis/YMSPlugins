call "%VS100COMNTOOLS%vsvars32.bat"

set cygwin=c:\cygwin\bin\
set subpath=Plugins\json

for /F "tokens=3" %%a in ('awk "/#define +VERSIONMAJOR +[0-9]+/" version.h') do set vermaj=%%a
for /F "tokens=3" %%a in ('awk "/#define +VERSIONMINOR +[0-9]+/" version.h') do set vermin=%%a
set versfx=%vermaj%%vermin%

goto files

for %%a in ("Release FAR1|Win32" "Release FAR2|Win32" "Release FAR2|x64" "Release FAR3|Win32" "Release FAR3|x64") do (
	devenv "json.sln" /rebuild %%a
	if errorlevel 1 goto exit
	)

:files

%cygwin%sed -r -e "s/@platform/ANSI for FAR 1.75/" -e "s/@version/%vermaj%.%vermin%/" <readme.md > "%FAR1%\%subpath%\readme.md"
@if errorlevel 1 goto exit
%cygwin%sed -r -e "s/@platform/Unicode for FAR 2/" -e "s/@version/%vermaj%.%vermin%/" <readme.md > "%FAR2%\%subpath%\readme.md"
@if errorlevel 1 goto exit
%cygwin%sed -r -e "s/@platform/Unicode for FAR 3/" -e "s/@version/%vermaj%.%vermin%/" <readme.md > "%FAR3%\%subpath%\readme.md"
@if errorlevel 1 goto exit
%cygwin%sed -r -e "s/@platform/64-bit Unicode for FAR 2/" -e "s/@version/%vermaj%.%vermin%/" <readme.md > "%FAR264%\%subpath%\readme.md"
@if errorlevel 1 goto exit
%cygwin%sed -r -e "s/@platform/64-bit Unicode for FAR 3/" -e "s/@version/%vermaj%.%vermin%/" <readme.md > "%FAR364%\%subpath%\readme.md"
@if errorlevel 1 goto exit


for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y changelog %%a\%subpath%\
@if errorlevel 1 goto exit
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y jsonEng.hlf %%a\%subpath%\
@if errorlevel 1 goto exit
for %%a in ("%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y jsonRus.hlf %%a\%subpath%\
@if errorlevel 1 goto exit
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y license.txt %%a\%subpath%\
@if errorlevel 1 goto exit
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y jsonEng.lng %%a\%subpath%\
@if errorlevel 1 goto exit
for %%a in ("%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y jsonRus.lng %%a\%subpath%\
@if errorlevel 1 goto exit
for %%a in ("%FAR3%" "%FAR364%") do xcopy /y Renewal.xml %%a\%subpath%\
@if errorlevel 1 goto exit

for %%f in (jsonRus.hlf jsonRus.lng) do (
       %cygwin%iconv -f utf-8 -t cp866 -c < %%f > "%FAR1%\%subpath%\%%f"
       :echo %cygwin%iconv -f utf-8 -t cp866 -c %%f
       @if errorlevel 1 goto exit
)

@pushd .
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do (
	cd %%a\Plugins
        del json%versfx%
	pkzipc -add json%versfx% -dir=specify json\json.dll json\ReadMe.md json\license.txt json\jsonEng.hlf json\jsonRus.hlf json\jsonEng.lng json\jsonRus.lng json\changelog
	)
for %%a in ("%FAR3%" "%FAR364%") do (
	cd %%a\Plugins
	pkzipc -add json%versfx% -dir=specify json\Renewal.xml
	)
@popd

copy "%FAR1%"\Plugins\json%versfx%.zip .\json%versfx%A.zip
copy "%FAR2%"\Plugins\json%versfx%.zip .\json%versfx%W2.zip
copy "%FAR3%"\Plugins\json%versfx%.zip .\json%versfx%W3.zip
copy "%FAR264%"\Plugins\json%versfx%.zip .\json%versfx%W2x64.zip
copy "%FAR364%"\Plugins\json%versfx%.zip .\json%versfx%W3x64.zip

:exit
pause
