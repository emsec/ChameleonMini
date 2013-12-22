@ECHO OFF
SET COMPORT=COM21
SET BATCHISP=C:\Programme\Atmel\Flip 3.4.7\bin
SET TIMEOUT=5
SET COMMAND=UPGRADE

ECHO Trying to set bootloader mode on Chameleon-Mini on %COMPORT%
@MODE %COMPORT% baud=11 parity=n data=8 stop=1 to=off xon=off odsr=off octs=off dtr=off rts=off idsr=off > nul
@ECHO. > \\.\%COMPORT%
@ECHO %COMMAND% > \\.\%COMPORT%

ECHO Waiting for DFU Bootloader
ping 127.0.0.1 -n %TIMEOUT% -w 1000 > nul

ECHO Start Programming
SET PATH=%BATCHISP%;%PATH%
make dfu-flip
