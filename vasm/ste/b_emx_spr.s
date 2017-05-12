*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	EMXSPR blitter codegen sprite implementation
*-------------------------------------------------------*
*	max source xsize: 32
*	max source ysize: viewport clipper ysize
*-------------------------------------------------------*
;	addq		#1,d0

	ifd		AGT_CONFIG_SAFETY_CHECKS
;	sanity check on frame index!
	cmp.w		sps_framecount(a0),d0
	ifne		(within_entitydraw)
	bhs		_AGT_Exception_EMXFrame_68k
	else
	blo.s		.emx_frameok
.emx_framefault:
	not.w		$ffff8240.w
	bra.s		.emx_framefault
.emx_frameok:
	endc
	endc


	; todo: opt
	
	move.b		#3,BLiTLOP.w			; HOP
	tst.w		d7
	beq.s		.emx_nohitflash
	move.b		#15,BLiTLOP.w			; HOP	
.emx_nohitflash:


;	move.w		#2,BLiTSYI.w
	

;	move.w		sps_flags(a0),.sp_flags(sp)

	ifne		(use_rasters)
;	not.w		$ffff8240.w
	add.w		#23,$ffff8240.w
	endc
*-------------------------------------------------------*

	add.w		d0,d0				; todo: game layer can prescale this
	add.w		d0,d0
	move.l		sps_frameindex(a0,d0.w),a0	; access sprite frame data
			
	; frame fields

	move.w		(a0)+,d3			; w
	move.w		(a0)+,d4			; h
	add.w		(a0)+,d1			; +xo
	add.w		(a0)+,d2			; +yo
			
*-------------------------------------------------------*
*	guardband test for optimized fastpath
*-------------------------------------------------------*

	ifne		(enable_clipping)
	
	; sprite clipping window
	lea		_g_spr_window,a2
	
	; todo: opt using carry window

	cmp.w		(a2)+,d1
	bmi.s		.BLiT_EMXSpr_GuardBand_4BPL

	cmp.w		(a2)+,d2
	bmi.s		.BLiT_EMXSpr_GuardBand_4BPL

	move.w		d1,d0
	add.w		d3,d0				; xr	
	sub.w		(a2)+,d0			; xr - cx2
	bgt.s		.BLiT_EMXSpr_GuardBand_4BPL

	move.w		d2,d0
	add.w		d4,d0				; yr	
	sub.w		(a2)+,d0			; yr - cy2
	ble		.BLiT_EMXSpr_NoClip_4BPL_go
	
	endc


*-------------------------------------------------------*
*	sprite lies in guardband region or beyond
*-------------------------------------------------------*
.BLiT_EMXSpr_GuardBand_4BPL:
*-------------------------------------------------------*
	
	; skew
	moveq		#16-1,d6
	and.w		d1,d6
	
	
	; todo: store skew+nfsr data in preshift
	move.b		d6,.sp_skew(sp)

;	access preshifted mask frame
	add.w		d6,d6
	add.w		d6,d6
	move.l		spf_emx_psmask-8(a0,d6.w),a6
;	wordwidth of source mask
	move.w		(a6)+,d5
	move.w		d5,.sp_maskwidth(sp)


;	access colour data
	move.l		spf_emx_colour-8(a0),a0

;	todo: relevant size & blitter fields should be stored with preshift
	move.w		(a0)+,d5
		
	; constants
	moveq		#16-1,d6
	moveq		#-16,d7
	
*-------------------------------------------------------*
*	initial source skip (unclipped case)
*-------------------------------------------------------*
	lea		2.w,a3	


	ifne		(enable_clipping)
	
	lea		_g_spr_window,a2
	
;	cmp.w		clip_y1(a2),d2
;	bmi.s		.emx_clip_unsafe_top
;	move.w		d2,d0
;	add.w		d4,d0				; xr	
;	sub.w		clip_y2(a2),d0			; xr - cx2
;	bgt.s		.emx_clip_unsafe_bot
;
;	cmp.w		clip_x1(a2),d1
;	bmi.s		.emx_clip_unsafe_left
;	move.w		d1,d0
;	add.w		d3,d0				; xr	
;	sub.w		clip_x2(a2),d0			; xr - cx2
;	ble		.BLiT_EMXSpr_Clip_4BPL_yclipped_go
;	bra		.emx_clip_unsafe_right

;.clipper:
	

*-------------------------------------------------------*
*	top edge
*-------------------------------------------------------*
	cmp.w		clip_y1(a2),d2
	bpl.s		.emx_clip_safe_top
*-------------------------------------------------------*
*	top clipped
*-------------------------------------------------------*
.emx_clip_unsafe_top:
*-------------------------------------------------------*
	move.w		clip_y1(a2),d5
	; relative to clipwindow
	sub.w		d5,d2
	; crop lines
	add.w		d2,d4
	ble		.BLiT_EMXSpr_4BPL_end

	; adjust src offset in pixelwords*planes*yerr
	neg.w		d2
	move.w		d3,d0
	add.w		d6,d0
	and.w		d7,d0
	lsr.w		#1,d0				; *8
	mulu.w		d2,d0
	add.w		d0,a0				; +8*w*yerr
	
	; adjust mask offset in pixelwords*yerr
	move.w		.sp_maskwidth(sp),d5
	add.w		d5,d5
	mulu.w		d2,d5
	add.w		d5,a6
		
	; relative to world
	move.w		clip_y1(a2),d2
	; ASSUMPTION: sprite is not big enough to cross top+bottom
	bra.s		.emx_clip_safe_bot

*-------------------------------------------------------*
.emx_clip_safe_top:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	bottom edge
*-------------------------------------------------------*
	move.w		d2,d0
	add.w		d4,d0				; xr	
	sub.w		clip_y2(a2),d0			; xr - cx2
	ble.s		.emx_clip_safe_bot
*-------------------------------------------------------*
*	bottom clipped	
*-------------------------------------------------------*
.emx_clip_unsafe_bot:
*-------------------------------------------------------*
	sub.w		d0,d4
	ble		.BLiT_EMXSpr_4BPL_end
*-------------------------------------------------------*
.emx_clip_safe_bot:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	left edge
*-------------------------------------------------------*
	; NISR: no initial source read (i.e. shiftregisters preloaded with 0)
	cmp.w		clip_x1(a2),d1
	bpl.s		.emx_clip_safe_left
*-------------------------------------------------------*
*	left clipped
*-------------------------------------------------------*
.emx_clip_unsafe_left:
*-------------------------------------------------------*

	; FISR: force initial source read (i.e. shiftregisters preloaded with 1st data word)
	or.b		#$80,.sp_skew(sp)
	
	; relative to clipwindow
	sub.w		clip_x1(a2),d1

	; adjust dest width (ignore FISR part)
	
	move.w		d3,d5
	
	add.w		d1,d3
	ble		.BLiT_EMXSpr_4BPL_end

	; adjust src offset by clipped pixelwords (include FISR part)

	move.w		d6,d0
	sub.w		d1,d0
	and.w		d7,d0
	lsr.w		#4-1,d0				; w*2
	lea		-2(a0,d0.w),a0			; offset colour
	add.w		d0,a6				; offset mask
		
	; adjust colour src skip by ceil'd change in width
	; don't bother with same for mask, since EMs are hardcoded
	
	move.w		d3,d0
	add.w		d6,d0
	and.w		d7,d0				; ceil new width
	add.w		d6,d5
	and.w		d7,d5				; ceil old width

	sub.w		d0,d5				; delta
	lsr.w		#4-1,d5
	lea		-2(a3,d5.w),a3			; adjust (+ FISR)

	; relative to world
	move.w		clip_x1(a2),d1	
	; ASSUMPTION: sprite is not big enough to cross left+right
	bra		.BLiT_EMXSpr_Clip_4BPL_xclipped_go
	

*-------------------------------------------------------*
.emx_clip_safe_left:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	right edge
*-------------------------------------------------------*
	move.w		d1,d0
	add.w		d3,d0				; xr	
	sub.w		clip_x2(a2),d0			; xr - cx2
	ble		.emx_clip_safe_right
*-------------------------------------------------------*
*	right clipped
*-------------------------------------------------------*
.emx_clip_unsafe_right:
*-------------------------------------------------------*
	move.w		d3,d5

	; adjust pixel width
	sub.w		d0,d3
	ble		.BLiT_EMXSpr_4BPL_end

	; adjust by ceil'd change in width
	move.w		d3,d0
	add.w		d6,d0
	and.w		d7,d0				; ceil new width
	add.w		d6,d5
	and.w		d7,d5				; ceil old width

	
	; adjust colour src skip by ceil'd change in width
	; don't bother with same for mask, since EMs are hardcoded
	
	sub.w		d0,d5				; delta
	lsr.w		#4-1,d5
	add.w		d5,a3				; adjust (no FISR)
	
;	bra		.BLiT_EMXSpr_Clip_4BPL_xclipped_go


*-------------------------------------------------------*
*	common impl. for general paths
*-------------------------------------------------------*
.BLiT_EMXSpr_Clip_4BPL_xclipped_go:
*-------------------------------------------------------*
;			.abs
*-------------------------------------------------------*
;.sp_skew:		ds.w	1
;.sp_maskwidth:		ds.w	1
;.sp_colrwidth:		ds.w	1
;.sp_maskptr:		ds.l	1
;.sp_frame_:
*-------------------------------------------------------*
;			.text
*-------------------------------------------------------*
;	bra		.BLiT_EMXSpr_4BPL_end

;	bra		AGT_BLiT_EMSprDrawUnClipped_end
		
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		usp,a5
	move.l		a5,d5
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
;	mulu.w		_g_linebytes,d2
;	add.l		d2,a1

	; address yline	
	; todo: worth the overhead? table pointer needs refetched...
	move.l		.sp_linetable(sp),a4
	add.w		d2,d2
	add.w		d2,d2
	add.l		(a4,d2.w),a1
	
	
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




	; NFSR test
	
	and.w		d7,d0
	sub.w		d6,d0
	beq.s		.emx_EFSR2

	; NFSR
	or.b		#$40,.sp_skew(sp)

.emx_EFSR2:


	; dst skip adjust by dst width	
	move.w		_g_linebytes,a4
	lsr.w		#1,d6				; *8
	sub.w		d6,a4

	moveq		#8+2,d0
	sub.w		d6,d0
;	move.w		d6,d0
;	neg.w		d0
;	add.w		#8+2,d0	
	move.w		d0,BLiTDYI.w

	move.w		a3,BLiTSYI.w			; src yinc

	move.b		.sp_skew(sp),BLiTSKEW.w

	; word-width				
	lsr.w		#(4-1),d6			; >>4 *2
	move.w		d6,BLiTXC.w			; BLIT: width

	ifne		(enable_restore)
	ifne		(use_sprite_pagerestore)

	add.w		d6,d6

	; clearing record
	
	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d4,(a5)+			; lines

	endc
	
	ifne		(use_sprite_saverestore)
	
	add.w		d6,d6
	
	; clearing record

	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d4,(a5)+			; lines

	move.l		a1,a3
	moveq		#-1,d1
	add.w		d4,d1

	add.w		d6,d6
	neg.w		d6
;	todo: optimize this
.emx_r_sylp1:
	jmp		.emx_r_jt1(pc,d6.w)
	rept		9
	move.l		(a3)+,(a5)+
	move.l		(a3)+,(a5)+
	endr
.emx_r_jt1:
	add.w		a4,a3
	dbra		d1,.emx_r_sylp1

	endc
	endc

*-------------------------------------------------------*

	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		d5,(a5)+
	move.l		a5,usp
	endc
	endc


*-------------------------------------------------------*
.BLiT_EMXSpr_Clip_4BPL_xclipped_blit:
*-------------------------------------------------------*
;			.abs
*-------------------------------------------------------*
;.sp_skew:		ds.w	1
;.sp_maskwidth:		ds.w	1
;.sp_colrwidth:		ds.w	1
;.sp_maskptr:		ds.l	1
;.sp_frame_:
*-------------------------------------------------------*
;			.text
*-------------------------------------------------------*
	ifne		(use_rasters)
	move.w		$ffff8240.w,$ffff8242.w
	move.w		#-1,$ffff8240.w
	endc
	
	move.l		a0,BLiTSRC.w


*-------------------------------------------------------*

	lea		BLiTYC.w,a0	
	lea		BLiTCTRL.w,a5
	lea		BLiTEM3.w,a4
	lea		BLiTDST.w,a3
	lea		BLiTEM1.w,a2
	moveq		#$c0,d1
	moveq		#4,d2
	move.w		_g_linebytes,d3

*-------------------------------------------------------*

	moveq		#-2,d0
	add.w		.sp_maskwidth(sp),d0
	beq		.emx_xc_dbl_word
	

*-------------------------------------------------------*
.emx_xc_tri_word_b:
*-------------------------------------------------------*
	moveq		#8-1,d0
	and.w		d4,d0
	
	add.w		d0,d0				; *2
	move.w		d0,d5
	lsl.w		#3,d5				; *16
	sub.w		d5,d0				; *-14
	lsr.w		#3,d4

	jmp		.emx_xc_t_j0(pc,d0.w)	

;	add.w		d0,d0
;	lsr.w		#3,d4
;	jmp		.emx_xc_t_jt(pc,d0.w)
;.emx_xc_t_jt:
;	bra.s		.emx_xc_t_j0
;	bra.s		.emx_xc_t_j1
;	bra.s		.emx_xc_t_j2
;	bra.s		.emx_xc_t_j3
;	bra.s		.emx_xc_t_j4
;	bra.s		.emx_xc_t_j5
;	bra.s		.emx_xc_t_j6
;	bra.s		.emx_xc_t_j7

.emx_xc_t_j8:	scan_32wc		
.emx_xc_t_j7:	scan_32wc		
.emx_xc_t_j6:	scan_32wc		
.emx_xc_t_j5:	scan_32wc		
.emx_xc_t_j4:	scan_32wc		
.emx_xc_t_j3:	scan_32wc		
.emx_xc_t_j2:	scan_32wc		
.emx_xc_t_j1:	scan_32wc		
.emx_xc_t_j0:	dbra		d4,.emx_xc_t_j8

	; reset constant SYI for sake of unclipped faspath
	move.w		#2,BLiTSYI.w
	
	ifne		(use_rasters)
	move.w		$ffff8242.w,$ffff8240.w
	endc

	bra		.BLiT_EMXSpr_4BPL_end
	
*-------------------------------------------------------*
.emx_xc_dbl_word:
*-------------------------------------------------------*
	moveq		#8-1,d0
	and.w		d4,d0
	add.w		d0,d0
	lsr.w		#3,d4
	jmp		.emx_xc_d_jt(pc,d0.w)
.emx_xc_d_jt:
	bra.s		.emx_xc_d_j0
	bra.s		.emx_xc_d_j1
	bra.s		.emx_xc_d_j2
	bra.s		.emx_xc_d_j3
	bra.s		.emx_xc_d_j4
	bra.s		.emx_xc_d_j5
	bra.s		.emx_xc_d_j6
	bra.s		.emx_xc_d_j7

.emx_xc_d_j8:	scan_2wc		
.emx_xc_d_j7:	scan_2wc		
.emx_xc_d_j6:	scan_2wc		
.emx_xc_d_j5:	scan_2wc		
.emx_xc_d_j4:	scan_2wc		
.emx_xc_d_j3:	scan_2wc		
.emx_xc_d_j2:	scan_2wc		
.emx_xc_d_j1:	scan_2wc		
.emx_xc_d_j0:	dbra		d4,.emx_xc_d_j8

	; reset constant SYI for sake of unclipped faspath
	move.w		#2,BLiTSYI.w

	ifne		(use_rasters)
	move.w		$ffff8242.w,$ffff8240.w
	endc

	bra		.BLiT_EMXSpr_4BPL_end


*-------------------------------------------------------*
.emx_clip_safe_right:
*-------------------------------------------------------*

	endc		; enable_clipping

	
*-------------------------------------------------------*
*	common impl. for general paths
*-------------------------------------------------------*
.BLiT_EMXSpr_Clip_4BPL_yclipped_go:
*-------------------------------------------------------*

	; should never get here for EMX type - unclipped case takes codegen fastpath
;	bra		*

;	bra		.BLiT_EMXSpr_4BPL_end


;	ifne		(0)
	
	
	

	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		usp,a5
	move.l		a5,d5
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
	; todo: just use playfield linetable here
;	mulu.w		#linebytes,d2
	mulu.w		_g_linebytes,d2
	add.l		d2,a1
	
	
	; find destination word width of shifted sprite

	move.w		d6,d7

	and.w		d1,d6				; rshift

	add.w		d3,d7				; w+round

	moveq		#-16,d0	
	add.w		d7,d6				; rshift+w+round
	and.w		d0,d6				; wceil(rshift+w)


	; NFSR test
	; note: this and some other values could be looked up from ((x&15):srcw)
	
	and.w		d7,d0
	sub.w		d6,d0
	beq.s		.emx_EFSR1
	or.b		#$40,.sp_skew(sp)
.emx_EFSR1:

	; dst skip adjust by dst width	
	move.w		_g_linebytes,a4
	lsr.w		#1,d6
	sub.w		d6,a4

	moveq		#8+2,d0
	sub.w		d6,d0	
	move.w		d0,BLiTDYI.w

;	move.w		a3,BLiTSYI.w

	move.b		.sp_skew(sp),BLiTSKEW.w


	; word-width				
	lsr.w		#(4-1),d6			; >>4 *2
	move.w		d6,BLiTXC.w			; BLIT: width



	ifne		(enable_restore)
	ifne		(use_sprite_pagerestore)

	add.w		d6,d6

	; clearing record
		
	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d4,(a5)+			; lines

	endc
	
	ifne		(use_sprite_saverestore)
	
	add.w		d6,d6
	
	; clearing record

	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d4,(a5)+			; lines

	move.l		a1,a3
	moveq		#-1,d1
	add.w		d4,d1

	add.w		d6,d6
	neg.w		d6
;	todo: optimize this
.emx_r_sylp2:
	jmp		.emx_r_jt2(pc,d6.w)
	rept		9
	move.l		(a3)+,(a5)+
	move.l		(a3)+,(a5)+
	endr 
.emx_r_jt2:
	add.w		a4,a3
	dbra		d1,.emx_r_sylp2

	endc
	endc

*-------------------------------------------------------*

	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		d5,(a5)+
	move.l		a5,usp
	endc
	endc


*-------------------------------------------------------*
.BLiT_EMXSpr_Clip_4BPL_unclipped_blit:
*-------------------------------------------------------*
	
	move.l		a0,BLiTSRC.w

*-------------------------------------------------------*

	lea		BLiTYC.w,a0	
	lea		BLiTCTRL.w,a5
	lea		BLiTEM3.w,a4
	lea		BLiTDST.w,a3
	lea		BLiTEM1.w,a2
	moveq		#$c0,d1
	moveq		#4,d2
	move.w		_g_linebytes,d3

*-------------------------------------------------------*

	moveq		#-1,d0
	add.w		.sp_maskwidth(sp),d0
	beq		.emx_uc_sgl_word
	subq.w		#1,d0
	beq		.emx_uc_dbl_word

*-------------------------------------------------------*
.emx_uc_tri_word:
*-------------------------------------------------------*
	moveq		#8-1,d0
	and.w		d4,d0
	add.w		d0,d0
	lsr.w		#3,d4
	jmp		.emx_uc_t_jt(pc,d0.w)
.emx_uc_t_jt:
	bra.s		.emx_uc_t_js
	bra.s		.emx_uc_t_j1
	bra.s		.emx_uc_t_j2
	bra.s		.emx_uc_t_j3
	bra.s		.emx_uc_t_j4
	bra.s		.emx_uc_t_j5
	bra.s		.emx_uc_t_j6
	bra.s		.emx_uc_t_j7


	; 12bytes
.emx_uc_t_js:	subq.w		#1,d4		
.emx_uc_t_j8:	scan_3w		
.emx_uc_t_j7:	scan_3w		
.emx_uc_t_j6:	scan_3w		
.emx_uc_t_j5:	scan_3w		
.emx_uc_t_j4:	scan_3w		
.emx_uc_t_j3:	scan_3w		
.emx_uc_t_j2:	scan_3w		
.emx_uc_t_j1:	scan_3w		
.emx_uc_t_j0:	dbra		d4,.emx_uc_t_j8

	bra		.BLiT_EMXSpr_4BPL_end
	
*-------------------------------------------------------*
.emx_uc_dbl_word:
*-------------------------------------------------------*
	moveq		#8-1,d0
	and.w		d4,d0
	add.w		d0,d0
	lsr.w		#3,d4
	jmp		.emx_uc_d_jt(pc,d0.w)
.emx_uc_d_jt:
	bra.s		.emx_uc_d_js
	bra.s		.emx_uc_d_j1
	bra.s		.emx_uc_d_j2
	bra.s		.emx_uc_d_j3
	bra.s		.emx_uc_d_j4
	bra.s		.emx_uc_d_j5
	bra.s		.emx_uc_d_j6
	bra.s		.emx_uc_d_j7

	; 12bytes
.emx_uc_d_js:	subq.w		#1,d4	
.emx_uc_d_j8:	scan_2w		
.emx_uc_d_j7:	scan_2w		
.emx_uc_d_j6:	scan_2w		
.emx_uc_d_j5:	scan_2w		
.emx_uc_d_j4:	scan_2w		
.emx_uc_d_j3:	scan_2w		
.emx_uc_d_j2:	scan_2w		
.emx_uc_d_j1:	scan_2w		
.emx_uc_d_j0:	dbra		d4,.emx_uc_d_j8

	bra		.BLiT_EMXSpr_4BPL_end
	
*-------------------------------------------------------*
.emx_uc_sgl_word:
*-------------------------------------------------------*
	moveq		#8-1,d0
	and.w		d4,d0
	add.w		d0,d0
	lsr.w		#3,d4
	jmp		.emx_uc_s_jt(pc,d0.w)
.emx_uc_s_jt:	
	bra.s		.emx_uc_s_js
	bra.s		.emx_uc_s_j1
	bra.s		.emx_uc_s_j2
	bra.s		.emx_uc_s_j3
	bra.s		.emx_uc_s_j4
	bra.s		.emx_uc_s_j5
	bra.s		.emx_uc_s_j6
	bra.s		.emx_uc_s_j7

	; 10bytes
.emx_uc_s_js:	subq.w		#1,d4
.emx_uc_s_j8:	scan_1w		
.emx_uc_s_j7:	scan_1w		
.emx_uc_s_j6:	scan_1w		
.emx_uc_s_j5:	scan_1w		
.emx_uc_s_j4:	scan_1w		
.emx_uc_s_j3:	scan_1w		
.emx_uc_s_j2:	scan_1w		
.emx_uc_s_j1:	scan_1w		
.emx_uc_s_j0:	dbra		d4,.emx_uc_s_j8
	
	bra		.BLiT_EMXSpr_4BPL_end




;	endc		;0





*-------------------------------------------------------*
*	Entrypoint for EMXSPR with cull-clipping only	
*-------------------------------------------------------*
.BLiT_EMXSpr_NoClip_4BPL:
*-------------------------------------------------------*

	add.w		d0,d0				; todo: game layer can prescale this
	add.w		d0,d0
	move.l		sps_frameindex(a0,d0.w),a0	; access sprite frame data
			
	; frame fields

	move.w		(a0)+,d3			; w
	move.w		(a0)+,d4			; h
	add.w		(a0)+,d1			; +xo
	add.w		(a0)+,d2			; +yo
			
*-------------------------------------------------------*
*	initial source skip (unclipped case)
*-------------------------------------------------------*

	ifne		(enable_clipping)
	
	; sprite clipping window
	lea		_g_spr_window,a2

	cmp.w		(a2)+,d1
	bmi.s		.BLiT_EMXSpr_4BPL_end

	cmp.w		(a2)+,d2
	bmi.s		.BLiT_EMXSpr_4BPL_end

	move.w		d1,d0
	add.w		d3,d0				; xr	
	sub.w		(a2)+,d0			; xr - cx2
	bgt.s		.BLiT_EMXSpr_4BPL_end

	move.w		d2,d0
	add.w		d4,d0				; yr	
	sub.w		(a2)+,d0			; yr - cy2
	ble.s		.BLiT_EMXSpr_NoClip_4BPL_go

	bra.s		.BLiT_EMXSpr_4BPL_end
	
	endc
	
*-------------------------------------------------------*
.BLiT_EMXSpr_NoClip_4BPL_go:
*-------------------------------------------------------*

	ifne		(use_rasters)
	move.w		$ffff8240.w,$ffff8242.w
	move.w		#-1,$ffff8240.w
	endc

	; skew
	moveq		#16-1,d6
	and.w		d1,d6
	
;	moveq		#12,d6

;	access preshifted maskopt frame
	add.w		d6,d6
	add.w		d6,d6
;	move.l		spf_data+(4*2)+(4*16)-8(a0,d6.w),a6
	move.l		spf_emx_psmaskopt-8(a0,d6.w),a6

;	access colouropt data
;	move.l		spf_data+(4*1)-8(a0),a0
	move.l		spf_emx_colouropt-8(a0),BLiTSRC.w


;	move.l		a0,BLiTSRC.w
;	move.w		#2,BLiTSYI.w

	; dst pixelword xoffset
	moveq		#-16,d0
	and.w		d1,d0
	lsr.w		#1,d0
	add.w		d0,a1

	ifne		(enable_restore&use_sprite_pagerestore)

	move.l		usp,a5
	move.l		a5,d5
	
	move.w		d2,(a5)+			; liney
	move.w		d0,(a5)+			; xoffbytes

	endc

	; address yline	
	move.l		.sp_linetable(sp),a3
	add.w		d2,d2
	add.w		d2,d2
	add.l		(a3,d2.w),a1
	
	ifne		(enable_restore&use_sprite_pagerestore)

	move.l		a1,(a5)+			; clear addr

	endc

	ifne		(use_rasters)
	move.w		$ffff8242.w,$ffff8240.w
	endc
		
	jsr		(a6)

		
*-------------------------------------------------------*
.BLiT_EMXSpr_4BPL_end:
*-------------------------------------------------------*


