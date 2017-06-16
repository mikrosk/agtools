
*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	IMSPR software sprite implementation
*-------------------------------------------------------*

	ifd		AGT_CONFIG_SAFETY_CHECKS
;	sanity check on frame index!
	cmp.w		sps_framecount(a0),d0
	ifne		(within_entitydraw)
	bhs		_AGT_Exception_IMSFrame_68k
	else
	blo.s		.ims_frameok
.ims_framefault:
	not.w		$ffff8240.w
	bra.s		.ims_framefault
.ims_frameok:
	endc
	endc
		
	add.w		d0,d0				; todo: game layer can prescale this
	add.w		d0,d0
	move.l		sps_frameindex(a0,d0.w),a0	; access sprite frame data

	; frame fields
	
	move.w		spf_w(a0),d3			; w
	move.w		spf_h(a0),d4			; h
	add.w		spf_xo(a0),d1			; +xo
	add.w		spf_yo(a0),d2			; +yo
	
	sub.w		_g_xpreshift,d1

	; sprite clipping window
	lea		_g_spr_window,a2
	
	; initial dst skip
	
;	lea		linebytes.w,a4			; dst stride = linebytes-spritedstlinebytes
	move.w		_g_linebytes,a4

*-------------------------------------------------------*

	lea		spf_data(a0),a0

	; initial source skip (unclipped case)
	
	sub.l		a3,a3
	
	; constants
	moveq		#16-1,d6
	moveq		#-16,d7
	
*-------------------------------------------------------*
*	top edge
*-------------------------------------------------------*
	cmp.w		clip_y1(a2),d2
	bpl.s		.STF_ISSprDrawClipped_clip_safe_top
*-------------------------------------------------------*
*	top clipped
*-------------------------------------------------------*
	move.w		clip_y1(a2),d5
	; relative to clipwindow
	sub.w		d5,d2
	; crop lines
	add.w		d2,d4
	ble		.STF_ISSprDrawClipped_clip_out
	; adjust src offset in pixelwords*planes
	neg.w		d2
	move.w		d3,d0
	add.w		d6,d0
	and.w		d7,d0
	lsr.w		#1,d0
	mulu.w		d0,d2
	add.w		d2,a0	
	; relative to world
	move.w		d5,d2
	; ASSUMPTION: sprite is not big enough to cross top+bottom
	bra.s		.STF_ISSprDrawClipped_clip_safe_bot
*-------------------------------------------------------*
.STF_ISSprDrawClipped_clip_safe_top:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	bottom edge
*-------------------------------------------------------*
	move.w		d2,d0
	add.w		d4,d0				; xr	
	sub.w		clip_y2(a2),d0			; xr - cx2
	ble.s		.STF_ISSprDrawClipped_clip_safe_bot
*-------------------------------------------------------*
*	bottom clipped	
*-------------------------------------------------------*
	sub.w		d0,d4
	ble		.STF_ISSprDrawClipped_clip_out
*-------------------------------------------------------*
.STF_ISSprDrawClipped_clip_safe_bot:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	left edge
*-------------------------------------------------------*
	; NISR: no initial source read (i.e. shiftregisters preloaded with 0)
;	dbramoda	.ylp_nisr,.xctrl
	cmp.w		clip_x1(a2),d1
	bpl.s		.STF_ISSprDrawClipped_clip_safe_left
*-------------------------------------------------------*
*	left clipped
*-------------------------------------------------------*
	; FXSR: force initial source read (i.e. shiftregisters preloaded with 1st data word)
;	dbramoda	.ylp_fxsr,.xctrl
	move.w		clip_x1(a2),d5
	; relative to clipwindow
	sub.w		d5,d1
	; find minimum pixelwords off left edge
	move.w		d1,d0
	neg.w		d0				; pixels off left
	; adjust dest width (ignore FXSR part)
	cmp.w		d0,d3
	ble		.STF_ISSprDrawClipped_clip_out
	add.w		d6,d0				;#16-1,d0			; round up
	and.w		d7,d0				;#-16,d0
	sub.w		d0,d3
	; adjust source offset and src skip by clipped pixelwords-1 (include FXSR part)
	lsr.w		#1,d0				; in terms of src pixelwords
	subq.w		#8,d0				; adjust for FXSR
	add.w		d0,a3				; src skip
	add.w		d0,a0				; src offset		
	; crop offset, retain shift
	and.w		d6,d1				;#16-1,d1
	; relative to world
	add.w		d5,d1	
	; ASSUMPTION: sprite is not big enough to cross left+right
;	bra		.clip_safe_right
	bra		.STF_ISSprDrawClipped_clip_unsafe_left
*-------------------------------------------------------*
.STF_ISSprDrawClipped_clip_safe_left:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	right edge
*-------------------------------------------------------*
	move.w		d1,d0
	add.w		d3,d0				; xr	
	sub.w		clip_x2(a2),d0			; xr - cx2

;	ble		.STF_ISSprDrawUnClipped_go	;.clip_safe_right

	ble.s		.STF_ISSprDrawClipped_clip_safe_right
*-------------------------------------------------------*
*	right clipped
*-------------------------------------------------------*

	; find minimum pixelwords off right edge
;	add.w		d6,d0				;#16-1,d0
;	and.w		d7,d0				;#-16,d0
	; adjust dest width
	cmp.w		d0,d3
	ble		.STF_ISSprDrawClipped_clip_out
	add.w		d6,d0				;#16-1,d0
	and.w		d7,d0				;#-16,d0
	sub.w		d0,d3
	; adjust src skip
	lsr.w		#1,d0
	add.w		d0,a3

	; dst skip: a4

	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		usp,a5
	move.l		a5,a6
	endc
	endc

	; constants
	moveq		#16-1,d6
	moveq		#-16,d7
	
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

	; NFSR test
	
	and.w		d7,d0
	sub.w		d6,d0
	beq.s		.zz
	subq		#8,a3
.zz:

	bra		.STF_ISSprDrawUnClipped_EFSR


*-------------------------------------------------------*
.STF_ISSprDrawClipped_clip_safe_right:
*-------------------------------------------------------*

;	bra		STFM_ISSprDrawUnClipped_go

*-------------------------------------------------------*

	; unclipped case is a subset of clipped
	
	; dst skip: a4
		
	include		"stf/spr_svmc_unc.s"
	
	bra		.STF_ISSprDrawClipped_recover


*-------------------------------------------------------*
.STF_ISSprDrawClipped_clip_unsafe_left:
*-------------------------------------------------------*

;	bra		.STF_ISSprDrawClipped_clip_out

*-------------------------------------------------------*
*	we only get here if sprite is off left edge
*	this is the most expensive case, so best avoid
*	for large sprites - use STFM_SprNoClipOpt path
*	instead
*-------------------------------------------------------*
.STF_ISSprDrawClipped_FXSR:
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

	; use SMC patch for FSR/NFSR case

	lea		.x1(pc),a2
	and.w		d0,d7
	sub.w		d6,d7
	beq.s		.efsr
.nfsr	move.l		#$74007600,(a2)+		; moveq #0,d2; moveq #0,d3
	move.l		#$4e714e71,(a2)+		; nop; nop
	bra.s		.ok
.efsr:	move.l		#$24182618,(a2)+		; (a0)+,d2; (a0)+,d3
	move.l		#$eebaeebb,(a2)+		; ror.l d7,d2; ror.l d7,d3
.ok:	


	; dst skip adjust by dst width	
	lsr.w		#1,d6
	sub.w		d6,a4

	; shift
;	move.l		d1,d7				; rshift
;	swap		d7
	
	; final height

	move.w		d4,d7				; h

	; init shifted combinemask

	scrollmaskVMC	d1,d0,d3
				
	; width tab scaling				
	lsr.w		#(4-1)-1,d6			; >>4 *2

	ifne		(enable_restore)
	ifne		(use_sprite_pagerestore)
;	; clearing record
	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d7,(a5)+			; lines
	endc
	endc
	
	lsl.w		#8,d7
	or.w		d1,d7

	move.w		.jt(pc,d6.w),d6
	lea		.jt(pc,d6.w),a2
	bra		.go
.jt:	dc.w		.x0-.jt
	dc.w		.x1-.jt
	dc.w		.x2-.jt
	dc.w		.x3-.jt
	dc.w		.x4-.jt
	dc.w		.x5-.jt
	dc.w		.x6-.jt
	dc.w		.x7-.jt
	dc.w		.x8-.jt
	dc.w		.x9-.jt

;	1-128 pixel source width ::> 16->144 pixel dest width under shift
.ylp_fxsr:
	shiftVMC_B_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
	jmp		(a2)
.x9:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
.x8:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
.x7:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
.x6:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
.x5:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
.x4:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
.x3:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
.x2:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	swap		d4
	swap		d5
.x1:	shiftVMC_M_FXSR	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:

	sub.w		#$100,d7
.xctrl:	bcc.w		.ylp_fxsr	;.ylp_nisr

	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	
*-------------------------------------------------------*
.STF_ISSprDrawClipped_clip_out:
*-------------------------------------------------------*

*-------------------------------------------------------*
.STF_ISSprDrawClipped_recover:
*-------------------------------------------------------*
