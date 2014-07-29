@echo on
echo Delete visual studio intermediate files
echo Begin deleting...
for /r . %%c in (.) do @if exist "%%c\Debug" rd /S /Q "%%c\Debug"
for /r . %%c in (.) do @if exist "%%c\Release" rd /S /Q "%%c\Release"
for /r . %%c in (.) do @if exist "%%c\x64" rd /S /Q "%%c\x64"
for /r . %%c in (.) do @if exist "%%c\GeneratedFiles" rd /S /Q "%%c\GeneratedFiles"
for /r . %%c in (*.bsc *.aps *.clw *.ncb *.plg *.positions *.WW *.user *.sdf *.opensdf *Log.txt *.aux *.bbl *.blg *.brf *.log *.synctex) do del "%%c"
del /q /A:H *.suo
rmdir /s /q .\ipch
echo End deleting...
pause