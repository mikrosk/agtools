
*-------------------------------------------------------*
;nfsr_case:
*-------------------------------------------------------*
	; combine width:shift lookup [WWWW:SSSS]
	and.w		#-16,d6
	or.w		d6,d1
	
	; dst skip adjust by dst width	
	lsr.w		#1,d6
	sub.w		d6,a4
	
	; width tab scaling
	lsr.w		#(4-1)-1,d6			; >>4 *2
	
	ifne		(enable_restore)
	ifne		(use_sprite_pagerestore)
	; clearing record
	move.l		a1,(a5)+			; clear addr
	move.w		a4,(a5)+			; dst skip
	move.w		d6,(a5)+			; word width*2
	move.w		d7,(a5)+			; lines
	endc
	endc
		
	; lookup impl.
	add.w		d1,d1
	move.w		.nfsr_table-(16*4)(pc,d1.w),d1
	jmp		.nfsr_table(pc,d1.w)
.nfsr_table:
;	shiftgroup	nfsr,.nfsr_table,g		; 1-wide with NFSR should be impossible - no data!
	shiftgroup	nfsr,.nfsr_table,w2		; 0-shift also impossible with NFSR
	shiftgroup	nfsr,.nfsr_table,w3
	shiftgroup	nfsr,.nfsr_table,g
	shiftgroup	nfsr,.nfsr_table,g
	shiftgroup	nfsr,.nfsr_table,g
	shiftgroup	nfsr,.nfsr_table,g
	shiftgroup	nfsr,.nfsr_table,g
	shiftgroup	nfsr,.nfsr_table,g

*-------------------------------------------------------*
nfsr_01_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$80008000,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
				
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		1
.x9:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_01_w2:
*-------------------------------------------------------*
	move.l		#$80008000,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		1	
.ylp:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_01_w3:
*-------------------------------------------------------*
	move.l		#$80008000,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		1	
.ylp:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_02_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$c000c000,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
				
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		2
.x9:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_02_w2:
*-------------------------------------------------------*
	move.l		#$c000c000,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		2	
.ylp:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_02_w3:
*-------------------------------------------------------*
	move.l		#$c000c000,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		2	
.ylp:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_03_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$e000e000,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
			
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		3
.x9:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_03_w2:
*-------------------------------------------------------*
	move.l		#$e000e000,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		3	
.ylp:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_03_w3:
*-------------------------------------------------------*
	move.l		#$e000e000,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		3	
.ylp:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_04_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$f000f000,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
				
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		4
.x9:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_04_w2:
*-------------------------------------------------------*
	move.l		#$f000f000,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		4	
.ylp:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
		
*-------------------------------------------------------*
nfsr_04_w3:
*-------------------------------------------------------*
	move.l		#$f000f000,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		4	
.ylp:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_05_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$f800f800,d0
			
	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl			
			
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		5
.x9:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_05_w2:
*-------------------------------------------------------*
	move.l		#$f800f800,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		5	
.ylp:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_05_w3:
*-------------------------------------------------------*
	move.l		#$f800f800,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		5	
.ylp:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_06_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$fc00fc00,d0
	
	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl	
				
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		6
.x9:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts			

*-------------------------------------------------------*
nfsr_06_w2:
*-------------------------------------------------------*
	move.l		#$fc00fc00,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		6	
.ylp:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_06_w3:
*-------------------------------------------------------*
	move.l		#$fc00fc00,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		6	
.ylp:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_07_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$fe00fe00,d0
			
	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl				
			
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		7
.x9:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_07_w2:
*-------------------------------------------------------*
	move.l		#$fe00fe00,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		7	
.ylp:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_07_w3:
*-------------------------------------------------------*
	move.l		#$fe00fe00,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		7	
.ylp:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_08_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$ff00ff00,d0
			
	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
			
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		8
.x9:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFLmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_08_w2:
*-------------------------------------------------------*
	move.l		#$ff00ff00,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		8	
.ylp:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
		
*-------------------------------------------------------*
nfsr_08_w3:
*-------------------------------------------------------*
	move.l		#$ff00ff00,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		8	
.ylp:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFLmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFLmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFLmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
			
*-------------------------------------------------------*
nfsr_09_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$ff80ff80,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
			
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		9
.x9:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts		

*-------------------------------------------------------*
nfsr_09_w2:
*-------------------------------------------------------*
	move.l		#$ff80ff80,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		9	
.ylp:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_09_w3:
*-------------------------------------------------------*
	move.l		#$ff80ff80,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		9	
.ylp:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_10_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$ffc0ffc0,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
				
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		10
.x9:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_10_w2:
*-------------------------------------------------------*
	move.l		#$ffc0ffc0,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		10	
.ylp:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts	

*-------------------------------------------------------*
nfsr_10_w3:
*-------------------------------------------------------*
	move.l		#$ffc0ffc0,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		10	
.ylp:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
			
*-------------------------------------------------------*
nfsr_11_g:
*-------------------------------------------------------*
	
	; init shifted combinemask

	move.l		#$ffe0ffe0,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
			
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		11
.x9:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_11_w2:
*-------------------------------------------------------*
	move.l		#$ffe0ffe0,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		11	
.ylp:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_11_w3:
*-------------------------------------------------------*
	move.l		#$ffe0ffe0,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		11	
.ylp:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
		
*-------------------------------------------------------*
nfsr_12_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$fff0fff0,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
			
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		12
.x9:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_12_w2:
*-------------------------------------------------------*
	move.l		#$fff0fff0,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		12	
.ylp:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
		
*-------------------------------------------------------*
nfsr_12_w3:
*-------------------------------------------------------*
	move.l		#$fff0fff0,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		12	
.ylp:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
		
*-------------------------------------------------------*
nfsr_13_g:
*-------------------------------------------------------*
	
	; init shifted combinemask

	move.l		#$fff8fff8,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
				
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		13
.x9:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_13_w2:
*-------------------------------------------------------*
	move.l		#$fff8fff8,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		13	
.ylp:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_13_w3:
*-------------------------------------------------------*
	move.l		#$fff8fff8,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		13	
.ylp:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
		
*-------------------------------------------------------*
nfsr_14_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$fffcfffc,d0

	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
			
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		14
.x9:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_14_w2:
*-------------------------------------------------------*
	move.l		#$fffcfffc,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		14	
.ylp:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
		
*-------------------------------------------------------*
nfsr_14_w3:
*-------------------------------------------------------*
	move.l		#$fffcfffc,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		14	
.ylp:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
			
*-------------------------------------------------------*
nfsr_15_g:
*-------------------------------------------------------*

	; init shifted combinemask

	move.l		#$fffefffe,d0
		
	move.w		.dbtab(pc,d6.w),.xctrl+2
	bra		.go
.dbtab:	dbradef.w	.x0,.xctrl
	dbradef.w	.x1,.xctrl
	dbradef.w	.x2,.xctrl
	dbradef.w	.x3,.xctrl
	dbradef.w	.x4,.xctrl
	dbradef.w	.x5,.xctrl
	dbradef.w	.x6,.xctrl
	dbradef.w	.x7,.xctrl
	dbradef.w	.x8,.xctrl
	dbradef.w	.x9,.xctrl
				
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
.ylp_nisr:
;	jmp		(a2)
.shift	set		15
.x9:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x8:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x7:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x6:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x5:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x4:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x3:	shiftFHmaskC	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x2:	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
.x1:	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
.x0:	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.go:	moveq		#0,d2				; wipe shift registers
	moveq		#0,d3
	moveq		#0,d4				; wipe shift registers
	moveq		#0,d5
.xctrl:	dbra		d7,.ylp_nisr
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts

*-------------------------------------------------------*
nfsr_15_w2:
*-------------------------------------------------------*
	move.l		#$fffefffe,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		15	
.ylp:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
nfsr_15_w3:
*-------------------------------------------------------*
	move.l		#$fffefffe,d0
	moveq		#4-1,d6
	and.w		d7,d6
	lsr.w		#2,d7
	add.w		d6,d6
	move.w		.bt(pc,d6.w),d6
	jmp		.bt(pc,d6.w)
.bt:	dc.w		.y0-.bt
	dc.w		.y1-.bt
	dc.w		.y2-.bt
	dc.w		.y3-.bt
.shift	set		15	
.ylp:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y3:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y2:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y1:	shiftFHmaskC_I	a0,a1,d2,d3,d4,d5,d0,d1,d6
	shiftFHmaskC	a0,a1,d4,d5,d2,d3,d0,d1,d6
	shiftFHmaskC_N	a0,a1,d2,d3,d4,d5,d0,d1,d6
	add.l		a3,a0				; + src skip
	add.l		a4,a1				; + dst lineskip
.y0:	dbra		d7,.ylp
	ifne		(enable_restore)
	ifne		(use_sprite_saverestore|use_sprite_pagerestore)
	move.l		a6,(a5)+
	move.l		a5,usp
	endc
	endc
	rts
	
*-------------------------------------------------------*
*	NFSR can only occur when shift >0
*-------------------------------------------------------*
nfsr_00_g:
nfsr_00_w1:
nfsr_00_w2:
nfsr_00_w3:
	rts
