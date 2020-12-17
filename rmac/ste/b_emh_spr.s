*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	EMHSPR compound blitter sprite implementation
*-------------------------------------------------------*
*	max source xsize: -
*	max source ysize: viewport clipper ysize
*-------------------------------------------------------*

	.if		^^defined AGT_CONFIG_SAFETY_CHECKS
;	sanity check on frame index!
	cmp.w		sps_framecount(a0),d0
	.if		(within_entitydraw)
	bhs		_AGT_Exception_EMSFrame_68k
	.else
	blo.s		.emh_frameok
.emh_framefault:
	not.w		$ffff8240.w
	bra.s		.emh_framefault
.emh_frameok:
	.endif
	.endif

	pushraster	#$062

	; todo: opt
	
	;bra		*

	move.b		#3,BLiTLOP.w
	tst.w		d7
	beq.s		.emh_nohitflash
	bpl.s		.emh_hitflash
.emh_blinky:
	lsr.w		#1,d7
	bcs		.emh_4BPL_end_all	
	bra.s		.emh_nohitflash
.emh_hitflash:
	move.b		#15,BLiTLOP.w	
.emh_nohitflash:


;	move.w		#2,BLiTSYI.w


;	move.w		sps_flags(a0),.sp_flags(sp)



*-------------------------------------------------------*

	moveq		#1,d3
	and.b		sps_flags(a0),d3		; 1 = double index
	beq.s		.emh_no_dfm
	add.w		d0,d0				;	
	and.w		.sp_mfield(sp),d3		; 1 = field offset
	add.w		d3,d0	
.emh_no_dfm:
	add.w		d0,d0				; todo: game layer can prescale this
	add.w		d0,d0
	move.l		sps_frameindex(a0,d0.w),a0	; access sprite frame data
	
	; frame fields

	move.w		(a0)+,d3			; w
	move.w		(a0)+,d4			; h
	add.w		(a0)+,d1			; +xo
	add.w		(a0)+,d2			; +yo

	move.l		a0,.emh_sprframe
	move.l		a1,.emh_framebuf
	movem.w		d1-d4,.emh_sprpos

*-------------------------------------------------------*
*	occlusion mapping	
*-------------------------------------------------------*
	move.w		(a0),d0
	.if		(enable_restore)
	.if		(use_sprite_saverestore|use_sprite_pagerestore)
	move.w		d0,.emh_update_prestore1+2
	.endif
	.endif
	beq.s		.emh_no_occlusion
*-------------------------------------------------------*
	move.l		a0,-(sp)
	lea		(a0,d0.w),a0		
	jsr		_OCM_occlusion_add_T			
	move.l		(sp)+,a0
*-------------------------------------------------------*
	.if		(enable_restore)
	.if		(use_sprite_saverestore|use_sprite_pagerestore)
	moveq		#-bgrframe_size,d0
	move.w		d0,.emh_update_prestore1+2
	.endif
	.endif
	
*-------------------------------------------------------*
.emh_no_occlusion:
*-------------------------------------------------------*

	; skew
	moveq		#16-1,d6
	and.w		d1,d6

;	access preshifted mask frame
	add.w		d6,d6
	add.w		d6,d6
	move.l		spf_em_psmask-8(a0,d6.w),a6
	
	
;spf_psm2_wordwidth:	ds.w	1
;spf_psm2_bskew:		ds.w	1
;spf_psm2_bsyi:		ds.w	1
;spf_psm2_bdyi:		ds.w	1
;spf_psm2_wordoffset:	ds.w	1
;spf_psm2_emdata:	ds.w	1		; offset to (unique) EM data
;spf_psm2_code:		ds.l	1		; offset to (shared) codegen
;spf_psm2_nextcmp:	ds.l	1		; next component
;spf_psm2_clip:					; clipping table follows

	
	st		.emh_is_first_cmp

.emh_firstcmp:
	move.l		a6,d6
;	wordwidth of source mask
	move.w		(a6)+,d5			; spf_psm2_wordwidth
	move.w		d5,.sp_maskwidth(sp)
;	move.w		d5,BLiTXC.w
	
	
	move.w		(a6)+,d0
;	move.b		d0,BLiTSKEW.w			; spf_psm2_bskew
	move.b		d0,.emh_skew_smc+3
	
	move.w		(a6)+,BLiTSYI.w			; spf_psm2_bsyi
	move.w		(a6)+,BLiTDYI.w			; spf_psm2_bdyi	
	move.w		(a6)+,BLiTXC.w			; spf_psm2_bxc
;	addq		#2,a6				; spf_psm2_wordoffset
;	move.w		(a6)+,d0

	move.w		(a6)+,d0			; spf_psm2_emdata
	move.l		d6,a3
	add.w		d0,a3				; psmask+spf_psm2_emdata	
	move.l		a3,.emh_psm2_emdata

;	bra		*

	move.l		(a6)+,d0			; spf_psm2_code
	move.l		d6,a3
	add.l		d0,a3				; psmask+spf_psm2_code	
	move.l		a3,.emh_psm2_mprog
	move.l		a3,.emh_psm2_mprog2
		

	; link to next component, if present
	move.l		(a6)+,d7
	beq.s		.emh_onlypsm
	add.l		d6,d7
.emh_onlypsm:
	move.l		d7,.emh_next_psmask			; spf_psm_nextcmp



;	access colour data
	move.w		.sp_cfield4(sp),d5
	move.l		spf_em_colour-8(a0,d5.w),a0

;	move.l		spf_em_colour-8(a0),a0

;	word-width for this preshift (will be 1, 2 or 3 for normal EMS, more for wide format)
;	todo: relevant size & blitter fields e.g. BLiTSYI could be stored with frame
	move.w		(a0)+,d5			; ***** unused *****


	move.l		a0,.emh_cmp_cplanes
;	clr.w		.emh_cmp_masklineoff
	
;.next_component:

	; constants
	moveq		#16-1,d6
	moveq		#-16,d7

*-------------------------------------------------------*
*	initial source skip (unclipped case)
*-------------------------------------------------------*
	lea		2.w,a3

	clr.w		.emh_cmp_dxadj
	clr.w		.emh_cmp_dyadj


	.if		(enable_clipping)

	lea		_g_spr_window,a2
	
;	move.w		#16+1,clip_y1(a2)
;	move.w		#64-1,clip_y2(a2)
	
	move.w		d4,.emh_cmp_height
	
;	moveq		#16-1,d0
;	and.w		d1,d0
;	move.b		d0,.skew_smc+3

*-------------------------------------------------------*
*	top edge
*-------------------------------------------------------*
	cmp.w		clip_y1(a2),d2
	bpl		.emh_safetop
*-------------------------------------------------------*
*	top clipped
*-------------------------------------------------------*
.emh_unsafetop:
*-------------------------------------------------------*
	move.w		clip_y1(a2),d5
	; relative to clipwindow
	sub.w		d5,d2
	; crop lines
	add.w		d2,d4
	ble		.emh_4BPL_end_all

	;bra		*
		
	neg.w		d2
	
;	move.w		d2,d0
;	mulu.w		#spct_size,d0
;	move.l		a6,a4
;	add.l		d0,a4				; spf_psm2_clip+(spct_size*tyc)

	move.w		d2,d0
	add.w		d0,d0
	move.w		(a6,d0.w),d0			; compressed offset to mprog inits
	move.l		a6,a4
	add.w		d0,a4
		
	
	;bra		*
;spct_emdata_off:	ds.w	1
;spct_srccol_off:	ds.w	1
;spct_mprog_off:		ds.w	1
;spct_lrem:		ds.w	1
;spct_dline_off:		ds.b	1
;spct_dword_off:		ds.b	1
;spct_b_syi:		ds.b	1
;spct_b_dyi:		ds.b	1
;spct_b_skew:		ds.b	1
;spct_b_xc:		ds.b	1
;spct_b_em1:		ds.w	1
;spct_b_em2:		ds.w	1
;spct_b_em3:		ds.w	1	

	move.w		spct_emdata_off(a4),d0
	add.w		d0,d0
	ext.l		d0
	add.l		d0,.emh_psm2_emdata

	move.w		spct_srccol_off(a4),d0
	ext.l		d0
	add.l		d0,a0
	
	move.w		spct_mprog_off(a4),d0
	add.w		d0,d0
	ext.l		d0
	add.l		d0,.emh_psm2_mprog
	
	move.b		spct_b_dyi(a4),d0
	ext.w		d0
	move.w		d0,BLiTDYI.w

	moveq		#0,d0
	move.b		spct_b_xc(a4),d0
	move.w		d0,BLiTXC.w
	move.b		spct_b_syi(a4),d0
	move.w		d0,BLiTSYI.w
	
	move.b		spct_b_skew(a4),d0
	move.b		d0,.emh_skew_smc+3	;BLiTSKEW.w

	move.w		spct_b_em1(a4),BLiTEM1.w	
	move.w		spct_b_em2(a4),BLiTEM2.w	
	move.w		spct_b_em3(a4),BLiTEM3.w	

	move.b		spct_dword_off(a4),d0
	lsl.w		#3,d0
	move.w		d0,.emh_cmp_dxadj

	move.b		spct_dline_off(a4),d0
	sub.w		d2,d0
	move.w		d0,.emh_cmp_dyadj
	
	
;	muls.w		_g_linebytes,d0
;	add.l		d0,a1
	
	
	
	; relative to world
	move.w		clip_y1(a2),d2
	; ASSUMPTION: sprite is not big enough to cross top+bottom
	;bra.s		.emh_safebot

*-------------------------------------------------------*
.emh_safetop:
*-------------------------------------------------------*
	move.l		#.emh_scratch,.emh_smc_pos

*-------------------------------------------------------*
*	bottom edge
*-------------------------------------------------------*
	move.w		d2,d0
	add.w		d4,d0				; xr
	sub.w		clip_y2(a2),d0			; xr - cx2
	ble.s		.emh_safebot
*-------------------------------------------------------*
*	bottom clipped
*-------------------------------------------------------*
.emh_unsafebot:
*-------------------------------------------------------*
	sub.w		d0,d4
	ble		.emh_4BPL_end_all
	
	; d0 adj
	
	neg.w		d0
	add.w		.emh_cmp_height,d0
	
	add.w		d0,d0
	move.w		(a6,d0.w),d0
	add.w		d0,a6	
	
;	mulu.w		#spct_size,d0
;	add.w		d0,a6				; spf_psm2_clip+(spct_size*tyc)
	
	move.l		.emh_psm2_mprog2,a4
	move.w		spct_mprog_off(a6),d0
	add.w		d0,d0
	add.w		d0,a4
	move.l		a4,.emh_smc_pos
	
	move.w		(a4),.emh_smc_save
	move.w		#$4e75,(a4)	
	
*-------------------------------------------------------*
.emh_safebot:
*-------------------------------------------------------*

	.endif		; enable_clipping


*-------------------------------------------------------*
*	common impl. for general paths
*-------------------------------------------------------*
.emh_Clip_4BPL_unclipped_got:
*-------------------------------------------------------*

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

	tst.b		.emh_is_first_cmp
	beq		.emh_no_emit_sr1

	.if		(enable_restore)
	.if		(use_sprite_pagerestore)
	move.w		d2,(a5)+
	move.w		d0,(a5)+
	.endif
	.endif

.emh_no_emit_sr1:


	add.w		d0,a1

	; dst yoffset (line)
	; todo: just use playfield linetable here
;	mulu.w		#linebytes,d2
	muls.w		_g_linebytes,d2
	add.l		d2,a1


	; find destination word width of whole sprite, all composites
	; to produce a single sprite restore rectangle
	
	move.w		d6,d7
	and.w		d1,d6				; rshift/skew
	add.w		d3,d7				; w+round
	moveq		#-16,d0
	add.w		d7,d6				; rshift+w+round
	and.w		d0,d6				; wceil(rshift+w)

	; dst skip adjust by dst width
	move.w		_g_linebytes,a4
	lsr.w		#1,d6
	sub.w		d6,a4
	lsr.w		#(4-1),d6		; >>4 *2


	tst.b		.emh_is_first_cmp
	beq		.emh_no_emit_sr2


	.if		(1)
	
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
.emh_sylp:	
	jmp		.emh_jt(pc,d6.w)
	rept		9
	move.l		(a3)+,(a5)+
	move.l		(a3)+,(a5)+
	endr
.emh_jt:
	add.w		a4,a3
	dbra		d1,.emh_sylp

	.endif
	.endif

*-------------------------------------------------------*

	.if		(enable_restore)
	.if		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		d5,(a5)+
.emh_update_prestore1:
	lea		$1234(a5),a5
	move.l		a5,usp	
	.endif
	.endif
	
	.endif		;0

.emh_no_emit_sr2:


	move.l		a0,.emh_cmp_src
	move.l		a1,.emh_cmp_dst
	move.w		d4,.emh_cmp_h
	
	move.w		.sp_maskwidth(sp),d6

*-------------------------------------------------------*
.emh_Clip_4BPL_unclipped_blit:
*-------------------------------------------------------*

;	move.w		.emh_cmp_masklineoff,d0
;	beq.s		.emh_noyi
;	move.w		d6,d5
;	add.w		d5,d5
;	mulu.w		.emh_cmp_masklineoff,d5
;	add.w		d5,a6
;.emh_noyi:

	move.l		a0,BLiTSRC.w

*-------------------------------------------------------*

	lea		BLiTYC.w,a0
	lea		BLiTCTRL.w,a5
	lea		BLiTDST.w,a3
	lea		BLiTEM1.w,a2
	lea		BLiTEM3.w,a4
	
	move.l		#$00ffc000,d1
.emh_skew_smc:
	move.b		#$55,d1
	
;	move.l		#$00ffc000,d1
;	move.b		BLiTSKEW.w,d1
	;moveq		#4,d2
	move.l		#$FF000004,d2
	
	move.w		_g_linebytes,d0

	move.w		.emh_cmp_dyadj,d3
	muls.w		d0,d3
	add.l		d3,a1
	add.w		.emh_cmp_dxadj,a1
	
	move.l		.emh_psm2_emdata,a6

	move.l		.emh_psm2_mprog,.emh_smc_call+2
.emh_smc_call:
	jsr		$123456

	; un-patch exit
	
	move.l		.emh_smc_pos,a4
	move.w		.emh_smc_save,(a4)

*-------------------------------------------------------*
.emh_4BPL_end:
*-------------------------------------------------------*

	move.l		.emh_next_psmask,d0

; clr.l		d0
	
	beq		.emh_4BPL_end_all
	move.l		d0,a6

;.yhyg:
;	not.w		$ffff8240.w
;	not.b		$ffff8240.w
;	bra		.yhyg

	move.l		.emh_sprframe,a0
	move.l		.emh_framebuf,a1
	movem.w		.emh_sprpos,d1-d4
	
;	move.w		_g_linebytes,d0
;	mulu.w		#64,d0
;	add.l		d0,a1

	sf		.emh_is_first_cmp

	bra		.emh_firstcmp
	
	
	
	
	




	move.l		.emh_cmp_cplanes,a0		; colour planes
	
	move.l		a6,d6
;	wordwidth of source mask
	move.w		(a6)+,d5		; spf_psm_wordwidth
	move.w		d5,.sp_maskwidth(sp)
	move.w		d5,BLiTXC.w		; xcount
	move.w		(a6)+,d0		; skew
	move.b		d0,BLiTSKEW.w
	move.w		(a6)+,BLiTSYI.w		; syinc
	move.w		(a6)+,BLiTDYI.w		; dyinc

	move.l		.emh_cmp_src,a0
	move.l		.emh_cmp_dst,a1

	move.w		(a6)+,d0		; wordoffset
	add.w		d0,d0
	add.w		d0,a0
	subq		#2,a0
	add.w		d0,d0
	add.w		d0,d0
	add.w		d0,a1

	; link to next component, if present
	move.l		(a6)+,d7
	beq.s		.emh_lastpsm
	add.l		d6,d7
.emh_lastpsm:
	move.l		d7,.emh_next_psmask			; spf_psm_nextcmp

*-------------------------------------------------------*
*	common impl. for general paths
*-------------------------------------------------------*
.emh_Clip_4BPL_unclipped_got2:
*-------------------------------------------------------*

	move.w		.emh_cmp_h,d4
	move.w		.sp_maskwidth(sp),d6

	altraster	#$021
			
	bra		.emh_Clip_4BPL_unclipped_blit

.emh_sprframe:
	ds.l		1
.emh_framebuf:
	ds.l		1
.emh_sprpos:
	ds.w		4
.emh_is_first_cmp:
	ds.w		1

	
.emh_cmp_height:
	ds.w		1
.emh_scratch:
	ds.w		1
.emh_smc_save:
	ds.w		1
.emh_smc_pos:
	ds.l		1
.emh_cmp_src:
	ds.l		1
.emh_cmp_dst:
	ds.l		1	
.emh_cmp_h:
	ds.w		1
;.emh_cmp_masklineoff:
;	ds.w		1
.emh_cmp_cplanes:
	ds.l		1
.emh_next_psmask:
	ds.l		1
	
.emh_psm2_emdata:
	ds.l		1
.emh_psm2_mprog:
	ds.l		1
.emh_psm2_mprog2:
	ds.l		1
.emh_cmp_dyadj:
	ds.w		1
.emh_cmp_dxadj:
	ds.w		1
	
	
.emh_4BPL_end_all:

	move.w		#2,BLiTSYI.w	; reset for default 32w EMS/EMX
	
	popraster
	