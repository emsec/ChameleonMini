#### Makefile for the libnfc-based tests in C
#### These tests are compiled for the local host system, not for AVR platforms

#CC=gcc-10   # for MacOS
CC=gcc
CFLAGS_BASE= -ILocalInclude -ISource \
		-g -O0 -Wall -pedantic -Wextra -std=c99 -Du_int8_t=uint8_t -Du_int16_t=uint16_t \
		`pkg-config libnfc --cflags` -I/usr/local/opt/openssl/include                   \
		-DHOST_BUILD -DCRYPTO_AES_DEFAULT=1
#LD=gcc-10   # for MacOS
LD=gcc
LDFLAGS_BASE= $(CFLAGS) -lc `pkg-config libnfc --libs` \
		 -L/usr/local/opt/openssl/lib -L/usr/lib64 -lssl -lcrypto

ifeq ("$(shell uname -s)", "Darwin")
    CC=/usr/local/opt/llvm/bin/clang-13
    CFLAGS=$(CFLAGS_BASE) -I/usr/local/opt/llvm/include
    LD=/usr/local/opt/llvm/bin/clang-13
    LDFLAGS=$(LDFLAGS_BASE) -L/usr/local/opt/llvm/lib
else
    CFLAGS=$(CFLAGS_BASE)
    LDFLAGS=$(LDFLAGS_BASE)
endif

BINDIR=./Bin
BINEXT=exe
OBJDIR=./Obj
OBJEXT=o

LIBNFC_SUPPORTED_DRIVERS= -DDRIVER_PN53X_USB_ENABLED                   \
					 -DDRIVER_PCSC_ENABLED         \
					 -DDRIVER_ACR122_PCSC_ENABLED  \
					 -DDRIVER_ACR122_USB_ENABLED   \
					 -DDRIVER_ACR122S_ENABLED      \
					 -DDRIVER_ARYGON_ENABLED       \
					 -DDRIVER_PN532_UART_ENABLED   \
					 -DDRIVER_PN532_SPI_ENABLED    \
					 -DDRIVER_PN532_I2C_ENABLED    \
					 -DDRIVER_PN71XX_ENABLED
LIBNFC_CUSTOM_CONFIG= -DLIBNFC_LOGLEVEL=NFC_LOG_PRIORITY_DEBUG                                    \
				  -ULOG_CATEGORY -DLOG_CATEGORY=\"DESFireTesting.libnfc.general\" \
				  -ULOG_GROUP -DLOG_GROUP=NFC_LOG_GROUP_GENERAL
CFLAGS+= $(LIBNFC_SUPPORTED_DRIVERS) $(LIBNFC_CUSTOM_CONFIG)

UTILS_SOURCE=LocalInclude/Config.h           \
	     LocalInclude/CryptoUtils.h      \
	     LocalInclude/DesfireUtils.h     \
	     LocalInclude/ErrorHandling.h    \
	     LocalInclude/GeneralUtils.h     \
	     LocalInclude/LibNFCUtils.h      \
	     LocalInclude/LibNFCWrapper.h

FILE_BASENAMES=NFCAntiCollisionMod                              \
			   TestAuthenticateLegacy               \
			   TestAuthenticateISO                  \
			   TestGeneralCommands                  \
			   TestGeneralCommands2                 \
			   TestKeyManagementCommands            \
			   TestApplicationManagementCommands    \
			   TestApplicationManagementCommands2   \
			   TestFileManagementCommands           \
			   TestDataManipulationCommands         \
			   TestDataManipulationCommands2        \
			   TestDataManipulationCommands3

OBJFILES=$(addprefix $(OBJDIR)/, $(addsuffix .$(OBJEXT), $(basename $(FILE_BASENAMES))))
BINOUTS=$(addprefix $(BINDIR)/, $(addsuffix .$(BINEXT), $(basename $(FILE_BASENAMES))))

.SECONDARY: $(OBJFILES)
.PRECIOUS: $(OBJFILES)

all: default

default: prelims $(OBJFILES) $(BINOUTS)

$(OBJDIR)/%.$(OBJEXT): Source/%.c $(UTILS_SOURCE)
	$(CC) $(CFLAGS) $< -c -o $@
	@/bin/bash ./Scripts/LibNFCObjectDefsRename.sh $@

$(BINDIR)/%.$(BINEXT): $(OBJDIR)/%.$(OBJEXT)
	$(LD) $< -o $@ $(LDFLAGS)

prelims:
	@mkdir -p ./Obj ./Bin

clean:
	@rm -f $(OBJDIR)/* $(BINDIR)/* *.code

style:
	# Make sure astyle is installed
	@which astyle >/dev/null || ( echo "Please install 'astyle' package first" ; exit 1 )
	# Remove spaces & tabs at EOL, add LF at EOF if needed on *.c, *.h, Makefile
	find . \( -name "*.[ch]" -or -name "Makefile" \) \
	    -exec perl -pi -e 's/[ \t]+$$//' {} \; \
	    -exec sh -c "tail -c1 {} | xxd -p | tail -1 | grep -q -v 0a$$" \; \
	    -exec sh -c "echo >> {}" \;
	# Apply astyle on *.c, *.h
	find . -name "*.[ch]" -exec astyle --formatted --mode=c --suffix=none \
	    --indent=spaces=4 --indent-switches \
	    --keep-one-line-blocks --max-instatement-indent=60 \
	    --style=google --pad-oper --unpad-paren --pad-header \
	    --align-pointer=name {} \;
