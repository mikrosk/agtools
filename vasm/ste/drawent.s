*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Entity drawing system:
*	- Walk viewport entity chain
*	- Interpret entity flags, states, functions
*	- Execute appropriate drawing routines
*-------------------------------------------------------*

	include		"config.inc"

	include		"blitter.inc"
	include		"emsprt.inc"
	
	include		"entity.inc"
	include		"asset.inc"

	include		"drawctx.inc"

	include		"prf.inc"
	
*-------------------------------------------------------*

within_entitydraw	set	1

*-------------------------------------------------------*
	text
*-------------------------------------------------------*

	ifd		AGT_CONFIG_WORLD_XMAJOR
	XDEF		_BLiT_EntityDrawVisibleXM_68k
	XDEF		_BLiT_EntityDeferVisibleXM_68k
	XDEF		_EntityInteractVisibleXM_68k
	XREF		_gp_entity_x_head
	XREF		_gp_entity_x_aftertail
	endc
	
	ifd		AGT_CONFIG_WORLD_YMAJOR
	XDEF		_BLiT_EntityDrawVisibleYM_68k
	XDEF		_BLiT_EntityDeferVisibleYM_68k
	XDEF		_EntityInteractVisibleYM_68k
	XREF		_gp_entity_y_head
	XREF		_gp_entity_y_aftertail
	endc

;	XDEF		_BLiT_EntityDrawLayers_68k
	XDEF		_EntityTick_68k
	XDEF		_EntitySpatial_68k
	XREF		_gp_entity_tick_tail
	XREF		_gp_entity_tick_head
	XREF		_gp_entity_tick_aftertail
;	XREF		_EntityLink_Tick_Ext

;	EM-optimized (but not code-generated) blitter sprite routines for <= 32-wide sprites

	XREF		_AGT_BLiT_EMSprDrawClipped
	XREF		_AGT_BLiT_EMSprDrawUnClipped
	XREF		_AGT_BLiT_EMSprInit
	XREF		_AGT_BLiT_IMSprInit
	XREF		_AGT_SlabDirtyRegion
	XREF		_AGT_BLiT_SlabInit

;	safety checks

	ifd		AGT_CONFIG_SAFETY_CHECKS
	XREF		_AGT_Exception_IMSFrame
	XREF		_AGT_Exception_EMSFrame
	XREF		_AGT_Exception_EMXFrame
	XREF		_AGT_Exception_SLSFrame
	XREF		_AGT_Exception_SLRFrame
	endc
	
;	clipping window

	XREF		_g_spr_window
	
	XREF		_g_linebytes
	
	XREF		_g_pe_viewport

	XREF		_drawcontext
	
	XREF		slab_precalcs

	ifd		AGT_CONFIG_DEBUGDRAW
	XREF		_g_debugdraw
	endc	

*-------------------------------------------------------*
*	Direct 68k version to support VISTRIG links
*-------------------------------------------------------*
EntityLink_Tick_68k:
*-------------------------------------------------------*
*	a0		pentity/pcurr
*	a1		pnear/psort
*	a3		*
*-------------------------------------------------------*
*	if (!psort || !(psort->f_self & EntityFlag_TICKED))
*-------------------------------------------------------*
	move.l		a1,d0
	beq.s		.not_near
.is_near:
	btst.b		#1,ent_self(a1)
	beq.s		.not_near
*-------------------------------------------------------*
.dir_tsort:
*-------------------------------------------------------*
*	if (psort->tick_priority < pcurr->tick_priority)
*-------------------------------------------------------*
	move.w		ent_tick_priority(a0),d0
	cmp.w		ent_tick_priority(a1),d0	; if (psort->tick_priority < pcurr->tick_priority)
	blt.s		.tsort_left
*-------------------------------------------------------*
.tsort_right:
*-------------------------------------------------------*
*	while (psort->pnext_tick->tick_priority < pcurr->tick_priority)
*		psort = psort->pnext_tick;	
*-------------------------------------------------------*
	move.l		a1,a3
.search_r:
	move.l		a3,a1				; psort = psort->pnext_tick;
	move.l		ent_pnext_tick(a1),a3
	cmp.w		ent_tick_priority(a3),d0
	bgt.s		.search_r
*-------------------------------------------------------*
*	// insert
*	pcurr->pnext_tick = psort->pnext_tick;
*	pcurr->pprev_tick = psort;
*	pcurr->pnext_tick->pprev_tick = pcurr;
*	psort->pnext_tick = pcurr;	
*-------------------------------------------------------*
	move.l		ent_pnext_tick(a1),a3
	move.l		a3,ent_pnext_tick(a0)
	move.l		a1,ent_pprev_tick(a0)
	move.l		a0,ent_pprev_tick(a3)
	move.l		a0,ent_pnext_tick(a1)
	rts
*-------------------------------------------------------*
.not_near:
*-------------------------------------------------------*
*	just link to end of entity chain
*-------------------------------------------------------*
	move.l		_gp_entity_tick_tail,a1
	move.w		ent_tick_priority(a0),d0
	bra.s		.tsort_left
*-------------------------------------------------------*
*	while (psort->tick_priority > pcurr->tick_priority)
*		psort = psort->pprev_tick;
*-------------------------------------------------------*
.search_l:
	move.l		ent_pprev_tick(a1),a1		; psort->pprev_tick;
.tsort_left:
	cmp.w		ent_tick_priority(a1),d0
	blt.s		.search_l
*-------------------------------------------------------*
*	// insert
*	pcurr->pnext_tick = psort->pnext_tick;
*	pcurr->pprev_tick = psort;
*	pcurr->pnext_tick->pprev_tick = pcurr;
*	psort->pnext_tick = pcurr;	
*-------------------------------------------------------*
	move.l		ent_pnext_tick(a1),a3
	move.l		a3,ent_pnext_tick(a0)
	move.l		a1,ent_pprev_tick(a0)
	move.l		a0,ent_pprev_tick(a3)
	move.l		a0,ent_pnext_tick(a1)
	rts
	


*-------------------------------------------------------*
_EntityTick_68k:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
;.savea6:		rs.l	1
.save:			rs.l	11
.return:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.l	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
*	locals:
*-------------------------------------------------------*
*-------------------------------------------------------*
.sp_frame_:		rs.l	0
*-------------------------------------------------------*	
			text
*-------------------------------------------------------*	
	movem.l		d2-d7/a2-a6,-(sp)
*-------------------------------------------------------*
	pushctxr	ent_tick,a2
*-------------------------------------------------------*
	
	; force-terminate list
	move.l		_gp_entity_tick_tail,a5
	move.l		ent_pprev_tick(a5),a4
	move.l		a4,-(sp)
	clr.l		ent_pnext_tick(a4)

*-------------------------------------------------------*
		
	subq.l		#4,sp

*-------------------------------------------------------*
	
	move.l		_gp_entity_tick_head,a6
	bra.s		.cont
	
.next:	move.l		d0,a6
	move.l		ent_fntick(a6),d0
	beq.s		.cont				; if (curr.fntick)
	move.l		d0,a0				; curr.fntick(pcurr);
	move.l		a6,(sp)
	jsr		(a0)
;	illegal
.cont:	move.l		ent_pnext_tick(a6),d0
	bne.s		.next

*-------------------------------------------------------*

.done:	addq.l		#4,sp

*-------------------------------------------------------*

	; restore list integrity
	move.l		(sp)+,a4
	move.l		_gp_entity_tick_tail,ent_pnext_tick(a4)
	
*-------------------------------------------------------*
	popctxr		a2
*-------------------------------------------------------*
	movem.l		(sp)+,d2-d7/a2-a6
*-------------------------------------------------------*
	rts



*-------------------------------------------------------*
_EntitySpatial_68k:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	3
.return:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
*	locals:
*-------------------------------------------------------*
*-------------------------------------------------------*
.sp_frame_:		rs.b	0
*-------------------------------------------------------*	
	movem.l		a2-a4,-(sp)
*-------------------------------------------------------*
	pushctxr	ent_spatial,a2
*-------------------------------------------------------*
	ifd		AGT_CONFIG_WORLD_XMAJOR	
*-------------------------------------------------------*
	move.l		_gp_entity_x_head,a0
	move.l		_gp_entity_x_aftertail,a4
*-------------------------------------------------------*
.xnext:
*-------------------------------------------------------*
	move.l		a0,a1				; pcurr->pprev_x
	move.l		ent_pnext_x(a0),a0		; pcurr
*-------------------------------------------------------*
.xresume:
*-------------------------------------------------------*
	move.w		ent_frx(a0),d0
	cmp.w		ent_frx(a1),d0
	bge.s		.xnext
*-------------------------------------------------------*
.xpushback:
*-------------------------------------------------------*
	cmp.l		a4,a0
	beq.s		.xend
*-------------------------------------------------------*
	move.l		ent_pprev_x(a0),a1
	move.l		ent_pnext_x(a0),a2
	move.l		a1,ent_pprev_x(a2)
	move.l		a2,ent_pnext_x(a1)
*-------------------------------------------------------*
;	move.w		ent_rx(a0),d0
.xscan:	move.l		ent_pprev_x(a1),a1
	cmp.w		ent_frx(a1),d0
	blt.s		.xscan
*-------------------------------------------------------*
	move.l		ent_pnext_x(a1),a3
	move.l		a3,ent_pnext_x(a0)
	move.l		a1,ent_pprev_x(a0)
	move.l		a0,ent_pprev_x(a3)
	move.l		a0,ent_pnext_x(a1)
*-------------------------------------------------------*
	move.l		a2,a0
	move.l		ent_pprev_x(a0),a1
	bra.s		.xresume
*-------------------------------------------------------*
.xend:
*-------------------------------------------------------*
	endc
*-------------------------------------------------------*
	ifd		AGT_CONFIG_WORLD_YMAJOR	
*-------------------------------------------------------*
	move.l		_gp_entity_y_head,a0
	move.l		_gp_entity_y_aftertail,a4
*-------------------------------------------------------*
.ynext:
*-------------------------------------------------------*
	move.l		a0,a1				; pcurr->pprev_x
	move.l		ent_pnext_y(a0),a0		; pcurr
*-------------------------------------------------------*
.yresume:
*-------------------------------------------------------*
	move.w		ent_fry(a0),d0
	cmp.w		ent_fry(a1),d0
	bge.s		.ynext
*-------------------------------------------------------*
.ypushback:
*-------------------------------------------------------*
	cmp.l		a4,a0
	beq.s		.yend
*-------------------------------------------------------*
	move.l		ent_pprev_y(a0),a1
	move.l		ent_pnext_y(a0),a2
	move.l		a1,ent_pprev_y(a2)
	move.l		a2,ent_pnext_y(a1)
*-------------------------------------------------------*
;	move.w		ent_ry(a0),d0
.yscan:	move.l		ent_pprev_y(a1),a1
	cmp.w		ent_fry(a1),d0
	blt.s		.yscan
*-------------------------------------------------------*
	move.l		ent_pnext_y(a1),a3
	move.l		a3,ent_pnext_y(a0)
	move.l		a1,ent_pprev_y(a0)
	move.l		a0,ent_pprev_y(a3)
	move.l		a0,ent_pnext_y(a1)
*-------------------------------------------------------*
	move.l		a2,a0
	move.l		ent_pprev_y(a0),a1
	bra.s		.yresume
*-------------------------------------------------------*
.yend:
*-------------------------------------------------------*
	endc
*-------------------------------------------------------*
	popctxr		a2
*-------------------------------------------------------*
	movem.l		(sp)+,a2-a4
*-------------------------------------------------------*
	rts


*-------------------------------------------------------*

	include		"ste\\b_slab.inc"

	ifd		AGT_CONFIG_WORLD_XMAJOR	
	ifd		AGT_CONFIG_ENTITYLAYERS
	include		"ste\\de_xm_dp.s"
	else
	include		"ste\\de_xm.s"
	endc
	endc

	ifd		AGT_CONFIG_WORLD_YMAJOR	
	ifd		AGT_CONFIG_ENTITYLAYERS
	include		"ste\\de_ym_dp.s"
	else
	include		"ste\\de_ym.s"
	endc
	endc
	
	ifd		AGT_CONFIG_ENTITYLAYERS
	include		"ste\\de_layers.s"
	endc

*-------------------------------------------------------*
			text
*-------------------------------------------------------*

	ifd		AGT_CONFIG_SAFETY_CHECKS
_AGT_Exception_IMSFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_IMSFrame
	; noreturn
_AGT_Exception_EMSFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_EMSFrame
	; noreturn
_AGT_Exception_EMXFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_EMXFrame
	; noreturn
_AGT_Exception_SLSFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_SLSFrame
	; noreturn
_AGT_Exception_SLRFrame_68k:
	move.l		a0,-(sp)
	move.l		d0,-(sp)
	jsr		_AGT_Exception_SLRFrame
	; noreturn
	endc

*-------------------------------------------------------*