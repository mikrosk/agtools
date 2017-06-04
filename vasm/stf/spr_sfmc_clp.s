
	add.w		d0,d0				; todo: game layer can prescale this
	add.w		d0,d0
	move.l		sps_frameindex(a0,d0.w),a0	; access sprite frame data

	; initial source skip (unclipped case)
	
	sub.l		a3,a3
	
	; initial dst skip
	
;	lea		linebytes.w,a4			; dst stride = linebytes-spritedstlinebytes
	move.w		_g_linebytes,a4

	
	; frame fields
	
	move.w		spf_w(a0),d3			; w
	move.w		spf_h(a0),d4			; h
	add.w		spf_xo(a0),d1			; +xo
	add.w		spf_yo(a0),d2			; +yo
	lea		spf_data(a0),a0

	sub.w		_g_xpreshift,d1

	; sprite clipping window
	lea		_g_spr_window,a2
	
	; constants
	moveq		#16-1,d6
	moveq		#-16,d7
	
*-------------------------------------------------------*
*	top edge
*-------------------------------------------------------*
	cmp.w		clip_y1(a2),d2
	bpl.s		.clip_safe_top
*-------------------------------------------------------*
*	top clipped
*-------------------------------------------------------*
	move.w		clip_y1(a2),d5
	; relative to clipwindow
	sub.w		d5,d2
	; crop lines
	add.w		d2,d4
	ble		.clip_out
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
	bra.s		.clip_safe_bot
*-------------------------------------------------------*
.clip_safe_top:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	bottom edge
*-------------------------------------------------------*
	move.w		d2,d0
	add.w		d4,d0				; xr	
	sub.w		clip_y2(a2),d0			; xr - cx2
	ble.s		.clip_safe_bot
*-------------------------------------------------------*
*	bottom clipped	
*-------------------------------------------------------*
	sub.w		d0,d4
	ble		.clip_out
*-------------------------------------------------------*
.clip_safe_bot:
*-------------------------------------------------------*


*-------------------------------------------------------*
*	left edge - safety-clipping only! no FISR!
*-------------------------------------------------------*

*-------------------------------------------------------*
*	left edge
*-------------------------------------------------------*
	cmp.w		clip_x1(a2),d1
	bpl		.clip_safe_left
*-------------------------------------------------------*
*	left clipped
*-------------------------------------------------------*
	move.w		clip_x1(a2),d5
	; relative to clipwindow
	sub.w		d5,d1
	; find minimum pixelwords off left edge
	move.w		d1,d0
	neg.w		d0				; pixels off left
	; adjust dest width (ignore FISR part)
	cmp.w		d0,d3
	ble		.clip_out
	add.w		d6,d0				;#16-1,d0			; round up
	and.w		d7,d0				;#-16,d0
	sub.w		d0,d3

	; adjust source offset and src skip by clipped pixelwords-1 (include FISR part)
	lsr.w		#1,d0				; in terms of src pixelwords
	subq.w		#8,d0				; adjust for FISR
	add.w		d0,a3				; src skip
	add.w		d0,a0				; src offset		
	; crop offset, retain shift
	and.w		d6,d1				;#16-1,d1
	; relative to world
	add.w		d5,d1	
	; ASSUMPTION: sprite is not big enough to cross left+right
;	bra		.clip_go


;	duplicate SVMC implementation here for FISR case
;	SVMC is slower than SFMC but does handle FISR for corrct left edge clip

*-------------------------------------------------------*
*	we only get here if sprite is off left edge
*	this is the most expensive case, so best avoid
*	for large sprites - use STFM_SprNoClipOpt path
*	instead
*-------------------------------------------------------*
.STF_ISSprDrawClipped_FISR:
*-------------------------------------------------------*
;	if		(use_sprite_saverestore)
;	move.l		_drawcontext+dctx_clearstack_tide,a5
;	endc

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

	moveq		#-1,d0
	lsr.w		d1,d0
	not.w		d0
	move.w		d0,d3
	swap		d0
	move.w		d3,d0
				
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
.ylp_fisr:
	shiftVMC_FISR	a0,a1,d2,d3,d4,d5,d0,d1,d6
	jmp		(a2)
;.ylp_nisr:
;	moveq		#0,d4				; wipe shift registers
;	moveq		#0,d5
;	jmp		(a2)
.x9:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x7:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x5:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x3:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x1:	shiftVMC2	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	sub.w		#$100,d7
.xctrl:	bcc.w		.ylp_fisr
;	dbra		d7,.ylp_nisr

	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
;	move.l		a5,_drawcontext+dctx_clearstack_tide
	endc
	endc
	
	bra		.clip_out

*-------------------------------------------------------*
.clip_safe_left:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	right edge
*-------------------------------------------------------*
	move.w		d1,d0
	add.w		d3,d0				; xr	
	sub.w		clip_x2(a2),d0			; xr - cx2
	ble		.clip_go
*-------------------------------------------------------*
*	right clipped
*-------------------------------------------------------*
	; find minimum pixelwords off right edge
	; adjust dest width
	cmp.w		d0,d3
	ble		.clip_out
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
	
	; dst skip adjust by dst width	
	lsr.w		#1,d6
	sub.w		d6,a4

	; final height
	move.w		d4,d7				; h
	
	; init shifted combinemask

	moveq		#-1,d0
	lsr.w		d1,d0
	not.w		d0
	move.w		d0,d3
	swap		d0
	move.w		d3,d0

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

	lea		$100.w,a2
	move.w		.fdbt(pc,d6.w),.fxc+4
	bra		.fgo
.fdbt:	dc.w		(.fx0-.fxc)-4
	dc.w		(.fx1-.fxc)-4
	dc.w		(.fx2-.fxc)-4
	dc.w		(.fx3-.fxc)-4
	dc.w		(.fx4-.fxc)-4
	dc.w		(.fx5-.fxc)-4
	dc.w		(.fx6-.fxc)-4
	dc.w		(.fx7-.fxc)-4
	dc.w		(.fx8-.fxc)-4
	dc.w		(.fx9-.fxc)-4

;	dbradef.w	.x0,.xctrl
;	dbradef.w	.x1,.xctrl
;	dbradef.w	.x2,.xctrl
;	dbradef.w	.x3,.xctrl
;	dbradef.w	.x4,.xctrl
;	dbradef.w	.x5,.xctrl
;	dbradef.w	.x6,.xctrl
;	dbradef.w	.x7,.xctrl
;	dbradef.w	.x8,.xctrl
;	dbradef.w	.x9,.xctrl

;	move.w		.jt(pc,d6.w),d6
;	lea		.jt(pc,d6.w),a2
;	bra		.xctrl	
;.jt:	dc.w		.x0-.jt
;	dc.w		.x1-.jt
;	dc.w		.x2-.jt
;	dc.w		.x3-.jt
;	dc.w		.x4-.jt
;	dc.w		.x5-.jt
;	dc.w		.x6-.jt
;	dc.w		.x7-.jt
;	dc.w		.x8-.jt
;	dc.w		.x9-.jt

;	1-128 pixel source width ::> 16->144 pixel dest width under shift
.fylp:
.fx9:	shiftVMC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.fx8:	shiftVMC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.fx7:	shiftVMC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.fx6:	shiftVMC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.fx5:	shiftVMC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.fx4:	shiftVMC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.fx3:	shiftVMC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.fx2:	shiftVMC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.fx1:	shiftVMC_F	a0,a1,d2,d3,d4,d5,d0,d1,d6
.fx0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.fgo:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.fxc:	sub.w		a2,d7
	bcc.w		.fylp

	;dbra		d7,.ylp_nisr

	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
;	move.l		a5,_drawcontext+dctx_clearstack_tide
	endc
	endc

	bra		.clip_out	
	
;	bra.s		STFM_SprOpt4BPL_go
*-------------------------------------------------------*
.clip_go:
*-------------------------------------------------------*
	jsr		STF_ISSprOpt_impl_
*-------------------------------------------------------*
.clip_out:
*-------------------------------------------------------*
