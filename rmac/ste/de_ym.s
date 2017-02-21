*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Entity processing: Y major
*-------------------------------------------------------*
	
*-------------------------------------------------------*
*	Tear through entity chain in sorted order
*	drawing with appropriate [drawtype] calls
*-------------------------------------------------------*
*	Note: implements statechange minimization so
*	different draw functions get a chance to
*	call their own init functions, but only when
*	the [drawtype] changes between entities.
*-------------------------------------------------------*
_BLiT_EntityDrawVisibleYM_68k:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
.savea6:		ds.l	1
.save:			ds.l	10
.return:		ds.l	1
*-------------------------------------------------------*
.pctx:			ds.l	1
*-------------------------------------------------------*
.frame_:		
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
*	locals: used by sprite routines
*-------------------------------------------------------*
.sp_skew:		ds.w	1			; don't move this one
.sp_maskwidth:		ds.w	1
.sp_yoff:		ds.w	1
.sp_yrem:		ds.w	1
.sp_save_usp:		ds.l	1
.sp_linetable:		ds.l	1
*-------------------------------------------------------*
*	locals: shadow function arguments
*-------------------------------------------------------*
.sp_framebuf:		ds.l	1
.sp_snapx:		ds.w	1
.sp_snapy:		ds.w	1
*-------------------------------------------------------*
*	locals: used by algorithm
*-------------------------------------------------------*
.sp_asset:		ds.l	1
.sp_pymax:		ds.l	1
.sp_savea0:		ds.l	1
.sp_drawtype:		ds.w	1
*-------------------------------------------------------*
.sp_frame_:		
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*
	.if		^^defined AGT_CONFIG_DEBUGDRAW
	tst.w		_g_debugdraw
	bne		_BLiT_EntityDebugVisibleYM_68k
	.endif
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
*	load C args to local frame
*-------------------------------------------------------*
	move.l		.pctx(a6),a4
	move.l		dctx_framebuffer(a4),.sp_framebuf(sp)
	move.l		dctx_lineidx(a4),.sp_linetable(sp)
	move.w		dctx_snapadjx(a4),.sp_snapx(sp)
	move.w		dctx_snapadjy(a4),.sp_snapy(sp)	
;	move.l		.pdst(a6),.sp_framebuf(sp)	; todo: should be part of a framebuffer context, not separate arg
;	move.w		.snapx+2(a6),.sp_snapx(sp)	; todo: ditto
;	move.w		.snapy+2(a6),.sp_snapy(sp)	; todo: ditto
;	move.l		.plinetable(a6),.sp_linetable(sp)
*-------------------------------------------------------*
*	load display chain terminating condition
*-------------------------------------------------------*
	move.l		_g_pe_viewport,a0
	move.w		ent_fry(a0),d0
	add.w		#240+(32*2),d0			; todo: hardcoded visibility margin - make configurable
	move.w		d0,.sp_pymax(sp)
*-------------------------------------------------------*
*	initial conditions
*-------------------------------------------------------*

	move.l		dctx_clearstack_tide(a4),a5
	move.l		a5,usp

*-------------------------------------------------------*
	
	; important to begin with invalid drawtype
	; so the first drawtype will call init first

	move.w		#-1,.sp_drawtype(sp)
	
	bra		.STE_EntityDrawVisibleYM_EMXSPR_done

*=======================================================*
*	dedicated iterator for [drawtype]=EMXSPR
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_EMXSPR_next:
*-------------------------------------------------------*

	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleYM_evaluate

*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_EMXSPR_go:
*-------------------------------------------------------*

	; access coordinates

	move.w		ent_frx(a0),d1
;	move.w		ent_fry(a0),d2			; we already fetched this in preamble

	; compensate for loopback buffer

;	sub.w		.sp_snapx(sp),d1		; not really needed unless LOOPBACK used on HSCROLL - kinda pointless
	sub.w		.sp_snapy(sp),d2		; must be done if LOOPBACK used for VSCROLL - very likely

	; access sprite frame

	move.w		ent_frame(a0),d0

	; decrement 'hitflash' counter on this object

	clr.w		d7
	sub.w		ent_draw_hitflash(a0),d7	; $0000 / $FFxx
	subx.w		d7,d7				; $0000 / $FFFF
	add.w		d7,ent_draw_hitflash(a0)

	; asset/framebuffer

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	.endif

	move.l		a0,.sp_savea0(sp)
	move.l		ent_passet(a0),a0
	move.l		.sp_framebuf(sp),a1

	; include raw drawing function here (configure it first)

enable_restore	set	1				; enable sprite-restore
enable_clipping	set	1				; enable full xy clipping

	include		"ste/b_emx_spr.s"		; body for EMX optimized blitter sprites (0<width<=32)

	move.l		.sp_savea0(sp),a0

*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_EMXSPR_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pymax(sp),d2
	blt		.STE_EntityDrawVisibleYM_EMXSPR_next
	bra		.STE_EntityDrawVisibleYM_complete
*-------------------------------------------------------*
*	[drawtype] has suddenly changed to EMXSPR
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_EMXSPR_setup:
*-------------------------------------------------------*
	jsr		_AGT_BLiT_EMSprInit			; todo: init type depends on platform & spr format
	bra		.STE_EntityDrawVisibleYM_EMXSPR_go



*=======================================================*
*	dedicated iterator for [drawtype]=EMXSPR
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_EMSPR_next:
*-------------------------------------------------------*

	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleYM_evaluate

*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_EMSPR_go:
*-------------------------------------------------------*

	; access coordinates

	move.w		ent_frx(a0),d1
;	move.w		ent_fry(a0),d2			; we already fetched this in preamble

	; compensate for loopback buffer

;	sub.w		.sp_snapx(sp),d1		; not really needed unless LOOPBACK used on HSCROLL - kinda pointless
	sub.w		.sp_snapy(sp),d2		; must be done if LOOPBACK used for VSCROLL - very likely

	; access sprite frame

	move.w		ent_frame(a0),d0

	; decrement 'hitflash' counter on this object

	clr.w		d7
	sub.w		ent_draw_hitflash(a0),d7	; $0000 / $FFxx
	subx.w		d7,d7				; $0000 / $FFFF
	add.w		d7,ent_draw_hitflash(a0)

	; asset/framebuffer

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	.endif

	move.l		a0,.sp_savea0(sp)
	move.l		ent_passet(a0),a0
	move.l		.sp_framebuf(sp),a1

	; include raw drawing function here (configure it first)

enable_restore	set	1				; enable sprite-restore
enable_clipping	set	1				; enable full xy clipping

	include		"ste/b_em_spr.s"		; body for EMX optimized blitter sprites (0<width<=32)

	move.l		.sp_savea0(sp),a0

*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_EMSPR_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pymax(sp),d2
	blt		.STE_EntityDrawVisibleYM_EMSPR_next
	bra		.STE_EntityDrawVisibleYM_complete
*-------------------------------------------------------*
*	[drawtype] has suddenly changed to EMSPR
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_EMSPR_setup:
*-------------------------------------------------------*
	jsr		_AGT_BLiT_EMSprInit			; todo: init type depends on platform & spr format
	bra		.STE_EntityDrawVisibleYM_EMSPR_go



*=======================================================*
*	dedicated iterator for [drawtype]=SPRITE
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_SPRITE_next:
*-------------------------------------------------------*

	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleYM_evaluate
	
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_SPRITE_go:
*-------------------------------------------------------*

	; access coordinates

	move.w		ent_frx(a0),d1
;	move.w		ent_fry(a0),d2			; we already fetched this in preamble
	
	; compensate for loopback buffer
	
;	sub.w		.sp_snapx(sp),d1		; not really needed unless LOOPBACK used on HSCROLL - kinda pointless
	sub.w		.sp_snapy(sp),d2		; must be done if LOOPBACK used for VSCROLL - very likely

	; access sprite frame
	
	move.w		ent_frame(a0),d0

	; decrement 'hitflash' counter on this object
	
	clr.w		d7
	sub.w		ent_draw_hitflash(a0),d7	; $0000 / $FFxx
	subx.w		d7,d7				; $0000 / $FFFF
	add.w		d7,ent_draw_hitflash(a0)
	
	; asset/framebuffer

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	.endif

	move.l		a0,.sp_savea0(sp)
	move.l		ent_passet(a0),a0
	move.l		.sp_framebuf(sp),a1
		
	; include raw drawing function here (configure it first)
	
enable_restore	set	1				; enable sprite-restore
enable_clipping	set	1				; enable full xy clipping

	include		"ste/b_spr.s"			; body for normal blitter sprites

	move.l		.sp_savea0(sp),a0

*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_SPRITE_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pymax(sp),d2
	blt		.STE_EntityDrawVisibleYM_SPRITE_next
	bra		.STE_EntityDrawVisibleYM_complete
*-------------------------------------------------------*
*	[drawtype] has suddenly changed to SPRITE
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_SPRITE_setup:
*-------------------------------------------------------*
	jsr		_AGT_BLiT_IMSprInit			; todo: init type depends on platform & spr format
	bra		.STE_EntityDrawVisibleYM_SPRITE_go



*=======================================================*
*	dedicated iterator for [drawtype]=SLAB
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_SLAB_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleYM_evaluate
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_SLAB_go:
*-------------------------------------------------------*

	; access coordinates
	
	move.w		ent_frx(a0),d1
;	move.w		ent_fry(a0),d2			; we already fetched this in preamble
	
	; compensate for loopback buffer
	
;	sub.w		.sp_snapx(sp),d1		; not really needed unless LOOPBACK used on HSCROLL - kinda pointless
	sub.w		.sp_snapy(sp),d2		; must be done if LOOPBACK used for VSCROLL - very likely

	; access sprite frame
	
	move.w		ent_frame(a0),d0

	; decrement 'hitflash' counter on this object
	
	clr.w		d7
	sub.w		ent_draw_hitflash(a0),d7	; $0000 / $FFxx
	subx.w		d7,d7				; $0000 / $FFFF
	add.w		d7,ent_draw_hitflash(a0)
	
	; asset/framebuffer

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	.endif

	move.l		a0,.sp_savea0(sp)
	move.l		ent_passet(a0),a0
	move.l		.sp_framebuf(sp),a1
		
	; include raw drawing function here (configure it first)
	
use_slab_restore	set	0
enable_restore		set	1
preserve_usp		set	1

	move.l		lpair_first(a0),a0
	
	include		"ste/b_slabopt.s"
	
	; restore step - only if restore asset is present

	move.l		.sp_savea0(sp),a0

	move.l		ent_passet(a0),a1
	move.l		lpair_second(a1),d3
	beq		.no_restore
	
	move.w		ent_frx(a0),d1
	move.w		ent_fry(a0),d2
	sub.w		.sp_snapy(sp),d2
	move.w		ent_frame(a0),d0

	move.l		.sp_framebuf(sp),a1

	; include raw restore function here (configure it first)

use_slab_restore	set	1
enable_restore		set	1
preserve_usp		set	0

	move.l		d3,a0

	include		"ste/b_slabres.s"

.no_restore:

	move.l		.sp_savea0(sp),a0
	
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_SLAB_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pymax(sp),d2
	blt		.STE_EntityDrawVisibleYM_SLAB_next
	bra		.STE_EntityDrawVisibleYM_complete
*-------------------------------------------------------*
*	[drawtype] has suddenly changed to SPRITE
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_SLAB_setup:
*-------------------------------------------------------*
	jsr		_AGT_BLiT_SlabInit			
	bra		.STE_EntityDrawVisibleYM_SLAB_go



*=======================================================*
*	dedicated iterator for [drawtype]=CUSTOM
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_CUSTOM_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleYM_evaluate
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_CUSTOM_go:
*-------------------------------------------------------*

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	.endif

;	move.l		ent_fndraw(a0),a6		; todo
;	jsr		(a6)
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_CUSTOM_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pymax(sp),d2
	blt		.STE_EntityDrawVisibleYM_CUSTOM_next
	bra		.STE_EntityDrawVisibleYM_complete
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_CUSTOM_setup:
*-------------------------------------------------------*
;	move.l		ent_fndrawinit(a0),a6		; todo
;	jsr		(a6)
	bra		.STE_EntityDrawVisibleYM_CUSTOM_go
	
*=======================================================*
*	[drawtype] has changed - perform path switch
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_evaluate:
*-------------------------------------------------------*
	move.w		d0,.sp_drawtype(sp)
	add.w		d0,d0
	add.w		d0,d0
	jmp		.types(pc,d0.w)
.types:	bra.w		.STE_EntityDrawVisibleYM_NONE_setup
	bra.w		.STE_EntityDrawVisibleYM_SPRITE_setup
	bra.w		.STE_EntityDrawVisibleYM_EMSPR_setup
	bra.w		.STE_EntityDrawVisibleYM_EMXSPR_setup
	bra.w		.STE_EntityDrawVisibleYM_SLAB_setup
	bra.w		.STE_EntityDrawVisibleYM_CUSTOM_setup
	bra.w		.STE_EntityDrawVisibleYM_RFILL_setup



*=======================================================*
*	dedicated iterator for [drawtype]=RFILL
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_RFILL_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleYM_evaluate
	
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_RFILL_go:
*-------------------------------------------------------*

	; access coordinates

	move.w		ent_frx(a0),d1
;	move.w		ent_fry(a0),d2			; we already fetched this in preamble
	
	; compensate for loopback buffer
	
;	sub.w		.sp_snapx(sp),d1		; not really needed unless LOOPBACK used on HSCROLL - kinda pointless
	sub.w		.sp_snapy(sp),d2		; must be done if LOOPBACK used for VSCROLL - very likely

	; use frame number as colour
	
	move.w		ent_frame(a0),d5
	
	move.w		ent_sx(a0),d3
	move.w		ent_sy(a0),d4
	
	; decrement 'hitflash' counter on this object
	
	clr.w		d7
	sub.w		ent_draw_hitflash(a0),d7	; $0000 / $FFxx
	subx.w		d7,d7				; $0000 / $FFFF
	add.w		d7,ent_draw_hitflash(a0)
	
	; asset/framebuffer

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	.endif

	move.l		a0,.sp_savea0(sp)
	move.l		ent_passet(a0),a0
	move.l		.sp_framebuf(sp),a1
		
	; include raw drawing function here (configure it first)
	
enable_restore	set	1				; enable sprite-restore
enable_clipping	set	1				; enable full xy clipping

	include		"ste/b_rfill.s"			; body for normal blitter sprites

	move.l		.sp_savea0(sp),a0

*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_RFILL_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pymax(sp),d2
	blt		.STE_EntityDrawVisibleYM_RFILL_next
	bra		.STE_EntityDrawVisibleYM_complete
*-------------------------------------------------------*
*	[drawtype] has suddenly changed to RFILL
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_RFILL_setup:
*-------------------------------------------------------*
	jsr		_AGT_BLiT_IMSprInit			; todo: init type depends on platform & spr format
	bra		.STE_EntityDrawVisibleYM_RFILL_go

	
*=======================================================*
*	dedicated iterator for [drawtype]=NONE
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_NONE_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleYM_evaluate
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_NONE_setup:
.STE_EntityDrawVisibleYM_NONE_go:
*-------------------------------------------------------*
	; nothing todo here - skip!
*-------------------------------------------------------*
.STE_EntityDrawVisibleYM_NONE_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pymax(sp),d2
	blt.s		.STE_EntityDrawVisibleYM_NONE_next
	bra		.STE_EntityDrawVisibleYM_complete

		
*=======================================================*
.STE_EntityDrawVisibleYM_complete:
*-------------------------------------------------------*

;	move.l		usp,a5
;	move.l		a5,_drawcontext+dctx_clearstack_tide

*-------------------------------------------------------*

	lea		.sp_frame_(sp),sp
	move.l		(sp)+,a6

	move.l		.pctx(a6),a4
	move.l		usp,a5
	move.l		a5,dctx_clearstack_tide(a4)
	
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts
	

	.if		^^defined AGT_CONFIG_DEBUGDRAW

*-------------------------------------------------------*
_BLiT_EntityDebugVisibleYM_68k:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
.savea6:		ds.l	1
.save:			ds.l	10
.return:		ds.l	1
*-------------------------------------------------------*
.pctx:			ds.l	1
*-------------------------------------------------------*
.frame_:		
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
*	locals: used by sprite routines
*-------------------------------------------------------*
.sp_skew:		ds.w	1			; don't move this one
.sp_maskwidth:		ds.w	1
.sp_yoff:		ds.w	1
.sp_yrem:		ds.w	1
.sp_save_usp:		ds.l	1
.sp_linetable:		ds.l	1
*-------------------------------------------------------*
*	locals: shadow function arguments
*-------------------------------------------------------*
.sp_framebuf:		ds.l	1
.sp_snapx:		ds.w	1
.sp_snapy:		ds.w	1
*-------------------------------------------------------*
*	locals: used by algorithm
*-------------------------------------------------------*
.sp_asset:		ds.l	1
.sp_pymax:		ds.l	1
.sp_savea0:		ds.l	1
.sp_drawtype:		ds.w	1
*-------------------------------------------------------*
.sp_frame_:		
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
*	load C args to local frame
*-------------------------------------------------------*
	move.l		.pctx(a6),a4
	move.l		dctx_framebuffer(a4),.sp_framebuf(sp)
	move.l		dctx_lineidx(a4),.sp_linetable(sp)
	move.w		dctx_snapadjx(a4),.sp_snapx(sp)
	move.w		dctx_snapadjy(a4),.sp_snapy(sp)	
;	move.l		.pdst(a6),.sp_framebuf(sp)	; todo: should be part of a framebuffer context, not separate arg
;	move.w		.snapx+2(a6),.sp_snapx(sp)	; todo: ditto
;	move.w		.snapy+2(a6),.sp_snapy(sp)	; todo: ditto
;	move.l		.plinetable(a6),.sp_linetable(sp)
*-------------------------------------------------------*
*	load display chain terminating condition
*-------------------------------------------------------*
	move.l		_g_pe_viewport,a0
	move.w		ent_fry(a0),d0
	add.w		#240+(32*2),d0			; todo: hardcoded visibility margin - make configurable
	move.w		d0,.sp_pymax(sp)
*-------------------------------------------------------*
*	initial conditions
*-------------------------------------------------------*

	move.l		dctx_clearstack_tide(a4),a5
	move.l		a5,usp

*-------------------------------------------------------*
	
	; important to begin with invalid drawtype
	; so the first drawtype will call init first

	jsr		_AGT_BLiT_IMSprInit

	bra		.STE_EntityDebugVisibleYM_RDEBUG_done

*=======================================================*
*	dedicated iterator for [drawtype]=RFILL
*-------------------------------------------------------*
.STE_EntityDebugVisibleYM_RDEBUG_next:
*-------------------------------------------------------*	
*-------------------------------------------------------*
.STE_EntityDebugVisibleYM_RDEBUG_go:
*-------------------------------------------------------*

	; access coordinates

	move.w		ent_frx(a0),d1
;	move.w		ent_fry(a0),d2			; we already fetched this in preamble
	
	; compensate for loopback buffer
	
;	sub.w		.sp_snapx(sp),d1		; not really needed unless LOOPBACK used on HSCROLL - kinda pointless
	sub.w		.sp_snapy(sp),d2		; must be done if LOOPBACK used for VSCROLL - very likely

	; use frame number as colour
	
	move.w		ent_frame(a0),d5
	add.w		ent_id(a0),d5
	
	move.w		ent_sx(a0),d3
	move.w		ent_sy(a0),d4
	
	; decrement 'hitflash' counter on this object
	
	clr.w		d7
	sub.w		ent_draw_hitflash(a0),d7	; $0000 / $FFxx
	subx.w		d7,d7				; $0000 / $FFFF
	add.w		d7,ent_draw_hitflash(a0)
	
	; asset/framebuffer

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	.endif

	move.l		a0,.sp_savea0(sp)
	move.l		ent_passet(a0),a0
	move.l		.sp_framebuf(sp),a1
		
	; include raw drawing function here (configure it first)
	
enable_restore	set	1				; enable sprite-restore
enable_clipping	set	1				; enable full xy clipping

	include		"ste/b_rfill.s"			; body for normal blitter sprites

	move.l		.sp_savea0(sp),a0

*-------------------------------------------------------*
.STE_EntityDebugVisibleYM_RDEBUG_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pymax(sp),d2
	blt		.STE_EntityDebugVisibleYM_RDEBUG_next
	bra		.STE_EntityDebugVisibleYM_complete
*-------------------------------------------------------*
*	[drawtype] has suddenly changed to RFILL
*-------------------------------------------------------*
.STE_EntityDebugVisibleYM_RDEBUG_setup:
*-------------------------------------------------------*
	jsr		_AGT_BLiT_IMSprInit			; todo: init type depends on platform & spr format
	bra		.STE_EntityDebugVisibleYM_RDEBUG_go

*=======================================================*
.STE_EntityDebugVisibleYM_complete:
*-------------------------------------------------------*

;	move.l		usp,a5
;	move.l		a5,_drawcontext+dctx_clearstack_tide

*-------------------------------------------------------*

	lea		.sp_frame_(sp),sp
	move.l		(sp)+,a6

	move.l		.pctx(a6),a4
	move.l		usp,a5
	move.l		a5,dctx_clearstack_tide(a4)
	
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

	.endif		;^^defined AGT_CONFIG_DEBUGDRAW	

	
*-------------------------------------------------------*
_EntityInteractVisibleYM_68k:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
.savea6:		ds.l	1
.save:			ds.l	10
.return:		ds.l	1
*-------------------------------------------------------*
.frame_:		
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
*	locals: shadow function arguments
*-------------------------------------------------------*
*-------------------------------------------------------*
*	locals: used by algorithm
*-------------------------------------------------------*
.sp_pxmax:		ds.w	1
.sp_pymax:		ds.w	1
.sp_savea0:		ds.l	1
*-------------------------------------------------------*
.sp_frame_:		
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*	
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
*	load C args to local frame
*-------------------------------------------------------*

	move.l		_g_pe_viewport,a2
	move.w		ent_frx(a2),d1
	add.w		#320+(32*2),d1			; todo: hardcoded visibility margin - make configurable
	move.w		d1,.sp_pxmax(sp)		; pxmax
	
	move.w		ent_fry(a2),d2
	add.w		#240+(32*2),d2
	move.w		d2,.sp_pymax(sp)		; pymax

	move.l		ent_pnext_y(a2),a0		; pcurr
	cmp.w		ent_fry(a0),d2			; (pcurr->ry < pymax)
	ble		.term_outer
*-------------------------------------------------------*
.next_outer:
*-------------------------------------------------------*
	move.w		ent_self(a0),d7
	bmi.s		.term_inner

	.if		(0)

	btst		#10,d7
	beq.s		.no_vistrig

	eor.w		#($0400|$0200),d7		; & (~EntityFlag_VISTRIG) | EntityFlag_TICKED
	move.w		d7,ent_self(a0)

	; todo: opt	
	movem.l		a0/d1/d2,-(sp)
	
	; todo: opt	
	pea		(a0)
	move.l		_g_pe_viewport,a2
	move.l		ent_pnext_y(a2),-(sp)	
;	jsr		_EntityLink_Tick
	addq.l		#8,sp

	; todo: opt	
	movem.l		(sp)+,a0/d1/d2
	
	.endif
	
*-------------------------------------------------------*
.no_vistrig:
*-------------------------------------------------------*
	move.w		ent_fry(a0),d4
	add.w		ent_sy(a0),d4			; vmax = e.ry + e.sy;
	move.w		ent_frx(a0),d3
	cmp.w		d3,d1
	ble.s		.term_inner
	add.w		ent_sx(a0),d3			; hmax = e.rx + e.sx
	cmp.w		ent_frx(a2),d3
	ble.s		.term_inner

*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a1		; petest = e.pnext_y;
	cmp.w		ent_fry(a1),d4			; (petest->ry < vmax)
	ble.s		.term_inner

*-------------------------------------------------------*
.next_inner:
*-------------------------------------------------------*
	
	move.w		ent_frx(a1),d5
	cmp.w		d5,d3				; (petest->rx < hmax)
	ble.s		.no_contact
	add.w		ent_sx(a1),d5			; ((petest->rx + petest->sx) > e.rx)
	cmp.w		ent_frx(a0),d5
	ble.s		.no_contact

;	avoid interacting pre-killed objects

	move.w		ent_self(a1),d0
	bmi.s		.no_contact
	
*-------------------------------------------------------*
.do_interactions:
*-------------------------------------------------------*

	move.w		ent_interactswith(a1),d6
	and.w		d7,d6
	bne.s		.do_i0
.no_i0:
	move.w		ent_interactswith(a0),d6
	and.w		ent_self(a1),d6
	bne.s		.do_i1
.no_i1:
	
*-------------------------------------------------------*
.no_contact:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a1),a1		; petest = ptest->pnext_y;
	cmp.w		ent_fry(a1),d4
	bgt.s		.next_inner

*-------------------------------------------------------*
.term_inner:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0
	cmp.w		ent_fry(a0),d2
	bgt.s		.next_outer
	bra.s		.term_outer	

.do_i0:
	move.l		ent_fncollide(a0),d0
	beq.s		.no_i0
	move.l		d0,a6

	move.w		d3,-(sp)
	move.w		d4,-(sp)

	pea		(a1)
	pea		(a0)
	jsr		(a6)
	move.l		(sp)+,a0
	move.l		(sp)+,a1

	move.w		(sp)+,d4
	move.w		(sp)+,d3

	move.l		_g_pe_viewport,a2
	move.w		.sp_pxmax(sp),d1
	move.w		.sp_pymax(sp),d2
	move.w		ent_self(a0),d7
	bra.s		.no_i0
	
.do_i1:
	move.l		ent_fncollide(a1),d0
	beq.s		.no_i1
	move.l		d0,a6

	move.w		d3,-(sp)
	move.w		d4,-(sp)

	pea		(a0)
	pea		(a1)
	jsr		(a6)
	move.l		(sp)+,a1
	move.l		(sp)+,a0

	move.w		(sp)+,d4
	move.w		(sp)+,d3

	move.l		_g_pe_viewport,a2
	move.w		.sp_pxmax(sp),d1
	move.w		.sp_pymax(sp),d2
	move.w		ent_self(a0),d7
	bra.s		.no_i1

*-------------------------------------------------------*
.term_outer:
*-------------------------------------------------------*
	lea		.sp_frame_(sp),sp
	move.l		(sp)+,a6
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

