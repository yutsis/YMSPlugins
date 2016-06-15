call "%VS100COMNTOOLS%vsvars32.bat"

set cygwin=c:\cygwin\bin\
set subpath=Plugins\ClipSel
set zipcmd=c:\ut\7z a -r

for /F "tokens=3" %%a in ('awk "/#define +VERSIONMAJOR +[0-9]+/" version.h') do set vermaj=%%a
for /F "tokens=3" %%a in ('awk "/#define +VERSIONMINOR +[0-9]+/" version.h') do set vermin=%%a
set versfx=%vermaj%%vermin%

:goto files

for %%a in ("Release FAR1|Win32" "Release FAR2|Win32" "Release FAR2|x64" "Release FAR3|Win32" "Release FAR3|x64") do (
	devenv "ClipSel.sln" /rebuild %%a
	if errorlevel 1 goto exit
	)

:files

%cygwin%sed -r -e "s/@platform/ANSI for FAR 1.75/" -e "s/@version/%vermaj%.%vermin%/" <readme.md > "%FAR1%\%subpath%\readme.md"
if errorlevel 1 goto exit

%cygwin%sed -r -e "s/@platform/ANSI for FAR 1.75/" -e "s/@version/%vermaj%.%vermin%/" <readme-ru.md | %cygwin%iconv -c -t cp866 -f utf-8  > "%FAR1%\%subpath%\readme-ru.md"
if errorlevel 2 goto exit

for %%a in (readme.md readme-ru.md) do (
  %cygwin%sed -r -e "s/@platform/Unicode for FAR2/" -e "s/@version/%vermaj%.%vermin%/" <%%a > "%FAR2%\%subpath%\%%a"
  if errorlevel 1 goto exit
  %cygwin%sed -r -e "s/@platform/Unicode for FAR3/" -e "s/@version/%vermaj%.%vermin%/" <%%a > "%FAR3%\%subpath%\%%a"
  if errorlevel 1 goto exit
  %cygwin%sed -r -e "s/@platform/64-bit Unicode for FAR2/" -e "s/@version/%vermaj%.%vermin%/" <%%a > "%FAR264%\%subpath%\%%a"
  if errorlevel 1 goto exit
  %cygwin%sed -r -e "s/@platform/64-bit Unicode for FAR3/" -e "s/@version/%vermaj%.%vermin%/" <%%a > "%FAR364%\%subpath%\%%a"
  if errorlevel 1 goto exit
)



for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y changelog %%a\%subpath%\
if errorlevel 1 goto exit
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y License.txt %%a\%subpath%\
if errorlevel 1 goto exit
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do xcopy /y ClipSelEng.lng %%a\%subpath%\
if errorlevel 1 goto exit
for %%a in ("%FAR3%" "%FAR364%") do xcopy /y Renewal.xml %%a\%subpath%\
if errorlevel 1 goto exit

xcopy /y ClipSelRus.lng "%FAR1%\%subpath%"
if errorlevel 1 goto exit
for %%a in ("%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do %cygwin%echo -ne '\xEF\xBB\xBF' > %%a\%subpath%\ClipSelRus.lng & %cygwin%iconv -f cp866 -t utf-8 < ClipSelRus.lng >> %%a\%subpath%\ClipSelRus.lng
if errorlevel 1 goto exit

set zipname=ClipSel%versfx%.zip

pushd .
for %%a in ("%FAR1%" "%FAR2%" "%FAR3%" "%FAR264%" "%FAR364%") do (
	cd %%a\Plugins
        del %zipname%
	%zipcmd% %zipname% ClipSel\ClipSel.dll ClipSel\readme.md ClipSel\ClipSelEng.lng ClipSel\ClipSelRus.lng ClipSel\readme-ru.md ClipSel\changelog ClipSel\License.txt
	)
for %%a in ("%FAR3%" "%FAR364%") do (
	cd %%a\Plugins
	%zipcmd% %zipname% ClipSel\Renewal.xml
	)
popd

copy "%FAR1%"\Plugins\ClipSel%versfx%.zip .\ClipSel%versfx%A.zip
copy "%FAR2%"\Plugins\ClipSel%versfx%.zip .\ClipSel%versfx%W2.zip
copy "%FAR3%"\Plugins\ClipSel%versfx%.zip .\ClipSel%versfx%W3.zip
copy "%FAR264%"\Plugins\ClipSel%versfx%.zip .\ClipSel%versfx%W2x64.zip
copy "%FAR364%"\Plugins\ClipSel%versfx%.zip .\ClipSel%versfx%W3x64.zip

:exit
pause
