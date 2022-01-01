SETLOCAL
SET MYDIR=%~dp0
@ECHO %MYDIR%>"C:\Download\debug.txt"
FOR /F "tokens=*" %%G IN ('wsl wslpath -a ^"%MYDIR%\^"') DO SET MYDIRLNX=%%G
ECHO %MYDIRLNX%
@ECHO %MYDIRLNX%>>"C:\Download\debug.txt"
ENDLOCAL