*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Entity processing: Y major
*-------------------------------------------------------*
	
*-------------------------------------------------------*
*	Tear through entity chain in sorted order
*	deferring all visible DrawTypes
*-------------------------------------------------------*
*	Note: does not actually draw anything - sorts
*	visible items into priority buckets only.
*-------------------------------------------------------*
_BLiT_EntityDeferVisibleYM_68k:
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
.sp_yc:			ds.w	1
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
.sp_pxymax:		ds.l	1
.sp_savea0:		ds.l	1
.sp_layers:		ds.l	1
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
	pushctxr	ent_draw,a2
	link		a6,#-.frame_
	move.l		a6,-(sp)
	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
*	load C args to local frame
*-------------------------------------------------------*

*-------------------------------------------------------*
*	Reset layer chains
*-------------------------------------------------------*
	lea		entity_layers,a0
	clr.l		(a0)+
	clr.l		(a0)+
	clr.l		(a0)+
	clr.l		(a0)+

*-------------------------------------------------------*
*	load display chain terminating condition
*-------------------------------------------------------*
	move.l		_g_pe_viewport,a0
	move.w		ent_fry(a0),d0
	add.w		#240+(32*2),d0			; todo: hardcoded visibility margin - make configurable
	move.w		d0,.sp_pxymax(sp)
*-------------------------------------------------------*
*	initial conditions
*-------------------------------------------------------*

	; important to begin with invalid drawtype
	; so the first drawtype will call init

	move.w		#-1,.sp_drawtype(sp)
	lea		entity_layers,a1
	
	bra		.EntityDeferVisibleYM_NONE_done

*=======================================================*
*	[drawtype] has changed - perform path switch
*-------------------------------------------------------*
.EntityDeferVisibleYM_evaluate:
*-------------------------------------------------------*
	move.w		d0,.sp_drawtype(sp)
	add.w		d0,d0
	add.w		d0,d0
	jmp		.types(pc,d0.w)
.types:	bra.w		.EntityDeferVisibleYM_NONE_setup
	bra.w		.EntityDeferVisibleYM_DEFER_setup
	bra.w		.EntityDeferVisibleYM_DEFER_setup
	bra.w		.EntityDeferVisibleYM_DEFER_setup
	bra.w		.EntityDeferVisibleYM_DEFER_setup
	bra.w		.EntityDeferVisibleYM_DEFER_setup
	bra.w		.EntityDeferVisibleYM_DEFER_setup

*=======================================================*
*	dedicated iterator for [drawtype]=ANY
*-------------------------------------------------------*
.EntityDeferVisibleYM_DEFER_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.EntityDeferVisibleYM_evaluate
*-------------------------------------------------------*
.EntityDeferVisibleYM_DEFER_go:
*-------------------------------------------------------*

	move.w		ent_drawlayer(a0),d1
	lea		(a1,d1.w),a3			; layer head
	move.l		(a3),a2
	move.l		a2,ent_layer_(a0)		; link old head after new entity
	move.l		a0,(a3)				; update head with new entity
	
*-------------------------------------------------------*
.EntityDeferVisibleYM_DEFER_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pxymax(sp),d2
	blt		.EntityDeferVisibleYM_DEFER_next
	bra		.EntityDeferVisibleYM_complete
*-------------------------------------------------------*
.EntityDeferVisibleYM_DEFER_setup:
*-------------------------------------------------------*
	bra		.EntityDeferVisibleYM_DEFER_go
	
*=======================================================*
*	dedicated iterator for [drawtype]=NONE
*-------------------------------------------------------*
.EntityDeferVisibleYM_NONE_next:
*-------------------------------------------------------*
	move.w		ent_drawtype(a0),d0
	cmp.w		.sp_drawtype(sp),d0
	bne		.EntityDeferVisibleYM_evaluate
*-------------------------------------------------------*
.EntityDeferVisibleYM_NONE_setup:
.EntityDeferVisibleYM_NONE_go:
*-------------------------------------------------------*
	; nothing todo here - skip!
*-------------------------------------------------------*
.EntityDeferVisibleYM_NONE_done:
*-------------------------------------------------------*
	move.l		ent_pnext_y(a0),a0		; next visible entity
	move.w		ent_fry(a0),d2
	cmp.w		.sp_pxymax(sp),d2
	blt.s		.EntityDeferVisibleYM_NONE_next
;	bra		.EntityDeferVisibleYM_complete
			
*=======================================================*
.EntityDeferVisibleYM_complete:
*-------------------------------------------------------*

	bra		_BLiT_EntityDrawLayers_68k

