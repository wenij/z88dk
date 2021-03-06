
INCLUDE "config_private.inc"
    
SECTION rodata_common1_data

PUBLIC asci0RxBuffer

asci0RxBuffer:   defs __ASCI0_RX_SIZE   ; Space for the Rx Buffer

; pad to next 256 byte boundary

IF (ASMPC & 0xff)
   defs 256 - (ASMPC & 0xff)
ENDIF

SECTION rodata_common1_data

PUBLIC asci0RxCount, asci0RxIn, asci0RxOut, asci0RxLock
 
asci0RxCount:    defb 0                 ; Space for Rx Buffer Management 
asci0RxIn:       defw asci0RxBuffer     ; non-zero item in bss since it's initialized anyway
asci0RxOut:      defw asci0RxBuffer     ; non-zero item in bss since it's initialized anyway
asci0RxLock:     defb $FE               ; lock flag for Rx exclusion

