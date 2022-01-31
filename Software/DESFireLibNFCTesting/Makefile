#### Makefile for the libnfc-based tests in C
#### These tests are compiled for the local host system, not for AVR platforms

#CC=gcc-10 # for MacOS
CC=gcc
CFLAGS= -ILocalInclude -ISource \
		-g -O0 -Wall -pedantic -Wextra -std=c99 -Du_int8_t=uint8_t -Du_int16_t=uint16_t \
		`pkg-config libnfc --cflags` -I/usr/local/opt/openssl/include \
		-DHOST_BUILD -DCRYPTO_AES_DEFAULT=1 \
		-ILibs/ArduinoCryptoLib
#LD=gcc-10
LD=gcc
LDFLAGS= $(CFLAGS) -lc `pkg-config libnfc --libs` \
		 -L/usr/local/opt/openssl/lib -lssl -lcrypto

BINDIR=./Bin
BINEXT=exe
OBJDIR=./Obj
OBJEXT=o

LIBNFC_SUPPORTED_DRIVERS= -DDRIVER_PN53X_USB_ENABLED \
					 -DDRIVER_PCSC_ENABLED \
					 -DDRIVER_ACR122_PCSC_ENABLED \
					 -DDRIVER_ACR122_USB_ENABLED \
					 -DDRIVER_ACR122S_ENABLED \
					 -DDRIVER_ARYGON_ENABLED \
					 -DDRIVER_PN532_UART_ENABLED \
					 -DDRIVER_PN532_SPI_ENABLED \
					 -DDRIVER_PN532_I2C_ENABLED \
					 -DDRIVER_PN71XX_ENABLED
LIBNFC_CUSTOM_CONFIG= -DLIBNFC_LOGLEVEL=NFC_LOG_PRIORITY_DEBUG \
				  -ULOG_CATEGORY -DLOG_CATEGORY=\"DESFireTesting.libnfc.general\" \
				  -ULOG_GROUP -DLOG_GROUP=NFC_LOG_GROUP_GENERAL
CFLAGS+= $(LIBNFC_SUPPORTED_DRIVERS) $(LIBNFC_CUSTOM_CONFIG)

FILE_BASENAMES=NFCAntiCollisionMod \
			   TestAuthenticateAES128 \
			   TestGeneralCommands \
			   TestKeyManagementCommands \
			   TestApplicationManagementCommands \
			   TestFileManagementCommandsSupport \
			   TestDataManipulationCommandsSupport \
			   TestDataManipulationCommandsSupport2 \
			   TestDataManipulationCommandsSupport3

OBJFILES=$(addprefix $(OBJDIR)/, $(addsuffix .$(OBJEXT), $(basename $(FILE_BASENAMES))))
BINOUTS=$(addprefix $(BINDIR)/, $(addsuffix .$(BINEXT), $(basename $(FILE_BASENAMES))))

.SECONDARY: $(OBJFILES)
.PRECIOUS: $(OBJFILES)

all: prelims clean default

default: $(OBJFILES) $(BINOUTS)

$(OBJDIR)/%.$(OBJEXT): Source/%.c
	$(CC) $(CFLAGS) $< -c -o $@
	@/bin/bash ./Scripts/LibNFCObjectDefsRename.sh $@

$(BINDIR)/%.$(BINEXT): $(OBJDIR)/%.$(OBJEXT)
	$(LD) $(LDFLAGS) $< -o $@

prelims:
	@mkdir -p ./Obj ./Bin

clean:
	@rm -f $(OBJDIR)/* $(BINDIR)/*

