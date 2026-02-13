#!/bin/bash

## The DESFire stack portion of this firmware source
## is free software written by Dr. Maxie Dion Schmidt (@maxieds):
## You can redistribute it and/or modify
## it under the terms of this license.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
##
## This notice must be retained at the top of all source files where indicated.

READLINK=""
GREP=""
SED=""
if [[ "$(uname -s)" == "Darwin" ]]; then
    # Requires GNU coreutils package: $ brew install coreutils
    READLINK=`which greadlink`
    GREP=`which ggrep`
    SED=`which gsed`
else
    READLINK=`which readlink`
    GREP=`which grep`
    SED=`which sed`
fi

if [ "$#" -ne 1 ]; then
    echo -e "ERROR: Usage: $0 <config-filename.cfg>"
    exit -1
fi

CONFIG_FILE_PATH=`${READLINK} -f $1`
if [ ! -f $CONFIG_FILE_PATH ]; then
    echo -e "ERROR: File \"${CONFIG_FILE_PATH}\" does not exist!"
    exit -2
fi

CUSTOM_CONFIG_MAKEFLAGS=""
CONFIG_SETTING_NAMES=( "UID" "ATS" "ATQA" "ManuID" "HwType" "HwSubtype" "HwProtoType" "HwVers" "SwType" "SwSubtype" "SwProtoType" "SwVers" "BatchNo" "ProdDate" )

function getConfigFileParamRHS() {

    configFilePath=$1
    configSettingName=$2

    # Get the first matching line while ignoring comments:
    settingValue=$(cat $configFilePath | $GREP -Ev "^#|^$" | $GREP "${configSettingName}" | head -n 1)

    # Right-side quote the value (possibly empty) of the RHS of the 'var=val' -> 'var=val"' ...
    # Without doing this, the next call to the `cut` command will only return the RHS value
    # up to the first (white)space character. We need the whole RHS value including spaces
    settingValue=$(echo $settingValue | $SED -e 's/=/=\"/g' | $SED -n -e 's/.*=\"\(.*\)$/\"\1\"/p')

    # Get the full RHS value (unquoted):
    settingValueUnquoted=$(echo $settingValue | cut -f 2 -d '='| cut -f 2 -d '"')
    #settingEvalTextRemoved=$(echo $settingValue | $SED -n -e "s/.*\$\$(\(.*\)).*\$/\1/p");

    echo -n "${settingValueUnquoted}"

}

function formatHexStringToByteArray() {

    byteString=$1
    numBytes=${#byteString}
    if [[ "$(( $numBytes % 2 ))" -ne "0" ]]; then
        byteString="0${byteString}"
        numBytes=$(($numBytes + 1))
    fi
    paramByteArray="0x${byteString:0:2}"
    if [[ "${numBytes}" -gt "2" ]]; then
        for i in $(seq 2 2 $numBytes); do
            paramByteArray="${paramByteArray}, 0x${byteString:$(($i - 2)):2}"
        done
    fi
    echo -n "(uint8_t *) { $paramByteArray }"

}

for configSettingName in "${CONFIG_SETTING_NAMES[@]}"; do    
    configSettingValue=$(getConfigFileParamRHS ${CONFIG_FILE_PATH} ${configSettingName})
    configSettingValueAsByteArray=$(formatHexStringToByteArray ${configSettingValue})
    configSettingNameToUpper=`echo -n ${configSettingName} | tr '[:lower:]' '[:upper:]'`
    CUSTOM_CONFIG_MAKEFLAGS="${CUSTOM_CONFIG_MAKEFLAGS} -DDESFIRE_DEFAULT_PICC_${configSettingNameToUpper}=${configSettingValueAsByteArray}"
done

echo -n $CUSTOM_CONFIG_MAKEFLAGS

exit 0
