@ECHO OFF

if not exist "%~dp0dfu-programmer.exe" (
	echo Cannot find dfu-programmer.exe. Please run this .bat script in the same directory where dfu-programmer.exe is saved.
	pause > nul
	exit
)

if not exist "%~dp0Chameleon-Mini.eep" (
	echo Cannot find Chameleon-Mini.eep. Please run this .bat script in the same directory where Chameleon-Mini.eep and Chameleon-Mini.hex are saved.
	pause > nul
	exit
)

if not exist "%~dp0Chameleon-Mini.hex" (
	echo Cannot find Chameleon-Mini.hex. Please run this .bat script in the same directory where Chameleon-Mini.eep and Chameleon-Mini.hex are saved.
	pause > nul
	exit
)

"%~dp0dfu-programmer.exe" atxmega128a4u erase

IF %ERRORLEVEL% NEQ 0 (
	echo There was an error with executing this command. Maybe your ChameleonMini is not in bootloader mode?
	pause > nul
	exit
)

"%~dp0dfu-programmer.exe" atxmega128a4u flash-eeprom "%~dp0Chameleon-Mini.eep" --force

IF %ERRORLEVEL% NEQ 0 (
	echo There was an error with executing this command. Maybe your ChameleonMini is not in bootloader mode?
	pause > nul
	exit
)

"%~dp0dfu-programmer.exe" atxmega128a4u flash "%~dp0Chameleon-Mini.hex"

IF %ERRORLEVEL% NEQ 0 (
	echo There was an error with executing this command. Maybe your ChameleonMini is not in bootloader mode?
	pause > nul
	exit
)

"%~dp0dfu-programmer.exe" atxmega128a4u launch

echo Flashing the firmware to your ChameleonMini is finished now.
pause > nul