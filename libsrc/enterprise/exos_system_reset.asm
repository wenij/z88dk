;
;	Enterprise 64/128 specific routines
;	by Stefano Bodrato, 2011
;
;	exos_system_reset();
;
;
;	$Id: exos_system_reset.asm,v 1.1 2011-03-14 11:36:48 stefano Exp $
;

	XLIB	exos_system_reset

exos_system_reset:

	rst   30h
	defb  0

	ret
