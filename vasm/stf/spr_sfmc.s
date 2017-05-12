*-------------------------------------------------------*
*	(A)tari (G)ame (T)ools / dml
*-------------------------------------------------------*
*	Realtime-shifted 4bpl sprites
*	Optimized version (about 50k of code)
*-------------------------------------------------------*

	include		"stf/spr_svmc.inc"
	include		"stf/spr_sfmc.inc"

*-------------------------------------------------------*
*	Optimized case for medium/larger objects:
*-------------------------------------------------------*
*	- 4 bitplanes
*	- top,right,bottom clipping only (no FISR/left)
*	- fixed shifting
*	- auto masking with keycolour=0
*-------------------------------------------------------*
_AGT_STF_ISSprDrawClipped:
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

	include		"stf/spr_sfmc_clp.s"
	
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
*	Optimized case for medium/larger objects:
*-------------------------------------------------------*
*	- 4 bitplanes
*	- top,right,bottom clipping only (no FISR/left)
*	- fixed shifting
*	- auto masking with keycolour=0
*-------------------------------------------------------*
_AGT_STF_ISSprDrawUnClipped:
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

	include		"stf/spr_sfmc_unc.s"

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
*	common implementation for clip/no-clip paths
*-------------------------------------------------------*
STF_ISSprOpt_impl_:
*-------------------------------------------------------*

	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		usp,a5
	move.l		a5,a6
	endc
	endc
	
	; dst pixelword xoffset
	moveq		#-16,d0
	and.w		d1,d0
	lsr.w		#1,d0
	
	ifne		(enable_restore)
	ifne		(use_sprite_pagerestore)	
	move.w		d2,(a5)+
	move.w		d0,(a5)+
	endc	
	endc
	
	add.w		d0,a1
	
	; dst yoffset (line)
;	mulu.w		#linebytes,d2
	mulu.w		_g_linebytes,d2
	add.l		d2,a1
	
	; find destination word width of shifted sprite
;	moveq		#16-1,d6
	move.w		d6,d7

	and.w		d1,d6				; rshift
	move.l		d6,d1

	add.w		d3,d7				; w+round
;	add.w		spf_w(a0),d7			; w+round
;	move.w		spf_w(a0),d7			; w
;	add.w		#16-1,d7			; w+round

	moveq		#-16,d0	
	add.w		d7,d6				; rshift+w+round
	and.w		d0,d6				; wceil(rshift+w)

;	moveq		#8,d1

	; shift tab scaling
;	add.w		d1,d1

	; branch for FSR/NFSR case	
	and.w		d7,d0
	; final height
	move.w		d4,d7				; h
	; NFSR test
	sub.w		d6,d0
	beq		efsr_case

*-------------------------------------------------------*
nfsr_case:
*-------------------------------------------------------*

	include		"stf/spr_sfmc_nfsr.s"
		
*-------------------------------------------------------*
;nfsr_spr_done:
*-------------------------------------------------------*
;	if		(use_sprite_saverestore)
;	move.l		a6,(a5)+
;	move.l		a5,_drawcontext+dctx_clearstack_tide
;	endc
*-------------------------------------------------------*
;	move.l		(sp)+,a6
;	unlk		a6
;	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
;	rts


*-------------------------------------------------------*
efsr_case:
*-------------------------------------------------------*

	include		"stf/spr_sfmc_efsr.s"

*-------------------------------------------------------*
;efsr_spr_done:
*-------------------------------------------------------*
;	if		(use_sprite_saverestore)
;	move.l		a6,(a5)+
;	move.l		a5,_drawcontext+dctx_clearstack_tide
;	endc
*-------------------------------------------------------*
;	move.l		(sp)+,a6
;	unlk		a6
;	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
;	rts
	
