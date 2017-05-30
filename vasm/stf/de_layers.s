*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Entity processing: layer drawing
*-------------------------------------------------------*
			
	include		"macros.inc"
			
*-------------------------------------------------------*
*	Tear through entity chain in sorted order
*	drawing with appropriate [drawtype] calls
*-------------------------------------------------------*
*	Note: implements statechange minimization so
*	different draw functions get a chance to
*	call their own init functions, but only when
*	the [drawtype] changes between entities.
*-------------------------------------------------------*
_STF_EntityDrawLayers_68k:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.pctx:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
*	locals: used by sprite routines
*-------------------------------------------------------*
.sp_skew:		rs.w	1			; don't move this one
.sp_maskwidth:		rs.w	1
.sp_yoff:		rs.w	1
.sp_yrem:		rs.w	1
.sp_save_usp:		rs.l	1
.sp_linetable:		rs.l	1
*-------------------------------------------------------*
*	locals: shadow function arguments
*-------------------------------------------------------*
.sp_framebuf:		rs.l	1
.sp_snapx:		rs.w	1
.sp_snapy:		rs.w	1
*-------------------------------------------------------*
*	locals: used by algorithm
*-------------------------------------------------------*
.sp_asset:		rs.l	1
.sp_pxymax_:		rs.l	1
.sp_savea0:		rs.l	1
.sp_layers:		rs.l	1
.sp_drawtype:		rs.w	1
*-------------------------------------------------------*
.sp_frame_:		rs.b	0
*-------------------------------------------------------*
;	ifd		AGT_CONFIG_DEBUGDRAW
;	tst.w		_g_debugdraw
;	bne		_STF_EntityDebugVisibleXM_68k
;	endc
*-------------------------------------------------------*	
;	movem.l		d2-d7/a2-a5,-(sp)
;	link		a6,#-.frame_
;	move.l		a6,-(sp)
;	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
*	load C args to local frame
*-------------------------------------------------------*
	move.l		.pctx(a6),a4
	move.l		dctx_framebuffer(a4),.sp_framebuf(sp)
	move.l		dctx_lineidx(a4),.sp_linetable(sp)
	move.w		dctx_snapadjx(a4),.sp_snapx(sp)
	move.w		dctx_snapadjy(a4),.sp_snapy(sp)	
*-------------------------------------------------------*
*	initial conditions
*-------------------------------------------------------*
	
;	move.l		dctx_clearstack_tide(a4),a5
	move.l		dctx_prestorestate(a4),a5
	move.l		rstr_ptide(a5),a5
	move.l		a5,usp

*-------------------------------------------------------*
	
	; important to begin with invalid drawtype
	; so the first drawtype will call init


*=======================================================*
.STF_EntityDrawLayers_layer:
*=======================================================*
	move.w		#-1,.sp_drawtype(sp)
	bra		.STF_EntityDrawLayers_start

*=======================================================*
.STF_EntityDrawLayers_startlayer:
*=======================================================*
	move.l		a1,.sp_layers(sp)
	move.l		d1,a0


*=======================================================*
*	dedicated iterator for [drawtype]=SPRITE
*-------------------------------------------------------*
.STF_EntityDrawLayers_SPRITE_next:
*-------------------------------------------------------*

	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STF_EntityDrawLayers_evaluate
	
*-------------------------------------------------------*
.STF_EntityDrawLayers_SPRITE_go:
*-------------------------------------------------------*
	
	; access coordinates
	
	move.w		ent_frx(a0),d1			; we already fetched this in preamble
	move.w		ent_fry(a0),d2
	
	; compensate for loopback buffer
	
;	sub.w		.sp_snapx(sp),d1		; not really needed unless LOOPBACK used on HSCROLL - kinda pointless
	sub.w		.sp_snapy(sp),d2		; must be done if LOOPBACK used for VSCROLL - very likely

	; access sprite frame
	
	move.w		ent_frame(a0),d0

	; decrement 'hitflash' counter on this object
	
;	clr.w		d7
;	sub.w		ent_draw_hitflash(a0),d7	; $0000 / $FFxx
;	subx.w		d7,d7				; $0000 / $FFFF
;	add.w		d7,ent_draw_hitflash(a0)
	
	; asset/framebuffer

	ifd		AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	endc

	move.l		a0,.sp_savea0(sp)
	move.l		ent_passet(a0),a0
	move.l		.sp_framebuf(sp),a1
		
	; include raw drawing function here (configure it first)
	
enable_restore	set	1				; enable sprite-restore
enable_clipping	set	1				; enable full xy clipping

	ifd		AGT_CONFIG_STF_OPT_SIZE
	include		"stf/spr_svmc_clp.s"		; body for normal clipped sprites
	else
	include		"stf/spr_sfmc_clp.s"		; body for normal clipped sprites
	endc

	move.l		.sp_savea0(sp),a0

*-------------------------------------------------------*
.STF_EntityDrawLayers_SPRITE_done:
*-------------------------------------------------------*
	move.l		ent_layer_(a0),d1
	move.l		d1,a0
	bne		.STF_EntityDrawLayers_SPRITE_next
	bra		.STF_EntityDrawLayers_endlayer
*-------------------------------------------------------*
*	[drawtype] has suddenly changed to SPRITE
*-------------------------------------------------------*
.STF_EntityDrawLayers_SPRITE_setup:
*-------------------------------------------------------*
	bra		.STF_EntityDrawLayers_SPRITE_go


*=======================================================*
*	dedicated iterator for [drawtype]=CUSTOM
*-------------------------------------------------------*
.STF_EntityDrawLayers_CUSTOM_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STF_EntityDrawLayers_evaluate
*-------------------------------------------------------*
.STF_EntityDrawLayers_CUSTOM_go:
*-------------------------------------------------------*

	ifd		AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	endc

;	move.l		ent_fndraw(a0),a6		; todo
;	jsr		(a6)
*-------------------------------------------------------*
.STF_EntityDrawLayers_CUSTOM_done:
*-------------------------------------------------------*
	move.l		ent_layer_(a0),d1
	move.l		d1,a0
	bne		.STF_EntityDrawLayers_CUSTOM_next
	bra		.STF_EntityDrawLayers_endlayer

*-------------------------------------------------------*
.STF_EntityDrawLayers_CUSTOM_setup:
.STF_EntityDrawLayers_NONE_setup:
*-------------------------------------------------------*
;	move.l		ent_fndrawinit(a0),a6		; todo
;	jsr		(a6)
	bra		.STF_EntityDrawLayers_CUSTOM_go
	
*=======================================================*
*	[drawtype] has changed - perform path switch
*-------------------------------------------------------*
.STF_EntityDrawLayers_evaluate:
*-------------------------------------------------------*
	move.w		d0,.sp_drawtype(sp)
	add.w		d0,d0
	add.w		d0,d0
	jmp		.types(pc,d0.w)
.types:	bra.w		*	;.STF_EntityDrawLayers_NONE_setup
	bra.w		.STF_EntityDrawLayers_SPRITE_setup
	bra.w		.STF_EntityDrawLayers_NONE_setup
	bra.w		.STF_EntityDrawLayers_NONE_setup
	bra.w		.STF_EntityDrawLayers_NONE_setup
	bra.w		.STF_EntityDrawLayers_CUSTOM_setup
	bra.w		.STF_EntityDrawLayers_NONE_setup


	
*=======================================================*
.STF_EntityDrawLayers_endlayer:
*-------------------------------------------------------*
	
	move.l		.sp_layers(sp),a1
*-------------------------------------------------------*
.STF_EntityDrawLayers_start:	
*-------------------------------------------------------*
.gap:	move.l		(a1)+,d1
	bgt		.STF_EntityDrawLayers_startlayer
	beq.s		.gap

*-------------------------------------------------------*
.STF_EntityDrawLayers_finished:
*-------------------------------------------------------*
	
	lea		.sp_frame_(sp),sp
	move.l		(sp)+,a6
	
	move.l		.pctx(a6),a4
	move.l		dctx_prestorestate(a4),a4
	move.l		usp,a5
	move.l		a5,rstr_ptide(a4)
	
	unlk		a6
	popctxr		a2
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts


*-------------------------------------------------------*
	data
*-------------------------------------------------------*
	
entity_layers:
	ds.l		4
entity_layers_end:
	dc.l		-1

*-------------------------------------------------------*
	text
*-------------------------------------------------------*
	