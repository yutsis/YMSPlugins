call "%VS100COMNTOOLS%vsvars32.bat"

set cygwin=c:\cygwin\bin\
set subpath=Plugins\XMLDOM

for /F "tokens=3" %%a in ('awk "/#define +VERSIONMAJOR +[0-9]+/" version.h') do set vermaj=%%a
for /F "tokens=3" %%a in ('awk "/#define +VERSIONMINOR +[0-9]+/" version.h') do set vermin=%%a
set versfx=%vermaj%%vermin%

:goto files

for %%a in ("Release FAR1|Win32" "Release FAR2|Win32" "Release FAR2|x64" "Release FAR3|Win32" "Release FAR3|x64") do (
	devenv "XMLDOM.sln" /rebuild %%a
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
if errorlevel 1 goto exit
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y XMLDOMeng.hlf %%a\%subpath%\
if errorlevel 1 goto exit
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y License.txt %%a\%subpath%\
if errorlevel 1 goto exit
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y XMLDOMEng.lng %%a\%subpath%\
if errorlevel 1 goto exit
for %%a in ("%FAR3%" "%FAR364%") do xcopy /y Renewal.xml %%a\%subpath%\
if errorlevel 1 goto exit

xcopy /y XMLDOMRus.hlf "%FAR1%\%subpath%"
if errorlevel 1 goto exit
for %%a in ("%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do %cygwin%echo -ne '\xEF\xBB\xBF' > %%a\%subpath%\XMLDOMRus.hlf & %cygwin%iconv -f cp866 -t utf-8 < XMLDOMRus.hlf >> %%a\%subpath%\XMLDOMRus.hlf
if errorlevel 1 goto exit
xcopy /y XMLDOMRus.lng "%FAR1%\%subpath%"
if errorlevel 1 goto exit
for %%a in ("%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do %cygwin%echo -ne '\xEF\xBB\xBF' > %%a\%subpath%\XMLDOMRus.lng & %cygwin%iconv -f cp866 -t utf-8 < XMLDOMRus.lng >> %%a\%subpath%\XMLDOMRus.lng
if errorlevel 1 goto exit

pushd .
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do (
	cd %%a\Plugins
	pkzipc -add XMLDOM%versfx% -dir=specify XMLDOM\XMLDOM.dll XMLDOM\ReadMe.md XMLDOM\License.txt XMLDOM\XMLDOMEng.hlf XMLDOM\XMLDOMRus.hlf XMLDOM\XMLDOMEng.lng XMLDOM\XMLDOMRus.lng XMLDOM\changelog
	)
for %%a in ("%FAR3%" "%FAR364%") do (
	cd %%a\Plugins
	pkzipc -add XMLDOM%versfx% -dir=specify XMLDOM\Renewal.xml
	)
popd

copy "%FAR1%"\Plugins\XMLDOM%versfx%.zip .\XMLDOM%versfx%A.zip
copy "%FAR2%"\Plugins\XMLDOM%versfx%.zip .\XMLDOM%versfx%W2.zip
copy "%FAR3%"\Plugins\XMLDOM%versfx%.zip .\XMLDOM%versfx%W3.zip
copy "%FAR264%"\Plugins\XMLDOM%versfx%.zip .\XMLDOM%versfx%W2x64.zip
copy "%FAR364%"\Plugins\XMLDOM%versfx%.zip .\XMLDOM%versfx%W3x64.zip

:exit
pause
