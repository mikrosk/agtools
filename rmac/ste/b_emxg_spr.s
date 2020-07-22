*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	EMXSPR blitter codegen sprite implementation
*-------------------------------------------------------*
*	max source xsize: 32
*	max source ysize: viewport clipper ysize
*-------------------------------------------------------*
;	addq		#1,d0
	
enable_x_clipping	set	0
	
	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
;	sanity check on frame index!
	cmp.w		sps_framecount(a0),d0
	.if		(within_entitydraw)
	bhs		_AGT_Exception_EMXFrame_68k
	.else
	blo.s		.emx_frameok
.emx_framefault:
	not.w		$ffff8240.w
	bra.s		.emx_framefault
.emx_frameok:
	.endif
	.endif


	; todo: opt

	move.b		#3,BLiTLOP.w
	tst.w		d7
	beq.s		.emx_nohitflash
	bpl.s		.emx_hitflash
.emx_blinky:
	lsr.w		#1,d7
	bcs		.BLiT_EMXSpr_4BPL_end	
	bra.s		.emx_nohitflash
.emx_hitflash:
	move.b		#15,BLiTLOP.w	
.emx_nohitflash:
	
	

;	move.w		#2,BLiTSYI.w
	

;	move.w		sps_flags(a0),.sp_flags(sp)

	pushraster	#$062

*-------------------------------------------------------*
	.if		(enable_clipping)
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
	
	; sprite clipping window
	lea		_g_spr_window,a2
	
	; todo: opt using carry window

	cmp.w		(a2)+,d1
	.if		(enable_x_clipping)
	bmi.s		.BLiT_EMXSpr_GuardBand_4BPL
	.else
	bmi		.BLiT_EMXSpr_4BPL_end
	.endif
	
	cmp.w		(a2)+,d2
	bmi.s		.BLiT_EMXSpr_GuardBand_4BPL

	move.w		d1,d0
	add.w		d3,d0				; xr	
	sub.w		(a2)+,d0			; xr - cx2
	.if		(enable_x_clipping)
	bgt.s		.BLiT_EMXSpr_GuardBand_4BPL
	.else
	bgt		.BLiT_EMXSpr_4BPL_end
	.endif

	move.w		d2,d0
	add.w		d4,d0				; yr	
	sub.w		(a2)+,d0			; yr - cy2
	ble		.BLiT_EMXSpr_NoClip_4BPL_go
	

*-------------------------------------------------------*
*	sprite lies in guardband region or beyond
*-------------------------------------------------------*
.BLiT_EMXSpr_GuardBand_4BPL:
*-------------------------------------------------------*

	
;	exit if we have no embedded EMS colour data

	move.w		.sp_field4(sp),d6
	move.l		spf_emx_colour-8(a0,d6.w),d0
	beq		.BLiT_EMXSpr_4BPL_end
		
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

	lea		spf_psm_data-2(a6),a6

;	access colour data
	move.l		d0,a0

;	todo: relevant size & blitter fields should be stored with preshift
	move.w		(a0)+,d5
		
	; constants
	moveq		#16-1,d6
	moveq		#-16,d7
	
*-------------------------------------------------------*
*	initial source skip (unclipped case)
*-------------------------------------------------------*
	lea		2.w,a3	


	.if		(enable_clipping)
	
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
		
	.if		(enable_restore)
	.if		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		usp,a5
	move.l		a5,d5
	.endif
	.endif
	
	; dst pixelword xoffset
	moveq		#-16,d0
	and.w		d1,d0
	lsr.w		#1,d0	; real x clip doesn't allow -ve x
	
	.if		(enable_restore)
	.if		(use_sprite_pagerestore)	
	move.w		d2,(a5)+
	move.w		d0,(a5)+
	.endif	
	.endif

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

	.if		(enable_restore)
	.if		(use_sprite_pagerestore)

	add.w		d6,d6

	; clearing record
	
	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d4,(a5)+			; lines

	.endif
	
	.if		(use_sprite_saverestore)
	
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

	.endif
	.endif

*-------------------------------------------------------*

	.if		(enable_restore)
	.if		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		d5,(a5)+
	move.l		a5,usp
	.endif
	.endif


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


	bra		.BLiT_EMXSpr_4BPL_end


*-------------------------------------------------------*
.emx_clip_safe_right:
*-------------------------------------------------------*

	.endif		; enable_clipping

	
*-------------------------------------------------------*
*	common impl. for general paths
*-------------------------------------------------------*
.BLiT_EMXSpr_Clip_4BPL_yclipped_go:
*-------------------------------------------------------*

	; should never get here for EMX type - unclipped case takes codegen fastpath
;	bra		*

;	bra		.BLiT_EMXSpr_4BPL_end


;	.if		(0)
	
	
	

	.if		(enable_restore)
	.if		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		usp,a5
	move.l		a5,d5
	.endif
	.endif
	
	; dst pixelword xoffset
	moveq		#-16,d0
	and.w		d1,d0
;	lsr.w		#1,d0
	asr.w		#1,d0		; guardband allows -ve x!
	
	.if		(enable_restore)
	.if		(use_sprite_pagerestore)	
	move.w		d2,(a5)+
	move.w		d0,(a5)+
	.endif	
	.endif

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



	.if		(enable_restore)
	.if		(use_sprite_pagerestore)

	add.w		d6,d6

	; clearing record
		
	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d4,(a5)+			; lines

	.endif
	
	.if		(use_sprite_saverestore)
	
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

	.endif
	.endif

*-------------------------------------------------------*

	.if		(enable_restore)
	.if		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		d5,(a5)+
	move.l		a5,usp
	.endif
	.endif


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




;	.endif		;0



	.else		;(enable_clipping)



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
	


	.endif		;(enable_clipping)


	
*-------------------------------------------------------*
.BLiT_EMXSpr_NoClip_4BPL_go:
*-------------------------------------------------------*

	setraster	#$262

	; skew
	moveq		#16-1,d6
	and.w		d1,d6
	
;	moveq		#12,d6

;	access preshifted maskopt frame
	add.w		d6,d6
	add.w		d6,d6
;	move.l		spf_data+(4*2)+(4*16)-8(a0,d6.w),a6
	move.l		spf_emx_psmaskopt-8(a0,d6.w),a6

	move.w		(a6),d6				; spf_psmo_wordwidth

	lea		spf_psmo_dstyskip(a6),a4	; baked fields for pagerestore
	
	lea		spf_psmo_codegen(a6),a6		; baked sprite code
	
;	access colouropt data
;	move.l		spf_data+(4*1)-8(a0),a0
	move.w		.sp_field4(sp),d0
	move.l		spf_emx_colouropt-8(a0,d0.w),BLiTSRC.w


;	move.l		a0,BLiTSRC.w
;	move.w		#2,BLiTSYI.w

	; dst pixelword xoffset
	moveq		#-16,d0
	and.w		d1,d0
;	lsr.w		#1,d0
	asr.w		#1,d0
	add.w		d0,a1

	.if		(enable_restore&use_sprite_pagerestore)

	move.l		usp,a5
	move.l		a5,d5
	
	move.w		d2,(a5)+			; liney
	move.w		d0,(a5)+			; xoffbytes

	.endif

	; address yline	
	move.l		.sp_linetable(sp),a3
	add.w		d2,d2
	add.w		d2,d2
	add.l		(a3,d2.w),a1
	
	.if		(enable_restore&use_sprite_pagerestore)

	move.l		a1,(a5)+			; clear addr


	;move.w		.sp_maskwidth(sp),d6
;	add.w		d6,d6
;	move.w		d6,d0
;	lsl.w		#2,d0
;	neg.w		d0
;	add.w		_g_linebytes,d0
;	move.w		d0,a4

;	move.w #$00a0,(a5)+	; dstskip
;	move.w #$0006,(a5)+	; dstww2
;	move.w		a4,(a5)+
;	move.w		a4,(a5)+
;	move.w		d6,(a5)+	
	move.w		(a4)+,(a5)+	; dstskip
	move.w		(a4)+,d0
	lsr.w		d0		; hack: pagerestore still expects wordwidth*2, should really be *4
	move.w		d0,(a5)+	; dstww2
	move.w		d4,(a5)+
	move.l		d5,(a5)+	
	move.l		a5,usp	

	.endif
	
;	move.w #$00a0,(a5)+	; dstskip
;	move.w #$0006,(a5)+	; dstww2
;	move.w d4,(a5)+	; lines
;	move.l d5,(a5)+	; record link
;	move.l a5,usp	; save ctx
	
;	not.w		$ffff8240.w
	jsr		(a6)
;	not.w		$ffff8240.w

		
*-------------------------------------------------------*
.BLiT_EMXSpr_4BPL_end:
*-------------------------------------------------------*

	popraster
