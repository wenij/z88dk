; void tshr_cls(uchar ink)

SECTION code_clib
SECTION code_arch

PUBLIC tshr_cls_fastcall

EXTERN asm_tshr_cls

defc tshr_cls_fastcall = asm_tshr_cls
