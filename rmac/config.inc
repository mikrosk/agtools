*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	68000 library configuration
*-------------------------------------------------------*

use_sprite_saverestore	=	0			; automatic save/restore of background
use_sprite_pagerestore	=	1
enable_slabrestore	set	0
use_blitter_nops	=	0			; insert nop before blitter restart as per Atari docs (!?)
;use_sanity_checks	set	0
use_rasters		set	0
use_hog			set	0
bldcfg_use_mouse	=	0
use_unsafe_function_keys	=	1

*-------------------------------------------------------*
*	externally configured via CFLAGS
*-------------------------------------------------------*

			.if	^^defined AGT_CONFIG_DEBUG_CONSOLE
use_debugconsole	=	1
			.else
use_debugconsole	=	0
			.endif

*-------------------------------------------------------*


VBServiceVec		=	$a0
DMAFlag			=	$a4
g_dbg_curr_entity	=	$a8

*-------------------------------------------------------*
	
; software breakpoint for Steem Boiler
; to continue... set V flag and trace
.macro	bbreak
	andi		#~2,ccr
	bvc.s		*
	.endm
	