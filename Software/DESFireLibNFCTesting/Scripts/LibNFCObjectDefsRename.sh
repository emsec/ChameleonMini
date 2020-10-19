#!/bin/bash

PLATFORM=$(uname -s)
if [[ "$PLATFORM" != "Darwin" ]]; then
	exit 0
fi

OBJCOPY=/usr/local/opt/binutils/bin/objcopy
archiveFile=$(greadlink -f $1)

$OBJCOPY --redefine-sym _nfc_device_pnd_set_property_bool=nfc_device_pnd_set_property_bool $archiveFile

exit 0
