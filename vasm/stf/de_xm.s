*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Entity processing: X major
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
_STF_EntityDrawVisibleXM_68k:
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
.sp_pxmax:		rs.l	1
.sp_savea0:		rs.l	1
.sp_drawtype:		rs.w	1
*-------------------------------------------------------*
.sp_frame_:		rs.b	0
*-------------------------------------------------------*
;	ifd		AGT_CONFIG_DEBUGDRAW
;	tst.w		_g_debugdraw
;	bne		_BLiT_EntityDebugVisibleXM_68k
;	endc
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
	move.w		ent_frx(a0),d0
	add.w		#320+(32*2),d0			; todo: hardcoded visibility margin - make configurable
	move.w		d0,.sp_pxmax(sp)
*-------------------------------------------------------*
*	initial conditions
*-------------------------------------------------------*

	move.l		dctx_clearstack_tide(a4),a5
	move.l		a5,usp

*-------------------------------------------------------*
	
	; important to begin with invalid drawtype
	; so the first drawtype will call init first

	move.w		#-1,.sp_drawtype(sp)
	
	bra		.STE_EntityDrawVisibleXM_SPRITE_done



*=======================================================*
*	dedicated iterator for [drawtype]=SPRITE
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_SPRITE_next:
*-------------------------------------------------------*

	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleXM_evaluate
	
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_SPRITE_go:
*-------------------------------------------------------*

	; access coordinates
	
;	move.w		ent_frx(a0),d1			; we already fetched this in preamble
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
.STE_EntityDrawVisibleXM_SPRITE_done:
*-------------------------------------------------------*
	move.l		ent_pnext_x(a0),a0		; next visible entity
	move.w		ent_frx(a0),d1
	cmp.w		.sp_pxmax(sp),d1
	blt		.STE_EntityDrawVisibleXM_SPRITE_next
	bra		.STE_EntityDrawVisibleXM_complete
*-------------------------------------------------------*
*	[drawtype] has suddenly changed to SPRITE
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_SPRITE_setup:
*-------------------------------------------------------*
;	jsr		_AGT_BLiT_IMSprInit			; todo: init type depends on platform & spr format
	bra		.STE_EntityDrawVisibleXM_SPRITE_go



*=======================================================*
*	dedicated iterator for [drawtype]=CUSTOM
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_CUSTOM_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleXM_evaluate
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_CUSTOM_go:
*-------------------------------------------------------*

	ifd		AGT_CONFIG_SAFETY_CHECKS
	move.l		a0,g_dbg_curr_entity
	endc

;	move.l		ent_fndraw(a0),a6		; todo
;	jsr		(a6)
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_CUSTOM_done:
*-------------------------------------------------------*
	move.l		ent_pnext_x(a0),a0		; next visible entity
	move.w		ent_frx(a0),d1
	cmp.w		.sp_pxmax(sp),d1
	blt		.STE_EntityDrawVisibleXM_CUSTOM_next
	bra		.STE_EntityDrawVisibleXM_complete
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_CUSTOM_setup:
*-------------------------------------------------------*
;	move.l		ent_fndrawinit(a0),a6		; todo
;	jsr		(a6)
	bra		.STE_EntityDrawVisibleXM_CUSTOM_go
	
*=======================================================*
*	[drawtype] has changed - perform path switch
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_evaluate:
*-------------------------------------------------------*
	move.w		d0,.sp_drawtype(sp)
	add.w		d0,d0
	add.w		d0,d0
	jmp		.types(pc,d0.w)
.types:	bra.w		.STE_EntityDrawVisibleXM_NONE_setup
	bra.w		.STE_EntityDrawVisibleXM_SPRITE_setup
	bra.w		.STE_EntityDrawVisibleXM_NONE_setup
	bra.w		.STE_EntityDrawVisibleXM_NONE_setup
	bra.w		.STE_EntityDrawVisibleXM_NONE_setup
	bra.w		.STE_EntityDrawVisibleXM_CUSTOM_setup
	bra.w		.STE_EntityDrawVisibleXM_NONE_setup



*=======================================================*
*	dedicated iterator for [drawtype]=NONE
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_NONE_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.STE_EntityDrawVisibleXM_evaluate
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_NONE_setup:
.STE_EntityDrawVisibleXM_NONE_go:
*-------------------------------------------------------*
	; nothing todo here - skip!
*-------------------------------------------------------*
.STE_EntityDrawVisibleXM_NONE_done:
*-------------------------------------------------------*
	move.l		ent_pnext_x(a0),a0		; next visible entity
	move.w		ent_frx(a0),d1
	cmp.w		.sp_pxmax(sp),d1
	blt.s		.STE_EntityDrawVisibleXM_NONE_next
	bra		.STE_EntityDrawVisibleXM_complete
	
		
*=======================================================*
.STE_EntityDrawVisibleXM_complete:
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
	
	

*-------------------------------------------------------*
_EntityInteractVisibleXM_68k:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
*	locals: shadow function arguments
*-------------------------------------------------------*
*-------------------------------------------------------*
*	locals: used by algorithm
*-------------------------------------------------------*
.sp_pxmax:		rs.w	1
.sp_pymax:		rs.w	1
.sp_savea0:		rs.l	1
*-------------------------------------------------------*
.sp_frame_:		rs.b	0
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

	move.l		ent_pnext_x(a2),a0		; pcurr
	cmp.w		ent_frx(a0),d1			; (pcurr->rx < pxmax)
	ble		.term_outer
*-------------------------------------------------------*
.next_outer:
*-------------------------------------------------------*
	move.w		ent_self(a0),d7
	bmi.s		.term_inner
*-------------------------------------------------------*
	ifd		AGT_CONFIG_VISTRIGGER
*-------------------------------------------------------*

	btst		#10,d7
	beq.s		.no_vistrig

	eor.w		#($0400|$0200),d7		; & (~EntityFlag_VISTRIG) | EntityFlag_TICKED
	move.w		d7,ent_self(a0)

	; todo: could optimise this (slightly) with inlined version
	; but it's an infrequent trigger event so not much gain...
	;
	; d0: *
	; a0: pself
	; a1: pnear
	; a3: *
	
	move.l		_g_pe_viewport,a1
	jsr		EntityLink_Tick_68k

*-------------------------------------------------------*
	endc
	
*-------------------------------------------------------*
.no_vistrig:
*-------------------------------------------------------*
	move.w		ent_frx(a0),d3
	add.w		ent_sx(a0),d3			; hmax = e.rx + e.sx;
	move.w		ent_fry(a0),d4
	cmp.w		d4,d2
	ble.s		.term_inner
	add.w		ent_sy(a0),d4			; vmax = e.ry + e.sy
	cmp.w		ent_fry(a2),d4
	ble.s		.term_inner

*-------------------------------------------------------*
	move.l		ent_pnext_x(a0),a1		; petest = e.pnext_x;
	cmp.w		ent_frx(a1),d3			; (petest->rx < hmax)
	ble.s		.term_inner

*-------------------------------------------------------*
.next_inner:
*-------------------------------------------------------*
	
	move.w		ent_fry(a1),d5
	cmp.w		d5,d4				; (petest->ry < vmax)
	ble.s		.no_contact
	add.w		ent_sy(a1),d5			; ((petest->ry + petest->sy) > e.ry)
	cmp.w		ent_fry(a0),d5
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
	move.l		ent_pnext_x(a1),a1		; petest = ptest->pnext_x;
	cmp.w		ent_frx(a1),d3
	bgt.s		.next_inner

*-------------------------------------------------------*
.term_inner:
*-------------------------------------------------------*
	move.l		ent_pnext_x(a0),a0
	cmp.w		ent_frx(a0),d1
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

