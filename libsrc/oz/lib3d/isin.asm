;
;	OZ-7xx DK emulation layer for Z88DK
;	by Stefano Bodrato - Oct. 2003
;
;	int isin(unsigned degrees);
;	input must be between 0 and 360
;	returns value from -16384 to +16384
;
; ------
; $Id: isin.asm,v 1.1 2003-10-29 11:37:11 stefano Exp $
;

	XLIB	isin

	XDEF	sin_start

isin:
        pop     de
        pop     hl
        push    hl
        push    de
sin_start:
        ld      c,l
        ld      b,h ;; save input
        ld      de,181
        or      a
        sbc     hl,de
        jr      c,DontFlip
        ld      hl,360
        sbc     hl,bc  ;; no carry here
        ld      c,l
        ld      b,h
        ld      a,1
        jr      Norm_0_180
DontFlip:
        ld      l,c
        ld      h,b
        xor     a
Norm_0_180:
;; degrees normalized between 0 and 180 in hl and in bc
;; sign of output in a
        ld      de,90
        or      a
        sbc     hl,de
        jr      c,DontFlip2
        ld      hl,180
        sbc     hl,bc   ;; carry is 0 here
        jr      Norm_0_90
DontFlip2:
        ld      l,c
        ld      h,b
Norm_0_90:
;; degrees normalized between 0 and 90 in hl and sign of answer is in a
        add     hl,hl
        ld      de,sin_table
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
;; de=answer
        or      a
        jr      z,DontNegate
        ld      hl,0
        sbc     hl,de   ;; carry is 0 here
        ret
DontNegate:
        ex      de,hl
        ret
sin_table:
;; generated by sintable.c
defw 0
defw 285
defw 571
defw 857
defw 1142
defw 1427
defw 1712
defw 1996
defw 2280
defw 2563
defw 2845
defw 3126
defw 3406
defw 3685
defw 3963
defw 4240
defw 4516
defw 4790
defw 5062
defw 5334
defw 5603
defw 5871
defw 6137
defw 6401
defw 6663
defw 6924
defw 7182
defw 7438
defw 7691
defw 7943
defw 8191
defw 8438
defw 8682
defw 8923
defw 9161
defw 9397
defw 9630
defw 9860
defw 10086
defw 10310
defw 10531
defw 10748
defw 10963
defw 11173
defw 11381
defw 11585
defw 11785
defw 11982
defw 12175
defw 12365
defw 12550
defw 12732
defw 12910
defw 13084
defw 13254
defw 13420
defw 13582
defw 13740
defw 13894
defw 14043
defw 14188
defw 14329
defw 14466
defw 14598
defw 14725
defw 14848
defw 14967
defw 15081
defw 15190
defw 15295
defw 15395
defw 15491
defw 15582
defw 15668
defw 15749
defw 15825
defw 15897
defw 15964
defw 16025
defw 16082
defw 16135
defw 16182
defw 16224
defw 16261
defw 16294
defw 16321
defw 16344
defw 16361
defw 16374
defw 16381
defw 16384
