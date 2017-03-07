@ECHO OFF

TITLE NyaSama Assembly Script Module

IF NOT EXIST nsasm.exe (
  CLS
  ECHO nsasm.exe could not be found.
  GOTO:error
)

PROMPT $P$_$G
SET PATH=%CD%;%PATH%
CLS
nsasm.exe
ECHO Current directory is now: "%CD%"
ECHO The bin directory has been added to PATH
ECHO.

CMD /Q /K 
GOTO:EOF

:error
ECHO.
ECHO Press any key to exit.
PAUSE >nul
GOTO:EOF
