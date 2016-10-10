*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	68000 library configuration
*-------------------------------------------------------*

use_sprite_saverestore	=	0			; automatic save/restore of background
use_sprite_pagerestore	=	1
use_slab_restore	set	0
use_blitter_nops	=	0			; insert nop before blitter restart as per Atari docs (!?)
use_sanity_checks	set	0
use_rasters		set	0
use_hog			set	0

; software breakpoint for Steem Boiler
; to continue... set V flag and trace
.macro	bbreak
	andi		#~2,ccr
	bvc.s		*
	.endm
	