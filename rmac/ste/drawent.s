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
	.text
*-------------------------------------------------------*

	.if		^^defined AGT_CONFIG_WORLD_XMAJOR
	.globl		_BLiT_EntityDrawVisibleXM_68k
	.globl		_BLiT_EntityDeferVisibleXM_68k
	.globl		_EntityInteractVisibleXM_68k
	.globl		_gp_entity_x_head
	.globl		_gp_entity_x_aftertail
	.endif
	
	.if		^^defined AGT_CONFIG_WORLD_YMAJOR
	.globl		_BLiT_EntityDrawVisibleYM_68k
	.globl		_BLiT_EntityDeferVisibleYM_68k
	.globl		_EntityInteractVisibleYM_68k
	.globl		_gp_entity_y_head
	.globl		_gp_entity_y_aftertail
	.endif

;	.globl		_BLiT_EntityDrawLayers_68k
	.globl		_EntitySpatial_68k
	.globl		_gp_entity_tick_tail
;	.globl		_EntityLink_Tick_Ext

;	EM-optimized (but not code-generated) blitter sprite routines for <= 32-wide sprites

	.globl		_AGT_BLiT_EMSprDrawClipped
	.globl		_AGT_BLiT_EMSprDrawUnClipped
	.globl		_AGT_BLiT_EMSprInit
	.globl		_AGT_BLiT_IMSprInit
	.globl		_AGT_SlabDirtyRegion
	.globl		_AGT_BLiT_SlabInit

;	safety checks

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
	.globl		_AGT_Exception_IMSFrame
	.globl		_AGT_Exception_EMSFrame
	.globl		_AGT_Exception_EMXFrame
	.globl		_AGT_Exception_SLSFrame
	.globl		_AGT_Exception_SLRFrame
	.endif
	
;	clipping window

	.globl		_g_spr_window

	.globl		_g_linebytes

	.globl		_g_pe_viewport

	.globl		_drawcontext

	.globl		slab_precalcs
	
	.if		^^defined AGT_CONFIG_DEBUGDRAW
	.globl		_g_debugdraw
	.endif

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
_EntitySpatial_68k:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
.savea6:		ds.l	1
.save:			ds.l	2
.return:		ds.l	1
*-------------------------------------------------------*
.frame_:
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
*	locals:
*-------------------------------------------------------*
*-------------------------------------------------------*
.sp_frame_:
*-------------------------------------------------------*	
			.text
*-------------------------------------------------------*	
	movem.l		a2-a3,-(sp)
*-------------------------------------------------------*
	pushctxr	ent_spatial,a2
*-------------------------------------------------------*
	.if		^^defined AGT_CONFIG_WORLD_XMAJOR	
*-------------------------------------------------------*
	move.l		_gp_entity_x_head,a0
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
	cmp.l		_gp_entity_x_aftertail,a0
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
	.endif
*-------------------------------------------------------*
	.if		^^defined AGT_CONFIG_WORLD_YMAJOR	
*-------------------------------------------------------*
	move.l		_gp_entity_y_head,a0
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
	cmp.l		_gp_entity_y_aftertail,a0
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
	.endif
*-------------------------------------------------------*
	popctxr		a2
*-------------------------------------------------------*
	movem.l		(sp)+,a2-a3
*-------------------------------------------------------*
	rts


*-------------------------------------------------------*

	include		"ste\\b_slab.inc"

	.if		^^defined AGT_CONFIG_WORLD_XMAJOR	
	include		"ste\\de_xm.s"
	include		"ste\\de_xm_dp.s"
	.endif

	.if		^^defined AGT_CONFIG_WORLD_YMAJOR	
	include		"ste\\de_ym.s"
	include		"ste\\de_ym_dp.s"
	.endif

	include		"ste\\de_layers.s"
	
*-------------------------------------------------------*
			.text
*-------------------------------------------------------*

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
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
	.endif

*-------------------------------------------------------*
