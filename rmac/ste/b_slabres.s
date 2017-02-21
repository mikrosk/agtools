*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	SLAB sprite restore implementation
*	allows object to be cleared using approximated
*	version of shape (precise to 16 pixel bound)
*	requiring far fewer spanblock regions
*-------------------------------------------------------*
*	max source xsize: 256
*	max source ysize: viewport clipper ysize
*-------------------------------------------------------*

_use_slab_restore_	set	(use_sprite_saverestore|use_sprite_pagerestore)


*-------------------------------------------------------*
.macro	doregions
*-------------------------------------------------------*

*-------------------------------------------------------*
.start_spanblock\~:	
*-------------------------------------------------------*

;	a0		*
;	a1		dst line
;	a2		spanblock offsets
;	a3		slab_precalcs
;	a4		linebytes
;	a5		[clearing]
;	a6		spanblock index


;	line skip
	move.w		(a0)+,d4
	beq.s		.no_lineskip_init\~

*-------------------------------------------------------*
*	skip empty lines
*-------------------------------------------------------*

;	todo: data setup pass
	mulu.w		_g_linebytes,d4
	add.l		d4,a1
	moveq		#0,d4
	
*-------------------------------------------------------*
.no_lineskip_init\~:
*-------------------------------------------------------*

	move.w		(a0)+,d3
	move.w		(a0)+,d4
	add.w		d1,d3

;	skew/nfsr	1
;	width		1
;	em1		2
;	em3		2
;	dyi		2

*-------------------------------------------------------*
*	single span or rectangular block of spans?
*-------------------------------------------------------*
	tst.w		d6
	beq		.do_sgl\~
	bra		.do_block\~
	

*-------------------------------------------------------*
.cont_spanblock\~:
*-------------------------------------------------------*

	move.l		a6,a0				; spanblock index
	add.w		(a2)+,a0			; next spanblock

;	a0		*
;	a1		dst line
;	a2		spanblock offsets
;	a3		slab_precalcs
;	a4		linebytes
;	a5		[clearing]
;	a6		spanblock index


;	line skip
	move.w		(a0)+,d4
	beq.s		.no_lineskip\~

*-------------------------------------------------------*
*	skip empty lines
*-------------------------------------------------------*

;	todo: data setup pass
	mulu.w		_g_linebytes,d4
	add.l		d4,a1
	moveq		#0,d4
	
*-------------------------------------------------------*
.no_lineskip\~:
*-------------------------------------------------------*

	move.w		(a0)+,d3
	move.w		(a0)+,d4
	add.w		d1,d3

;	skew/nfsr	1
;	width		1
;	em1		2
;	em3		2
;	dyi		2

*-------------------------------------------------------*
*	single span or rectangular block of spans?
*-------------------------------------------------------*
	move.w		ssb_ys-6(a0),d6
	beq.s		.do_sgl\~
*-------------------------------------------------------*
*	block of related, unique spans
*-------------------------------------------------------*
.do_block\~:
*-------------------------------------------------------*

	.if		(use_slab_restore)
	.if		(use_sprite_pagerestore)
	move.l		a5,d2
	move.w		ssb_y-6(a0),d5
	add.w		.sp_yoff(sp),d5
	bpl.s		.qqb\~
	clr.w		d5
.qqb\~:	move.w		d5,(a5)+
	.endif
	.endif

*-------------------------------------------------------*
;	dst addr xoffset	
	moveq		#-16,d5
	and.l		d3,d5				; ceil16(x)
	eor.w		d5,d3				; pixel offset
	lsr.w		d5	
	.if		(use_slab_restore)
	.if		(use_sprite_pagerestore)	
	move.w		d5,(a5)+
	.endif	
	.endif
	add.l		a1,d5
*-------------------------------------------------------*
;	lsl.w		#4,d4
	or.w		d3,d4
	lsl.w		#3,d4

;	dst word count for spanblock at this xoffset
	move.b		1(a3,d4.l),d3

*-------------------------------------------------------*

	.if		(use_slab_restore)
	.if		(use_sprite_pagerestore)

	; clearing record

	move.l		d5,(a5)+			; clear addr

	add.w		d3,d3
	move.w		d3,d4

	add.w		d4,d4
	add.w		d4,d4
	move.w		a4,d0
	sub.w		d4,d0

	move.w		d0,(a5)+			; dst skip
	move.w		d3,(a5)+			; word width*2

	moveq		#1,d4
	add.w		d6,d4
	move.w		d4,(a5)+			; lines

	move.l		d2,(a5)+

	.endif
	.endif

*-------------------------------------------------------*
	move.w		a4,d0
	addq.w		#1,d6
	mulu.w		d6,d0
	add.l		d0,a1

*-------------------------------------------------------*
.spanloop_final\~:
*-------------------------------------------------------*

*-------------------------------------------------------*
	dbra		d7,.cont_spanblock\~
*-------------------------------------------------------*
	bra.s		.end\~

*-------------------------------------------------------*
*	single span
*-------------------------------------------------------*
.do_sgl\~:
*-------------------------------------------------------*

	.if		(use_slab_restore)
	.if		(use_sprite_pagerestore)
	move.l		a5,d2
	move.w		ssb_y-6(a0),d5
	add.w		.sp_yoff(sp),d5
	bpl.s		.qqs\~
	clr.w		d5
.qqs\~:	move.w		d5,(a5)+
	.endif
	.endif

*-------------------------------------------------------*
;	dst addr xoffset	
	moveq		#-16,d5
	and.l		d3,d5				; ceil16(x)
	eor.w		d5,d3				; pixel offset
	lsr.w		d5
	.if		(use_slab_restore)
	.if		(use_sprite_pagerestore)	
	move.w		d5,(a5)+
	.endif	
	.endif
	add.l		a1,d5
*-------------------------------------------------------*
;	lsl.w		#4,d4
	or.w		d3,d4
	lsl.w		#3,d4
	
;	dst word count for spanblock at this xoffset
	move.b		1(a3,d4.l),d3

*-------------------------------------------------------*

	.if		(use_slab_restore)
	.if		(use_sprite_pagerestore)

	; clearing record

	move.l		d5,(a5)+			; clear addr

	add.w		d3,d3
	move.w		d3,d4

	add.w		d4,d4
	add.w		d4,d4
	move.w		a4,d0
	sub.w		d4,d0

	move.w		d0,(a5)+			; dst skip
	move.w		d3,(a5)+			; word width*2

	move.w		#1,(a5)+			; lines

	move.l		d2,(a5)+
		
	.endif
	.endif	
	
*-------------------------------------------------------*

	add.w		a4,a1
	
*-------------------------------------------------------*
;	hide the dbra behind the blitter restart
	dbra		d7,.cont_spanblock\~

*=======================================================*

*-------------------------------------------------------*
.end\~:

*-------------------------------------------------------*
	endm
*-------------------------------------------------------*




*=======================================================*
*	entrypoint
*=======================================================*

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
;	sanity check on frame index!
	cmp.w		sps_framecount(a0),d0
	.if		(within_entitydraw)
	bhs		_AGT_Exception_SLRFrame_68k
	.else
	blo.s		.slr_frameok
.slr_framefault:
	not.w		$ffff8240.w
	bra.s		.slr_framefault
.slr_frameok:
	.endif
	.endif
	
	; access sprite frame table
	
	add.w		d0,d0				; todo: game layer can prescale this
	add.w		d0,d0
	move.l		sls_frameindex(a0,d0.w),a0	; access sprite frame data
			
	; remaining spanblocks
		
	moveq		#-1,d7
	add.w		slf_spanblocks(a0),d7
	
	; frame coord. fields
	
	move.w		slf_w(a0),d3			; w
	move.w		slf_h(a0),d4			; h
	add.w		slf_xo(a0),d1			; +xo
	add.w		slf_yo(a0),d2			; +yo

	move.w		d2,.sp_yoff(sp)

	; relative base for offset tables
	
	lea		slf_indexbase(a0),a6		; indexbase
	
	move.l		a6,a2

	cmp.w		_drawcontext+dctx_clipwindow_y1,d2
	bpl.s		.slr_clip_safetop

*-------------------------------------------------------*
.slr_clip_top:
*-------------------------------------------------------*
*	top clipping is a bit complex, but relative to 
*	drawing the object and limits on slab drawing
*	count generally, the overhead is neglegible	
*-------------------------------------------------------*

	sub.w		_drawcontext+dctx_clipwindow_y1,d2
	neg.w		d2
	cmp.w		d4,d2
	bge		.slr_degen			; completely cropped?
	
	move.w		d2,d5				; total amount to crop

	lsl.w		#3,d2
	lea		(a6,d2.w),a0

	add.w		(a0)+,a2			; slc_tyc_offset : first spanblock record = [clipper + yc]
	move.w		(a0)+,d7			; slc_tyc_remain : spanblocks remaining for this yc
	subq.w		#1,d7

	moveq		#0,d4
;	move.w		(a0),d4				; slc_tyc_data
;	lea		(a6,d4.l),a3			; +64k reach for span pixel data
	

; A     0
; B ____4____
; B          8
; B
; C     16

;	address first spanblock record

	move.l		a6,a0				; spanblock index
	add.w		(a2)+,a0			; next spanblock

;	real cropping is (y+dy) since dy = optional empty lines at start of block

	sub.w		ssb_y(a0),d5
	sub.w		ssb_dy(a0),d5

;	crop first block size by yc-(y+dy)

	move.w		ssb_ys(a0),d6
	sub.w		d5,d6

;	index source by yc-(y+dy) pixeldata lines

;	lsl.w		#3,d5
;	mulu.w		ssb_dxw(a0),d5
;	add.l		d5,a3

;	y origin = clipping edge

	move.w		_drawcontext+dctx_clipwindow_y1,d2
	
;	slab size is restricted - if clipped at top, 
;	can't also be clipped at bottom	

	bra		.slr_clip_safebot
						
*-------------------------------------------------------*
.slr_clip_safetop:
*-------------------------------------------------------*
*	not clipped at window top
*	may still be clipped at window bottom
*-------------------------------------------------------*

;	access first spanblock reference

	add.w		slc_tyc_offset(a6),a2

;	access spanblock record

	move.l		a6,a0				; index base
	add.w		(a2)+,a0

;	first block is uncropped - draw all lines

	move.w		ssb_ys(a0),d6

;	first block uses unadjusted start of pixeldata
	
;	move.l		ssb_data(a0),a3

;	test window bottom

	add.w		d2,d4
	cmp.w		_drawcontext+dctx_clipwindow_y2,d4
	ble		.slr_clip_safebot

*-------------------------------------------------------*
.slr_clip_bot:
*-------------------------------------------------------*
*	bottom clipping is achieved by drawing
*	(spanblocks-nc) normally and then one final
*	block with cropped height, if required
*-------------------------------------------------------*

;	test for fully clipped out

	cmp.w		_drawcontext+dctx_clipwindow_y2,d2
	bge		.slr_degen

;	ys: recover slab ysize

	move.w		d4,d5
	sub.w		d2,d5

;	byc: amount to crop from full slab height (up to max of slab height)
	
	sub.w		_drawcontext+dctx_clipwindow_y2,d4
;	move.w		d4,.sp_ycrop(sp)

;	byr: amount rmaining of full slab height (up to max of slab height)
	
	sub.w		d4,d5
	move.w		d5,.sp_yrem(sp)
	
;	index clipper to obtain (spanblocks-1)-1 for main, unclipped portion

	lsl.w		#3,d4
	move.w		slc_byc_remain(a6,d4.w),d7

;	address initial line

;	move.w		d2,.sp_yoff(sp)
	mulu.w		_g_linebytes,d2
	add.l		d2,a1	

;	usp tends to be used by engine for context/object/sprite-restore,
;	but here its more valuable for an inner loop so we need to save it first

	move.l		usp,a5				; usp (auto sprite restore) is preserved via a5

	.if		(0=_use_slab_restore_)
	.if		(preserve_usp)			; auto sprite restore is off, so
	move.l		a5,.sp_save_usp(sp)		; usp needs preserved explicitly
	.endif
	.endif
	
;	lea		slab_precalcs,a4
;	move.l		a4,usp
	lea		slab_precalcs,a3

	move.w		_g_linebytes,a4

	moveq		#0,d3
	moveq		#0,d4

	subq.w		#1,d7
	bmi		.slr_bc_no_main
	
;	(spanblocks-nc) in d7
;	(spans-1) in d6

	doregions

	move.l		a6,a0				; spanblock index
	add.w		(a2)+,a0			; next spanblock

	move.w		ssb_ys(a0),d6
	
.slr_bc_no_main:

;	always render one final (cropped) block - 0 or more full blocks before this

;	actual spans remaining = yrem-(y+dy)

	move.w		.sp_yrem(sp),d6
	sub.w		ssb_y(a0),d6
	sub.w		ssb_dy(a0),d6
	ble		.slr_done

	subq.w		#1,d6		
	moveq		#0,d7
	
;	(spanblocks-1) in d7
;	(spans-1) in d6
	
	doregions
	
	bra		.slr_done
		
*-------------------------------------------------------*
.slr_clip_safebot:
*-------------------------------------------------------*

;	address initial line

;	move.w		d2,.sp_yoff(sp)
	mulu.w		_g_linebytes,d2
	add.l		d2,a1

;	setup

;	usp tends to be used by engine for context/object/sprite-restore,
;	but here its more valuable for an inner loop so we need to save it first

	move.l		usp,a5				; usp (auto sprite restore) is preserved via a5

	.if		(0=_use_slab_restore_)
	.if		(preserve_usp)			; auto sprite restore is off, so
	move.l		a5,.sp_save_usp(sp)		; usp needs preserved explicitly
	.endif
	.endif
	
	lea		slab_precalcs,a3
;	lea		slab_precalcs,a4
;	move.l		a4,usp
	move.w		_g_linebytes,a4

	moveq		#0,d3
	moveq		#0,d4

;	(spanblocks-1) in d7
;	(spans-1) in d6

	doregions

*-------------------------------------------------------*
.slr_done:
*-------------------------------------------------------*

	.if		(_use_slab_restore_)
	move.l		a5,usp
	.else
	.if		(preserve_usp)			; auto sprite restore is off, so
	move.l		.sp_save_usp(sp),a5		; usp needs preserved explicitly
	move.l		a5,usp
	.endif	
	.endif

*-------------------------------------------------------*
.slr_degen:
*-------------------------------------------------------*

*=======================================================*
*	exitpoint
*=======================================================*
