*-------------------------------------------------------*
*	common impl. for general paths
*-------------------------------------------------------*
.STF_ISSprDrawUnClipped_go:
*-------------------------------------------------------*

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
	beq		.STF_ISSprDrawUnClipped_FSR

*-------------------------------------------------------*
.STF_ISSprDrawUnClipped_NFSR:
*-------------------------------------------------------*

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
	move.w		.ndbt(pc,d6.w),.nxc+4
	bra		.ngo
.ndbt:	dc.w		(.nx0-.nxc)-4
	dc.w		(.nx1-.nxc)-4
	dc.w		(.nx2-.nxc)-4
	dc.w		(.nx3-.nxc)-4
	dc.w		(.nx4-.nxc)-4
	dc.w		(.nx5-.nxc)-4
	dc.w		(.nx6-.nxc)-4
	dc.w		(.nx7-.nxc)-4
	dc.w		(.nx8-.nxc)-4
	dc.w		(.nx9-.nxc)-4


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
.nylp:
.nx9:	shiftVMC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.nx8:	shiftVMC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.nx7:	shiftVMC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.nx6:	shiftVMC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.nx5:	shiftVMC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.nx4:	shiftVMC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.nx3:	shiftVMC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.nx2:	shiftVMC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.nx1:	shiftVMC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.nx0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.ngo:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.nxc:	sub.w		a2,d7
	bcc.w		.nylp
	
	bra		.STF_ISSprDrawUnClipped_recover
	
;	if		(use_sprite_saverestore)
;	move.l		a6,(a5)+
;	move.l		a5,_drawcontext+dctx_clearstack_tide
;	endc
;	
*-------------------------------------------------------*
;	move.l		(sp)+,a6
;	unlk		a6
;	movem.l		(sp)+,d2-d7/a2-a5
*-------------------------------------------------------*
;	rts
	
*-------------------------------------------------------*
.STF_ISSprDrawUnClipped_FSR:
*-------------------------------------------------------*

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

.STF_ISSprDrawUnClipped_recover:
	
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
;	move.l		a5,_drawcontext+dctx_clearstack_tide
	endc
	endc
