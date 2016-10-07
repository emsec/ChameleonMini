#include <avr/io.h>
#include "Memory.h"

#define NVM_CMD_NO_OPERATION_gc (0x00<<0)
#define NVM_CMD_READ_USER_SIG_ROW_gc (0x01<<0)
#define NVM_CMD_READ_CALIB_ROW_gc (0x02<<0)
#define NVM_CMD_READ_EEPROM_gc (0x06<<0)
#define NVM_CMD_READ_FUSES_gc (0x07<<0)
#define NVM_CMD_WRITE_LOCK_BITS_gc (0x08<<0)
#define NVM_CMD_ERASE_USER_SIG_ROW_gc (0x18<<0)
#define NVM_CMD_WRITE_USER_SIG_ROW_gc (0x1A<<0)
#define NVM_CMD_ERASE_APP_gc (0x20<<0)
#define NVM_CMD_ERASE_APP_PAGE_gc (0x22<<0)
#define NVM_CMD_LOAD_FLASH_BUFFER_gc (0x23<<0)
#define NVM_CMD_WRITE_APP_PAGE_gc (0x24<<0)
#define NVM_CMD_ERASE_WRITE_APP_PAGE_gc (0x25<<0)
#define NVM_CMD_ERASE_FLASH_BUFFER_gc (0x26<<0)
#define NVM_CMD_ERASE_BOOT_PAGE_gc (0x2A<<0)
#define NVM_CMD_WRITE_BOOT_PAGE_gc (0x2C<<0)
#define NVM_CMD_ERASE_WRITE_BOOT_PAGE_gc (0x2D<<0)
#define NVM_CMD_ERASE_EEPROM_gc (0x30<<0)
#define NVM_CMD_ERASE_EEPROM_PAGE_gc (0x32<<0)
#define NVM_CMD_LOAD_EEPROM_BUFFER_gc (0x33<<0)
#define NVM_CMD_WRITE_EEPROM_PAGE_gc (0x34<<0)
#define NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc (0x35<<0)
#define NVM_CMD_ERASE_EEPROM_BUFFER_gc (0x36<<0)
#define NVM_CMD_APP_CRC_gc (0x38<<0)
#define NVM_CMD_BOOT_CRC_gc (0x39<<0)
#define NVM_CMD_FLASH_RANGE_CRC_gc (0x3A<<0)
#define CCP_SPM_gc (0x9D<<0)
#define CCP_IOREG_gc (0xD8<<0)

/* FlashCommonSPM needs to reside in boot loader section */
#define SPM_HELPER_ADDR_REF		(BOOT_SECTION_START + BOOT_SECTION_SIZE - 32)
#ifdef SPM_HELPER_ADDR
/* If enabled by makefile, we can check the hardcoded address from makefile
 * against avr/io.h and generate an error on mismatch */
#if ((SPM_HELPER_ADDR - SPM_HELPER_ADDR_REF) != 0)
#error SPM helper function addresses from Makefile and avr/io.h do not match!
#endif
#endif

/* The following assembly code is taken from Atmels sp_driver.S in AVR1316 */

; This routine reads a word from flash given by the address in
; R25:R24:R23:R22.
;
; Input:
;     R25:R24:R23:R22.
;
; Returns:
;     R25:R24 - Read word.
.section .text
.global FlashReadWord
FlashReadWord:
	in	r19, RAMPZ      ; Save RAMPZ.
	out	RAMPZ, r24      ; Load RAMPZ with the MSB of the address.
	movw	ZL, r22         ; Move the low bytes to the Z pointer
	elpm	r24, Z+         ; Extended load byte from address pointed to by Z.
	elpm	r25, Z          ; Extended load byte from address pointed to by Z.
	out	RAMPZ, r19      ; Restore RAMPZ register.
	ret

; This routine erases the page at address R25:R24:R23:R22 in the application
; section. The address can point anywhere inside the page.
;
; Input:
;     R25:R24:R23:R22 - Byte address into Flash page.
;
; Returns:
;     Nothing.
.section .text
.global FlashEraseApplicationPage
FlashEraseApplicationPage:
	in	r19, RAMPZ                      ; Save RAMPZ, which is restored in FlashCommonSPM.
	out	RAMPZ, r24                      ; Load RAMPZ with the MSB of the address.
	movw    r24, r22                        ; Move low bytes for ZH:ZL to R25:R24
	ldi	r20, NVM_CMD_ERASE_APP_PAGE_gc  ; Prepare NVM command in R20.
	jmp	FlashCommonSPM                    ; Jump to common SPM code.

; This routine writes the word from R23:R22 into the Flash page buffer at
; address R25:R24.
;
; Input:
;     R25:R24 - Byte address into Flash page.
;     R23:R22 - Word to write.
;
; Returns:
;     Nothing.
.section .text
.global FlashLoadFlashWord
FlashLoadFlashWord:
	in	r19, RAMPZ                         ; Save RAMPZ, which is restored in FlashCommonSPM.
	movw	r0, r22                            ; Prepare flash word in R1:R0.
	ldi	r20, NVM_CMD_LOAD_FLASH_BUFFER_gc  ; Prepare NVM command in R20.
	jmp	FlashCommonSPM                       ; Jump to common SPM code.


; This routine erases first and then writes the page buffer to the
; Flash page at address R25:R24:R23:R22 in the application section. The address
; can point anywhere inside the page.
;
; Input:
;     R25:R24:R23:R22 - Byte address into Flash page.
;
; Returns:
;     Nothing.
.section .text
.global FlashEraseWriteApplicationPage
FlashEraseWriteApplicationPage:
	in	r19, RAMPZ                            ; Save RAMPZ, which is restored in FlashCommonSPM.
	out	RAMPZ, r24                            ; Load RAMPZ with the MSB of the address.
	movw	r24, r22                              ; Move low bytes of address to ZH:ZL from R23:R22
	ldi	r20, NVM_CMD_ERASE_WRITE_APP_PAGE_gc  ; Prepare NVM command in R20.
	jmp	FlashCommonSPM                          ; Jump to common SPM code.

; This routine flushes the Flash page buffer.
;
; Input:
;     Nothing.
;
; Returns:
;     Nothing.
.section .text
.global FlashEraseFlashBuffer
FlashEraseFlashBuffer:
	in	r19, RAMPZ                          ; Save RAMPZ, which is restored in FlashCommonSPM.
	ldi	r20, NVM_CMD_ERASE_FLASH_BUFFER_gc  ; Prepare NVM command in R20.
	;jmp	FlashCommonSPM                        ; Jump to common SPM code.
	jmp	FlashCommonCMD

; This routine wait for the SPM to finish and clears the command register.
;
; Note that this routine is blocking, and will halt any execution until the SPM
; is finished.
;
; Input:
;     Nothing.
;
; Returns:
;     Nothing.
.section .text
.global FlashWaitForSPM
FlashWaitForSPM:
	lds	r18, NVM_STATUS     ; Load the NVM Status register.
	sbrc	r18, NVM_NVMBUSY_bp ; Check if bit is cleared.
	rjmp	FlashWaitForSPM       ; Repeat check if bit is not cleared.
	clr	r18
	sts	NVM_CMD, r18        ; Clear up command register to NO_OPERATION.
	ret

; This routine is called by several other routines, and contains common code
; for executing an NVM command, including the return statement itself.
;
; If the operation (NVM command) requires the NVM Address registers to be
; prepared, this must be done before jumping to this routine.
;
; Note that R25:R24:R23:R22 is used for returning results, even if the
; C-domain calling function only expects a single byte or even void.
;
; Input:
;     R20 - NVM Command code.
;
; Returns:
;     R25:R24:R23:R22 - 32-bit result from NVM operation.
.section .text
FlashCommonCMD:
	sts	NVM_CMD, r20        ; Load command into NVM Command register.
	ldi	r18, CCP_IOREG_gc   ; Prepare Protect IO-register signature in R18.
	ldi	r19, NVM_CMDEX_bm   ; Prepare bitmask for setting NVM Command Execute bit into R19.
	sts	CCP, r18            ; Enable IO-register operation (this disables interrupts for 4 cycles).
	sts	NVM_CTRLA, r19      ; Load bitmask into NVM Control Register A, which executes the command.
	lds	r22, NVM_DATA0      ; Load NVM Data Register 0 into R22.
	lds	r23, NVM_DATA1      ; Load NVM Data Register 1 into R23.
	lds	r24, NVM_DATA2      ; Load NVM Data Register 2 into R24.
	clr	r25                 ; Clear R25 in order to return a clean 32-bit value.
	ret

; This routine is called by several other routines, and contains common code
; for executing an LPM command, including the return statement itself.
;
; Note that R24 is used for returning results, even if the
; C-domain calling function expects a void.
;
; Input:
;     R25:R24 - Low bytes of Z pointer.
;     R20     - NVM Command code.
;
; Returns:
;     R24     - Result from LPM operation.
.section .text
FlashCommonLPM:
	movw	ZL, r24             ; Load index into Z.
	sts	NVM_CMD, r20        ; Load prepared command into NVM Command register.
	lpm	r24,Z
	ret

; This routine is called by several other routines, and contains common code
; for executing an SPM command, including the return statement itself.
;
; If the operation (SPM command) requires the R1:R0 registers to be
; prepared, this must be done before jumping to this routine.
;
; Input:
;     R1:R0    - Optional input to SPM command.
;     R25:R24  - Low bytes of Z pointer.
;     R20      - NVM Command code.
;
; Returns:
;     Nothing.
.section .spmhelper
FlashCommonSPM:
	movw	ZL, r24          ; Load R25:R24 into Z.
	sts	NVM_CMD, r20     ; Load prepared command into NVM Command register.
	ldi	r18, CCP_SPM_gc  ; Prepare Protect SPM signature in R18
	sts	CCP, r18         ; Enable SPM operation (this disables interrupts for 4 cycles).
	spm                      ; Self-program.
	clr	r1               ; Clear R1 for GCC _zero_reg_ to function properly.
	out	RAMPZ, r19       ; Restore RAMPZ register.
	ret

/* Reserve memory for data */
.section .flashdata
.align 1
.skip MEMORY_SIZE, MEMORY_INIT_VALUE
