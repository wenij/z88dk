
SECTION code_clib
SECTION code_l_sdcc

PUBLIC ____sdcc_ll_sub_deix_bc_hl

EXTERN ____sdcc_ll_sub_de_bc_hl

____sdcc_ll_sub_deix_bc_hl:

   push hl
   
   push ix
   pop hl
   
   add hl,de
   ex de,hl
   
   pop hl

   jp ____sdcc_ll_sub_de_bc_hl