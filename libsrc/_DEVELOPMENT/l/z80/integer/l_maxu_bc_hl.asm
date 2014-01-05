
XLIB l_maxu_bc_hl

l_maxu_bc_hl:

   ; return unsigned maximum of bc and hl
   ;
   ; enter : hl = unsigned 16 bit number
   ;         bc = unsigned 16 bit number
   ;
   ; exit  : hl = larger of the two unsigned numbers
   ;         bc = unchanged
   ;         carry set if hl was larger
   ;
   ; uses  : af, hl

   ld a,b
   cp h
   ret c
   
   jr nz, bc_larger
   
   ld a,c
   cp l
   ret c

bc_larger:

   ld l,c
   ld h,b

   ret
