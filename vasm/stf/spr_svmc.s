*-------------------------------------------------------*
*	(A)tari (G)ame (T)ools / dml
*-------------------------------------------------------*
*	Realtime-shifted 4bpl sprites
*	Small-footprint version
*-------------------------------------------------------*

	include		"stf/spr_svmc.inc"

*-------------------------------------------------------*
*	Most general (and most expensive) case:
*-------------------------------------------------------*
*	- 4 bitplanes
*	- full playfield window clipping
*	- variable software shifting
*	- auto masking with keycolour=0
*-------------------------------------------------------*
_AGT_STF_ISSprDrawClipped:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.pctx:			rs.l	1
.psps:			rs.l	1
.sprframe:		rs.l	1
.xpos:			rs.l	1
.ypos:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
;.sp_skew:		rs.w	1
;.sp_maskwidth:		rs.w	1
;.sp_flags:		rs.w	1
.sp_linetable:		rs.l	1
.sp_frame_:		rs.b	0
*-------------------------------------------------------*
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
	move.l		.pctx(a6),a4
	move.l		dctx_framebuffer(a4),a1
	move.l		dctx_lineidx(a4),.sp_linetable(sp)

	move.l		.psps(a6),a0
	move.w		.xpos+2(a6),d1
	move.w		.ypos+2(a6),d2
	sub.w		dctx_snapadjy(a4),d2	

	; access sprite frame
	
	move.w		.sprframe+2(a6),d0
	moveq		#0,d7

;	move.l		dctx_clearstack_tide(a4),a5
	move.l		dctx_prestorestate(a4),a5
	move.l		rstr_ptide(a5),a5
	move.l		a5,usp
	
	sub.w		_g_xpreshift,d1

	; access sprite frame
	
	move.w		.sprframe+2(a6),d0
	
	; initial dst skip
	
	move.w		_g_linebytes,a4
	
*-------------------------------------------------------*

	include		"stf/spr_svmc_clp.s"

*-------------------------------------------------------*

	lea		.sp_frame_(sp),sp
	move.l		(sp)+,a6
	
	move.l		.pctx(a6),a4
	move.l		dctx_prestorestate(a4),a4
	move.l		usp,a5
	move.l		a5,rstr_ptide(a4)
	
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts


*-------------------------------------------------------*
*	Optimized case for medium/smaller objects:
*-------------------------------------------------------*
*	- 4 bitplanes
*	- no window clipping
*	- variable software shifting
*	- auto masking with keycolour=0
*-------------------------------------------------------*
_AGT_STF_ISSprDrawUnClipped:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.pctx:			rs.l	1
.psps:			rs.l	1
.sprframe:		rs.l	1
.xpos:			rs.l	1
.ypos:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
;.sp_skew:		rs.w	1
;.sp_maskwidth:		rs.w	1
;.sp_flags:		rs.w	1
.sp_linetable:		rs.l	1
.sp_frame_:		rs.b	0
*-------------------------------------------------------*
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
	move.l		.pctx(a6),a4
	move.l		dctx_framebuffer(a4),a1
	move.l		dctx_lineidx(a4),.sp_linetable(sp)

	move.l		.psps(a6),a0
	move.w		.xpos+2(a6),d1
	move.w		.ypos+2(a6),d2
	sub.w		dctx_snapadjy(a4),d2	

	; access sprite frame
	
	move.w		.sprframe+2(a6),d0
	moveq		#0,d7

;	move.l		dctx_clearstack_tide(a4),a5
	move.l		dctx_prestorestate(a4),a5
	move.l		rstr_ptide(a5),a5
	move.l		a5,usp
	
	sub.w		_g_xpreshift,d1
		
	; access sprite frame
	
	move.w		.sprframe+2(a6),d0

*-------------------------------------------------------*

	; dst skip: a4

	move.w		_g_linebytes,a4

	include		"stf/spr_svmc_unc.s"
			
*-------------------------------------------------------*

	lea		.sp_frame_(sp),sp
	move.l		(sp)+,a6
	
	move.l		.pctx(a6),a4
	move.l		dctx_prestorestate(a4),a4
	move.l		usp,a5
	move.l		a5,rstr_ptide(a4)
	
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts
	