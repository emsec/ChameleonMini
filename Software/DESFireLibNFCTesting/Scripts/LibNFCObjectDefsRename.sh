#!/bin/bash

PLATFORM=$(uname -s)
if [[ "$PLATFORM" != "Darwin" ]]; then
	exit 0
fi

# NOTE: Run ``brew install coreutils binutils``
OBJCOPY=/usr/local/opt/binutils/bin/objcopy
#OBJCOPY=$(which objcopy)
archiveFile=$(greadlink -f $1)

$OBJCOPY --redefine-sym _nfc_device_pnd_set_property_bool=nfc_device_pnd_set_property_bool $archiveFile

exit 0
