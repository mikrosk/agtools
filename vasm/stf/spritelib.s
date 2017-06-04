*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Realtime-shifted 4bpl sprites:
*	- Optional auto-clearing mechanism
*	- Optional clipping path
*	- Includes FISR/NFSR read optimizations
*	- Compatible with .ims files from AGTCut
*-------------------------------------------------------*

	COMMENT		HEAD=7
	OPT		P=68000,NOTYPE
	OPT		W+
;	OPT		OW+
	OPT		O-

*-------------------------------------------------------*

	include		"config.inc"

	include		"asset.inc"

	include		"drawctx.inc"

	include		"macros.inc"

*-------------------------------------------------------*

within_entitydraw	set	0

*-------------------------------------------------------*

; enable this for automatic clearing of sprites in
; reverse drawing order, to restore background area

;use_sprite_saverestore	=	1
enable_restore	set	1

;	general purpose sprite routines
;	XDEF		_AGT_IMSprDrawClipped
;	XDEF		_AGT_IMSprDrawUnClipped
;	XDEF		_AGT_IMSprInit

	XDEF		_AGT_MarkDirtyRectangle

	XDEF		_AGT_BLiT_IMSprDrawString

*-------------------------------------------------------*

;	general purpose sprite routines
	XDEF		_AGT_STF_ISSprDrawClipped
	XDEF		_AGT_STF_ISSprDrawUnClipped

;	optimized version (uses more code space)
	XDEF		_AGT_STF_ISSprDrawClipOpt
	XDEF		_AGT_STF_ISSprDrawNoClipOpt
	
	XDEF		STF_ISSprOpt_impl_

	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
;	sprite clearing mechanism
	XDEF		_AGT_PlayfieldRestoreInit
	XDEF		_AGT_PlayfieldRestoreReset
	XDEF		_AGT_STF_PlayfieldRestore
	endc
	
;	pixel offset for a preshifted framebuffer
;	XREF		_g_tshift

;	clipping window
	XDEF		_g_spr_window
	XDEF		_g_spr_window_x1
	XDEF		_g_spr_window_y1
	XDEF		_g_spr_window_x2
	XDEF		_g_spr_window_y2

	
	XREF		_g_linebytes
	
	XDEF		_ascii2font

*-------------------------------------------------------*
	text
*-------------------------------------------------------*

enable_restore		set	1

*-------------------------------------------------------*
_AGT_MarkDirtyRectangle:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.pdst:			rs.l	1
.xpos:			rs.l	1
.ypos:			rs.l	1
.xs:			rs.l	1
.ys:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
*-------------------------------------------------------*
	move.l		.pdst(a6),a1
	move.w		.xpos+2(a6),d1
	move.w		.ypos+2(a6),d2
	move.w		.xs+2(a6),d3
	move.w		.ys+2(a6),d4
	
	; todo: optimize all of this
	
	add.w		d1,d3
	add.w		#16-1,d3
	and.w		#-16,d1
	and.w		#-16,d3
	sub.w		d1,d3
		
	lsr.w		#1,d3
	lsr.w		#1,d1
	
	ifne		(enable_restore&(use_sprite_saverestore|use_sprite_pagerestore))
	move.l		_drawcontext+dctx_prestorestate,a4
	move.l		rstr_ptide(a4),a5
	move.l		a5,d5
	endc
	
	
;	lea		160.w,a4
	move.w		_g_linebytes,a4
	sub.w		d3,a4


	move.w		d3,d6
	lsr.w		#2,d6
	
	add.w		d1,a1
	
	move.w		d2,d7
	mulu.w		_g_linebytes,d7
	add.l		d7,a1
	
	ifne		(enable_restore&use_sprite_pagerestore)	
	move.w		d2,(a5)+			; y
	move.w		d1,(a5)+			; xo

	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d4,(a5)+			; lines
	endc	


	ifne		(enable_restore&(use_sprite_saverestore|use_sprite_pagerestore))
	move.l		d5,(a5)+
	move.l		_drawcontext+dctx_prestorestate,a4
	move.l		a5,rstr_ptide(a4)
	endc

*-------------------------------------------------------*
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*
*	SVMC: variable-shift, clipped, masked on key=0
*	SFMC: optimized/fixed-shift, clipped, masked on key=0
*-------------------------------------------------------*

	ifd		AGT_CONFIG_STF_OPT_SIZE
	include		"stf/spr_svmc.s"
	else
	include		"stf/spr_sfmc.s"
	endc

*-------------------------------------------------------*

	include		"stf/spr_clear.s"

*-------------------------------------------------------*
	bss
*-------------------------------------------------------*
	
_ascii2font:		ds.b		256
	
_g_spr_window:
_g_spr_window_x1:	ds.w		1
_g_spr_window_y1:	ds.w		1
_g_spr_window_x2:	ds.w		1
_g_spr_window_y2:	ds.w		1
	
*-------------------------------------------------------*
	text
*-------------------------------------------------------*
