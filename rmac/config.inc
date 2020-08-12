*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	68000 library configuration
*-------------------------------------------------------*

use_sprite_saverestore	=	0			; automatic save/restore of background
use_sprite_pagerestore	=	1
enable_slabrestore	set	0
use_blitter_nops	=	1			; insert nop before blitter restart as per Atari docs (!?)
;use_sanity_checks	set	0
use_rasters		set	0
use_hog			set	0
bldcfg_use_mouse	=	0
use_unsafe_function_keys	=	1
use_timecritical_rasters	=	1

*-------------------------------------------------------*
*	externally configured via CFLAGS
*-------------------------------------------------------*

			.if	^^defined AGT_CONFIG_DEBUG_CONSOLE
use_debugconsole	=	1
			.else
use_debugconsole	=	0
			.endif

			.if	^^defined TIMING_RASTERS
use_rasters		set	1
			.endif
			
			.if	^^defined AGT_CONFIG_SYS_GUARDX
			.else
AGT_CONFIG_SYS_GUARDX	=	32
			.endif

dst_linewid	set	(((20+1)+((AGT_CONFIG_SYS_GUARDX+15)>>4))*8)

*-------------------------------------------------------*

BGRAST			=	$64
BGRSAVE			=	$34
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

.macro	pushraster	bgc
	.if		(use_rasters)
	move.w		BGRAST.w,BGRSAVE.w
	move.w		\bgc,$ffff8240.w
	move.w		\bgc,BGRAST.w
	.endif
	.endm

.macro	setraster	bgc
	.if		(use_rasters)
	move.w		\bgc,BGRAST.w
	move.w		\bgc,$ffff8240.w
	.endif
	.endm

.macro	addraster	bgc
	.if		(use_rasters)
	add.w		\bgc,BGRAST.w
	move.w		BGRAST.w,$ffff8240.w
	.endif
	.endm


.macro	altraster	bgc
	.if		(use_rasters)
	eor.w		\bgc,BGRAST.w
	move.w		BGRAST.w,$ffff8240.w
	.endif
	.endm
	
.macro	popraster
	.if		(use_rasters)
	move.w		BGRSAVE.w,BGRAST.w
	move.w		BGRSAVE.w,$ffff8240.w
	.endif
	.endm

	