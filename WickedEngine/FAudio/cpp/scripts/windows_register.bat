@echo off

CALL :register XAudio2_0.dll
CALL :register XAudio2_1.dll
CALL :register XAudio2_2.dll
CALL :register XAudio2_3.dll
CALL :register XAudio2_4.dll
CALL :register XAudio2_5.dll
CALL :register XAudio2_6.dll
CALL :register XAudio2_7.dll
CALL :register XACTEngine3_0.dll
CALL :register XACTEngine3_1.dll
CALL :register XACTEngine3_2.dll
CALL :register XACTEngine3_3.dll
CALL :register XACTEngine3_4.dll
CALL :register XACTEngine3_5.dll
CALL :register XACTEngine3_6.dll
CALL :register XACTEngine3_7.dll

EXIT /B %ERRORLEVEL%

:: a function to register a DLL
:register
regsvr32 /s "%CD%"\%* && goto :register_ok
echo %* failed
goto :register_end
:register_ok
echo %* OK
:register_end
EXIT /B 0
