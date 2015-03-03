@ECHO OFF

REM This script programs the Chameleon using batchisp (Atmel Flip) and DFU (device firmware upgrade via USB)
REM You need to install Flip, otherwise it does not work
REM Make sure to adjust the COMPORT variable to the actual com port the CDC Device enumerates as
REM This is necessary to send the upgrade command and thus set the chameleon into DFU mode

SET COMPORT=COM5
SET TIMEOUT=5
SET COMMAND=UPGRADE

ECHO Trying to set bootloader mode on Chameleon-Mini on %COMPORT%
@MODE %COMPORT% baud=11 parity=n data=8 stop=1 to=off xon=off odsr=off octs=off dtr=off rts=off idsr=off > nul
@ECHO. > \\.\%COMPORT%
@ECHO %COMMAND% > \\.\%COMPORT%

ECHO Waiting for DFU Bootloader
ping 127.0.0.1 -n %TIMEOUT% -w 1000 > nul

ECHO Start Programming
make dfu-flip
