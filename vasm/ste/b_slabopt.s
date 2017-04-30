*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	SLAB sprite implementation
*-------------------------------------------------------*
*	max source xsize: 256
*	max source ysize: viewport clipper ysize
*-------------------------------------------------------*

*=======================================================*
*	entrypoint
*=======================================================*

	ifd		AGT_CONFIG_SAFETY_CHECKS
;	sanity check on frame index!
	cmp.w		sps_framecount(a0),d0
	ifne		(within_entitydraw)
	bhs		_AGT_Exception_SLSFrame_68k
	else
	blo.s		.slb_frameok
.slb_framefault:
	not.w		$ffff8240.w
	bra.s		.slb_framefault
.slb_frameok:
	endc
	endc

	; 'hitflash' for strobing shape to single colour on damage etc.
	; todo: opt
	
	move.b		#3,BLiTLOP.w			; HOP
	tst.w		d7
	beq.s		.slb_no_hitflash
	move.b		#15,BLiTLOP.w			; HOP	
.slb_no_hitflash:
	

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

	; relative base for offset tables
	
	lea		slf_indexbase(a0),a6		; indexbase
	
	move.l		a6,a2

	cmp.w		_drawcontext+dctx_clipwindow_y1,d2
	bpl.s		.slb_clip_safetop

*-------------------------------------------------------*
.slb_clip_top:
*-------------------------------------------------------*
*	top clipping is a bit complex, but relative to 
*	drawing the object and limits on slab drawing
*	count generally, the overhead is neglegible	
*-------------------------------------------------------*

	sub.w		_drawcontext+dctx_clipwindow_y1,d2
	neg.w		d2
	cmp.w		d4,d2
	bge		.slb_degen			; completely cropped?
	
	move.w		d2,d5				; total amount to crop

	lsl.w		#3,d2
	lea		(a6,d2.w),a0

	add.w		(a0)+,a2			; slc_tyc_offset : first spanblock record = [clipper + yc]
	move.w		(a0)+,d7			; slc_tyc_remain : spanblocks remaining for this yc
	subq.w		#1,d7

	moveq		#0,d4
	move.w		(a0),d4				; slc_tyc_data
	lea		(a6,d4.l),a3			; +64k reach for span pixel data
	

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

	lsl.w		#3,d5
	mulu.w		ssb_dxw(a0),d5
	add.l		d5,a3

;	y origin = clipping edge

	move.w		_drawcontext+dctx_clipwindow_y1,d2
	
;	slab size is restricted - if clipped at top, 
;	can't also be clipped at bottom	

	bra		.slb_clip_safebot
						
*-------------------------------------------------------*
.slb_clip_safetop:
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
	
	move.l		ssb_data(a0),a3

;	test window bottom

	add.w		d2,d4
	cmp.w		_drawcontext+dctx_clipwindow_y2,d4
	ble		.slb_clip_safebot

*-------------------------------------------------------*
.slb_clip_bot:
*-------------------------------------------------------*
*	bottom clipping is achieved by drawing
*	(spanblocks-nc) normally and then one final
*	block with cropped height, if required
*-------------------------------------------------------*

;	test for fully clipped out

	cmp.w		_drawcontext+dctx_clipwindow_y2,d2
	bge		.slb_degen

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

;	load initial block source

	move.l		a3,BLiTSRC.w

;	address initial line

	move.w		d2,.sp_yoff(sp)
	mulu.w		_g_linebytes,d2
	add.l		d2,a1	

;	usp tends to be used by engine for context/object/sprite-restore,
;	but here its more valuable for an inner loop so we need to save it first

	move.l		usp,a5				; usp (auto sprite restore) is preserved via a5

	ifne		(preserve_usp)			; auto sprite restore is off, so
	move.l		a5,.sp_save_usp(sp)		; usp needs preserved explicitly
	endc
	lea		BLiTDST.w,a5
	
	lea		BLiTCTRL.w,a3
	lea		slab_precalcs,a4
	move.l		a4,usp
	move.w		_g_linebytes,a4

	moveq		#7,d0
	moveq		#$80,d2
	moveq		#0,d3
	moveq		#0,d4

	subq.w		#1,d7
	bmi		.slb_bc_no_main
	
;	(spanblocks-nc) in d7
;	(spans-1) in d6

	dospans

	move.l		a6,a0				; spanblock index
	add.w		(a2)+,a0			; next spanblock

	move.w		ssb_ys(a0),d6
	
.slb_bc_no_main:

;	always render one final (cropped) block - 0 or more full blocks before this

;	actual spans remaining = yrem-(y+dy)

	move.w		.sp_yrem(sp),d6
	sub.w		ssb_y(a0),d6
	sub.w		ssb_dy(a0),d6
	ble		.slb_done

	subq.w		#1,d6		
	moveq		#0,d7
	
;	(spanblocks-1) in d7
;	(spans-1) in d6
	
	dospans
	
	bra		.slb_done
		
*-------------------------------------------------------*
.slb_clip_safebot:
*-------------------------------------------------------*

;	load initial block source

	move.l		a3,BLiTSRC.w

;	address initial line

	move.w		d2,.sp_yoff(sp)
	mulu.w		_g_linebytes,d2
	add.l		d2,a1

;	setup

;	usp tends to be used by engine for context/object/sprite-restore,
;	but here its more valuable for an inner loop so we need to save it first

	move.l		usp,a5				; usp (auto sprite restore) is preserved via a5

	ifne		(preserve_usp)			; auto sprite restore is off, so
	move.l		a5,.sp_save_usp(sp)		; usp needs preserved explicitly
	endc
	lea		BLiTDST.w,a5
	
	lea		BLiTCTRL.w,a3
	lea		slab_precalcs,a4
	move.l		a4,usp
	move.w		_g_linebytes,a4

	moveq		#7,d0
	moveq		#$80,d2
	moveq		#0,d3
	moveq		#0,d4

;	(spanblocks-1) in d7
;	(spans-1) in d6

	dospans

*-------------------------------------------------------*
.slb_done:
*-------------------------------------------------------*

	ifne		(preserve_usp)			; auto sprite restore is off, so
	move.l		.sp_save_usp(sp),a5		; usp needs preserved explicitly
	move.l		a5,usp
	endc	

*-------------------------------------------------------*
.slb_degen:
*-------------------------------------------------------*

*=======================================================*
*	exitpoint
*=======================================================*
