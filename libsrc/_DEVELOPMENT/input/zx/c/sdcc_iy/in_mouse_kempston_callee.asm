
; void in_mouse_kempston_callee(uint8_t *buttons, uint16_t *x, uint16_t *y)

SECTION code_clib
SECTION code_input

PUBLIC _in_mouse_kempston_callee

EXTERN asm_in_mouse_kempston

_in_mouse_kempston_callee:

   call asm_in_mouse_kempston

   pop ix
      
   pop hl
   ld (hl),a
   
   pop hl
   ld (hl),e
   inc hl
   ld (hl),d
   
   pop hl
   ld (hl),c
   inc hl
   ld (hl),b

   jp (ix)
