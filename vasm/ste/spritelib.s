*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Blitter 4 bitplane sprites:
*	- Optional auto-clearing mechanism
*	- Optional clipping paths
*	- Compatible with formats produced by AGTCut
*-------------------------------------------------------*

	COMMENT		HEAD=7
	OPT		P=68000,NOTYPE
	OPT		W+
	OPT		OW+

*-------------------------------------------------------*

	include		"config.inc"

	include		"blitter.inc"
	include		"emsprt.inc"
	
	include		"asset.inc"

	include		"drawctx.inc"

*-------------------------------------------------------*
	text
*-------------------------------------------------------*

; enable this for automatic clearing of sprites in
; reverse drawing order, to restore background area


;	general purpose sprite routines
	XDEF		_AGT_BLiT_IMSprDrawClipped
	XDEF		_AGT_BLiT_IMSprDrawUnClipped
	XDEF		_AGT_BLiT_IMSprInit

	XDEF		_AGT_MarkDirtyRectangle

;	EM-optimized sprite routines for <= 32-wide sprites

	XDEF		_AGT_BLiT_EMSprDrawClipped
	XDEF		_AGT_BLiT_EMSprDrawUnClipped
	XDEF		_AGT_BLiT_EMSprInit
	
	XDEF		_AGT_BLiT_EMSprDrawString

;	slab routines for large objects
;	XDEF		_STE_SlabNoClipGeneral4BPL
	XDEF		_AGT_BLiT_SlabDraw
	XDEF		_AGT_SlabDirtyRegion
	XDEF		_AGT_BLiT_SlabSetupOnce
	XDEF		_AGT_BLiT_SlabInit

	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
;	sprite clearing mechanism
	XDEF		_AGT_PlayfieldRestoreInit
	XDEF		_AGT_PlayfieldRestoreReset
	XDEF		_AGT_BLiT_PlayfieldRestore
	endc

;	clipping window
	XDEF		_g_spr_window
	XDEF		_g_spr_window_x1
	XDEF		_g_spr_window_y1
	XDEF		_g_spr_window_x2
	XDEF		_g_spr_window_y2
	
	
	XREF		_g_linebytes
	
	XREF		_ascii2font
	
	XDEF		slab_precalcs


*-------------------------------------------------------*
_AGT_BLiT_IMSprInit:
*-------------------------------------------------------*
	move.w		#-1,BLiTEM2.w

	move.b		#2,BLiTHOP.w			; HOP

	move.w		#8,BLiTDXI.w			; dst xinc

	; planes (5, 1st being mask) x words x lines
	move.w		#10,BLiTSXI.w			; src xinc
	
	rts



*-------------------------------------------------------*
*	Most general (and most expensive) case:
*-------------------------------------------------------*
*	- 4 bitplanes
*	- full playfield window clipping
*	- masking with explicit keycolour/mask
*-------------------------------------------------------*
_AGT_BLiT_IMSprDrawClipped:
_AGT_BLiT_IMSprDrawUnClipped:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.psps:			rs.l	1
.pdst:			rs.l	1
.sprframe:		rs.l	1
.xpos:			rs.l	1
.ypos:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	subq.l		#4,sp
*-------------------------------------------------------*
	move.l		.psps(a6),a0
	move.l		.pdst(a6),a1
	move.w		.xpos+2(a6),d1
	move.w		.ypos+2(a6),d2
;	sub.w		_g_xpreshift,d1


	; access sprite frame
	
	move.w		.sprframe+2(a6),d0

enable_restore	set	1
enable_clipping	set	1
use_hog		set	0
	
	include 	"ste/b_spr.s"

	addq.l		#4,sp
	move.l		(sp)+,a6
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts















*-------------------------------------------------------*
_AGT_BLiT_EMSprInit:
*-------------------------------------------------------*

	move.w		#-1,BLiTEM2.w

	move.b		#2,BLiTHOP.w			; HOP
	move.b		#3,BLiTLOP.w			; LOP = SRC

	; words x planes x lines
		
	move.w		#2,BLiTSXI.w

	move.w		#8,BLiTDXI.w

	move.w		#2,BLiTSYI.w			; changed by clipping, must be reset

	rts


*-------------------------------------------------------*
*	Most general (and most expensive) case:
*-------------------------------------------------------*
*	- 4 bitplanes
*	- full playfield window clipping
*	- masking with explicit keycolour/mask
*-------------------------------------------------------*
_AGT_BLiT_EMSprDrawClipped:
_AGT_BLiT_EMSprDrawUnClipped:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.psps:			rs.l	1
.pdst:			rs.l	1
.plinetable:		rs.l	1
.sprframe:		rs.l	1
.xpos:			rs.l	1
.ypos:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.sp_skew:		rs.w	1
.sp_maskwidth:		rs.w	1
.sp_flags:		rs.w	1
.sp_linetable:		rs.l	1
.sp_frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
	move.l		.psps(a6),a0
	move.l		.pdst(a6),a1
	move.w		.xpos+2(a6),d1
	move.w		.ypos+2(a6),d2
	
	move.l		.plinetable(a6),.sp_linetable(sp)

	; access sprite frame
	
	move.w		.sprframe+2(a6),d0

	move.l		_drawcontext+dctx_clearstack_tide,a5
	move.l		a5,usp

*-------------------------------------------------------*

enable_restore	set	1
enable_clipping	set	1

	include		"ste/b_emx_spr.s"
	
*-------------------------------------------------------*

	move.l		usp,a5
	move.l		a5,_drawcontext+dctx_clearstack_tide

	lea		.sp_frame_(sp),sp
	move.l		(sp)+,a6
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts











*-------------------------------------------------------*
_AGT_BLiT_EMSprDrawString:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.pstr:			rs.l	1
.psps:			rs.l	1
.pdst:			rs.l	1
.plinetable:		rs.l	1
.xpos:			rs.l	1
.ypos:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.sp_skew:		rs.w	1
.sp_maskwidth:		rs.w	1
.sp_flags:		rs.w	1
.sp_linetable:		rs.l	1
*-------------------------------------------------------*
.sp_store:		rs.l	1
.sp_psps:		rs.l	1
.sp_pdst:		rs.l	1
.sp_x:			rs.w	1
.sp_y:			rs.w	1
;.sp_z:			rs.w	1
.sp_frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	lea		-.sp_frame_(sp),sp
*-------------------------------------------------------*
	jsr		_AGT_BLiT_EMSprInit
;	jsr		_AGT_BLiT_IMSprInit
*-------------------------------------------------------*	
	move.l		.pstr(a6),a0
	move.l		a0,usp
*-------------------------------------------------------*	
	move.l		.psps(a6),.sp_psps(sp)
	move.l		.pdst(a6),.sp_pdst(sp)
	move.w		.xpos+2(a6),.sp_x(sp)
	move.w		.ypos+2(a6),.sp_y(sp)
	move.l		.plinetable(a6),.sp_linetable(sp)
*-------------------------------------------------------*
	moveq		#0,d0
;	clr.w		.sp_z(sp)
	bra		.fetch
*-------------------------------------------------------*
.next:
*-------------------------------------------------------*
	move.w		.sp_x(sp),d1
	moveq		#8,d3
	add.w		d1,d3
	move.w		d3,.sp_x(sp)
	
;	btst		#3,d3
;	bne		.fetch
	
;	moveq		#15,d4
;	and.w		d3,d4
;	bne		.fetch
	
;	addq.w		#1,.sp_z(sp)
	
	
*-------------------------------------------------------*
	cmp.b		#' ',d0
	beq		.fetch
*-------------------------------------------------------*	
	move.w		.sp_y(sp),d2
	move.l		.sp_psps(sp),a0
	move.l		.sp_pdst(sp),a1
*-------------------------------------------------------*

	; access sprite frame

	lea		_ascii2font,a4
	move.b		(a4,d0.w),d0
	
	; draw cells, without background-restore per cell
	
enable_restore	set	0
enable_clipping	set	1
;use_hog		set	1

;	note: don't bother with EMX codegen optimization for fonts - too expensive. assume standard EM path.

	moveq		#0,d7

	include		"ste/b_em_spr.s"
;	include 	"ste/b_spr.s"

	moveq		#0,d0

*-------------------------------------------------------*
.fetch:
*-------------------------------------------------------*
	move.l		usp,a3
	move.b		(a3)+,d0
	move.l		a3,usp
	bne		.next
*-------------------------------------------------------*	
.sstop:	
*-------------------------------------------------------*
	lea		.sp_frame_(sp),sp
	move.l		(sp)+,a6
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts


;	include		"ste/spr2bpl.s"



*-------------------------------------------------------*
*
*-------------------------------------------------------*
*
*-------------------------------------------------------*
_AGT_BLiT_SlabDraw:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.psls:			rs.l	1
.pdst:			rs.l	1
.slabframe:		rs.l	1
.xpos:			rs.l	1
.ypos:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.sp_yoff:		rs.w	1
;.sp_ycrop:		rs.w	1
.sp_yrem:		rs.w	1
.sp_save_usp:		rs.l	1
.sp_frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	subq.l		#.sp_frame_,sp
*-------------------------------------------------------*
	move.l		.psls(a6),a0
	move.l		.pdst(a6),a1
	move.w		.xpos+2(a6),d1
	move.w		.ypos+2(a6),d2
;	sub.w		_g_xpreshift,d1
*-------------------------------------------------------*

	move.w		.slabframe+2(a6),d0
	moveq		#0,d7
	
use_slab_restore	set	0
enable_restore	set	1
preserve_usp	set	0
		
	include		"ste/b_slab.s"

*-------------------------------------------------------*
	addq.l		#.sp_frame_,sp
	move.l		(sp)+,a6
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts


*-------------------------------------------------------*
*
*-------------------------------------------------------*
*
*-------------------------------------------------------*
_AGT_SlabDirtyRegion:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.psls:			rs.l	1
.pdst:			rs.l	1
.slabframe:		rs.l	1
.xpos:			rs.l	1
.ypos:			rs.l	1
*-------------------------------------------------------*
.frame_:		rs.b	0
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.sp_yoff:		rs.w	1
.sp_yrem:		rs.w	1
.sp_save_usp:		rs.l	1
.sp_frame_:		rs.b	0
*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
	subq.l		#.sp_frame_,sp
*-------------------------------------------------------*
	move.l		.psls(a6),a0
	move.l		.pdst(a6),a1
	move.w		.xpos+2(a6),d1
	move.w		.ypos+2(a6),d2
*-------------------------------------------------------*
	jsr		_AGT_BLiT_SlabInit
	
	move.l		_drawcontext+dctx_clearstack_tide,a5
	move.l		a5,usp


	move.w		.slabframe+2(a6),d0
	moveq		#0,d7
	
use_slab_restore	set	1
enable_restore		set	1
preserve_usp		set	0
		
	include		"ste/b_slabres.s"

	move.l		usp,a5
	move.l		a5,_drawcontext+dctx_clearstack_tide

*-------------------------------------------------------*
	addq.l		#.sp_frame_,sp
	move.l		(sp)+,a6
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts

*-------------------------------------------------------*
*
*-------------------------------------------------------*
*
*-------------------------------------------------------*
_AGT_BLiT_SlabInit:
*-------------------------------------------------------*
	move.b		#2,BLiTHOP.w			; HOP = SRC
	move.w		#-1,BLiTEM2.w			; em2
	move.w		#2,BLiTSXI.w			; src xinc
	move.w		#8,BLiTDXI.w			; dst xinc
	move.w		#2,BLiTSYI.w			; src yinc, unclipped
*-------------------------------------------------------*
	rts
	
*-------------------------------------------------------*
*
*-------------------------------------------------------*
*
*-------------------------------------------------------*
_AGT_BLiT_SlabSetupOnce:
*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
.savea6:		rs.l	1
.save:			rs.l	10
.return:		rs.l	1
*-------------------------------------------------------*
.frame_:		rs.l	0
*-------------------------------------------------------*

*-------------------------------------------------------*
	movem.l		d2-d7/a2-a5,-(sp)
	link		a6,#-.frame_
	move.l		a6,-(sp)
*-------------------------------------------------------*
	lea		slab_precalcs,a5	
	moveq		#0,d1
	moveq		#0,d4
*-------------------------------------------------------*
.xsl:
.xol:	movem.l		d1/d4,-(sp)
	bsr		precalculate_spandata	
	movem.l		(sp)+,d1/d4
*-------------------------------------------------------*
	move.b		spandata_skew,(a5)+
	move.b		spandata_wordwidth,(a5)+
	move.w		spandata_em1,(a5)+
	move.w		spandata_em3,(a5)+
	move.w		spandata_dyinc,(a5)+
*-------------------------------------------------------*
	addq.w		#1,d1	
	and.w		#16-1,d1
	bne.s		.xol
*-------------------------------------------------------*
	addq.w		#1,d4
	cmp.w		#256,d4
	ble.s		.xsl
*-------------------------------------------------------*
.end:	move.l		(sp)+,a6
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts


precalculate_spandata:
	move.w		d1,d2
	move.w		d1,d3

;	skew/nfsr	1
;	width		1
;	em1		2
;	em3		2
;	dyi		2
	
;	lea		ssb_data-6(a0),a3
;	move.l		a3,$ffff8a24.w			; src
	
;	word offset	
	moveq		#-16,d5
	and.w		d3,d5				; ceil16(x)
	eor.w		d5,d3				; pixel offset
	lsr.w		d5	
	lea		(a1,d5.w),a3
;	move.l		a3,$ffff8a32.w			; dst
	
;	skew
;	move.b		d3,$ffff8a3d.w
	move.b		d3,spandata_skew

	; find destination word width of shifted sprite
	
	moveq		#16-1,d5
	add.w		d4,d5				; w+round	
	moveq		#-16,d0	
	move.w		d3,d6
	add.w		d5,d6				; rshift+w+round
	and.w		d0,d6				; wceil(rshift+w)

.STE_SlabNoClipGeneral4BPL_NFSR_test:

	; NFSR test

;	or.b		#$40,$ffff8a3d.w		; BLIT: nfsr
	
	and.w		d5,d0
	sub.w		d6,d0
	beq.s		.STE_SlabNoClipGeneral4BPL_EFSR


*-------------------------------------------------------*
.STE_SlabNoClipGeneral4BPL_NFSR:
*-------------------------------------------------------*
	; NFSR
;	or.b		#$40,$ffff8a3d.w		; BLIT: nfsr
	or.b		#$40,spandata_skew

.STE_SlabNoClipGeneral4BPL_EFSR:

	; dst skip adjust by dst width	
;	move.w		_g_linebytes,a4
	lsr.w		#1,d6
;	sub.w		d6,a4

	; final height
;	move.w		d4,d7				; h

;	move.w		a3,$ffff8a22.w			; src yinc


	; shifted left-edge mask
	moveq		#-1,d0
	lsr.w		d3,d0

	; shifted right-edge mask	
	add.w		d4,d2
	subq.w		#1,d2
	and.w		#16-1,d2
	addq.w		#1,d2
	moveq		#-1,d4
	lsr.w		d2,d4
	not.w		d4
	
	;moveq		#-1,d4

	; word-width				
	lsr.w		#(4-1),d6			; >>4 *2
;	move.w		d6,$ffff8a36.w			; BLIT: width
	move.b		d6,spandata_wordwidth

	cmp.w		#1,d6
	bne.s		.multi
	and.w		d4,d0
.multi:	
;	move.w		d0,$ffff8a28.w	
;	move.w		d4,$ffff8a2c.w

	move.w		d0,spandata_em1
	move.w		d4,spandata_em3
	

*-------------------------------------------------------*
*	common to mask and colour
*-------------------------------------------------------*

;	addq.l		#8,a4
;	move.w		a4,$ffff8a30.w			; dst yinc = -dstwidth

	neg.w		d6
	asl.w		#3,d6
	addq.w		#8,d6
	addq.w		#2,d6
;	move.w		d6,$ffff8a30.w			; dst yinc = -dstwidth

	move.w		d6,spandata_dyinc
	rts


*-------------------------------------------------------*

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
	
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		_drawcontext+dctx_clearstack_tide,a5
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
	
	ifne		(use_sprite_pagerestore)	
	move.w		d2,(a5)+			; y
	move.w		d1,(a5)+			; xo

	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d4,(a5)+			; lines
	endc	


	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		d5,(a5)+
	move.l		a5,_drawcontext+dctx_clearstack_tide
	endc

*-------------------------------------------------------*
	unlk		a6
	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
	rts


*-------------------------------------------------------*

	include		"ste/spr_clear.s"
	
*-------------------------------------------------------*
			bss
*-------------------------------------------------------*
	
_g_spr_window:
_g_spr_window_x1:	ds.w		1
_g_spr_window_y1:	ds.w		1
_g_spr_window_x2:	ds.w		1
_g_spr_window_y2:	ds.w		1

spandata_skew:		ds.b		1
spandata_wordwidth:	ds.b		1
spandata_em1:		ds.w		1
spandata_em3:		ds.w		1
spandata_dyinc:		ds.w		1

slab_precalcs:		ds.b		8*(257*16)
	
*-------------------------------------------------------*
			text
*-------------------------------------------------------*