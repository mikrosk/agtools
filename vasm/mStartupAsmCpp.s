
; --------------------------------------------------------------
        
	xref	___CTOR_LIST__
	xref	___DTOR_LIST__

        xdef    ___cxa_pure_virtual
	xdef	__ZSt17__throw_bad_allocv

	xdef	_memcpy
	xdef	_memset
;	xdef	_qmemclr
	xref	_qmemclr
	
	xref	_qmemcpy
	xref	_qmemset

	xref	_main
	xdef	_exit
	xdef	___cxa_guard_acquire
	xdef	___cxa_guard_release

; --------------------------------------------------------------

BASEPAGE_SIZE		=		$100
USPS			=		$100*4
SSPS			=		$4000

; --------------------------------------------------------------

*-------------------------------------------------------*
__crt_entrypoint:	xdef		__crt_entrypoint
*-------------------------------------------------------*
;	bra		*
	
	pea		.here(pc)
	move.w		#38,-(sp)
	trap		#14
	addq.l		#6,sp
	bra.s		.cont
.here:	;move.l		#512*1024,$436
	;move.l		#512*1024,$44e
	;move.l		#512*1024,$42E
	rts
.cont:	
	
	move.l		4(sp),a5
*-------------------------------------------------------*
*	command info
*-------------------------------------------------------*
;	lea		128(a5),a4
;	move.l		a4,cli
*-------------------------------------------------------*
*	Mshrink
*-------------------------------------------------------*
	move.l		12(a5),d0			; text segment
	add.l		20(a5),d0			; data segment
	add.l		28(a5),d0			; bss segment
	add.l		#BASEPAGE_SIZE+USPS,d0		; base page
;	add.l		#BASEPAGE_SIZE,d0		; base page
*-------------------------------------------------------*
	move.l		a5,d1				; address to basepage
	add.l		d0,d1				; end of program
	and.w		#-16,d1				; align stack
	move.l		sp,d2
	move.l		d1,sp				; temporary USP stackspace
	move.l		d2,-(sp)	
*-------------------------------------------------------*
	move.l		d0,-(sp)
	move.l		a5,-(sp)
	clr.w		-(sp)
	move.w		#$4a,-(sp)
	trap		#1				; Mshrink
	lea		12(sp),sp	
*-------------------------------------------------------*
*	Program
*-------------------------------------------------------*
	bsr		user_start
*-------------------------------------------------------*
*	Begone
*-------------------------------------------------------*
;	move.l		(sp)+,sp			; restore USP
	clr.w		-(sp)				; Pterm0
	trap		#1

user_start:

;	move.l		4(sp),a5				;address to basepage
;	move.l		$0c(a5),d0				;length of text segment
;	add.l		$14(a5),d0				;length of data segment
;	add.l		$1c(a5),d0				;length of bss segment
;	add.l		#STACK_SIZE+BASEPAGE_SIZE,d0		;length of stackpointer+basepage
;	move.l		a5,d1					;address to basepage
;	add.l		d0,d1					;end of program
;	and.w		#-16,d1					;align stack

;	move.l		sp,d2
;	move.l		d1,sp					;new stackspace
;	move.l		d2,-(sp)
	
;	move.l		d0,-(sp)				;mshrink()
;	move.l		a5,-(sp)				;
;	clr.w		-(sp)					;
;	move.w		#$4a,-(sp)				;
;	trap		#1					;
;	lea.l		12(sp),sp				;
			
	; clear bss segment
			
	move.l		$18(a5),a0
	move.l		$1c(a5),d0				;length of bss segment

	move.l		d0,-(sp)
	move.l		a0,-(sp)
	jsr		_qmemclr
	addq.l		#8,sp

	
;	if (REDIRECT_OUTPUT_TO_SERIAL==1)  
;	; redirect to serial
;	
;	move.w		#2,-(sp)
;	move.w		#1,-(sp)
;	move.w		#$46,-(sp)
;	trap		#1
;	addq.l		#6,sp
;	endif
	; execute static constructors
	
	lea		___CTOR_LIST__,a0
	bsr		static_init

;	jsr		_main
	
	pea		super_start
	move.w		#38,-(sp)
	trap		#14
	addq.l		#6,sp

	lea		___DTOR_LIST__,a0
	bsr		static_init

;	move.w		#1,-(sp)				; Cconin
;	trap		#1
;	addq.l		#2,sp

	rts
		
;	clr.w		-(sp)					; Pterm0
;	trap		#1

; --------------------------------------------------------------
super_start:
; --------------------------------------------------------------
	lea		new_ssp,a0
	move.l		a0,d0
	subq.l		#4,d0	
	and.w		#-16,d0
	move.l		d0,a0
	move.l		sp,-(a0)
	move.l		a0,sp
	
	jsr		_main
	
_exit:	move.l		(sp)+,sp
	rts
	
; --------------------------------------------------------------
static_init:	
; --------------------------------------------------------------
	move.l		(a0)+,d0
	beq.s		.none
.next:	move.l		(a0)+,a1
	movem.l		d0/a0,-(sp)
	moveq		#0,d0
	jsr		(a1)
	movem.l		(sp)+,d0/a0
	subq.l		#1,d0
	bne.s		.next
.none:	rts


	if		(1)
	
; --------------------------------------------------------------
_memcpy:	
; --------------------------------------------------------------
	jmp		_qmemcpy

	if		(0)
		
	move.l		4(sp),a0
	move.l		8(sp),a1

	move.l		12(sp),d1
	lsr.l		#4,d1
	move.l		d1,d0
	swap		d0
	subq.w		#1,d1	
	bmi.s		.ev1mb

.lp1mb:
.lp16b:	move.l		(a1)+,(a0)+
	move.l		(a1)+,(a0)+
	move.l		(a1)+,(a0)+
	move.l		(a1)+,(a0)+
	dbra		d1,.lp16b

.ev1mb:	subq.w		#1,d0
	bpl.s		.lp1mb

	moveq		#16-1,d1
	and.w		12+2(sp),d1
	lsl.b		#4+1,d1
	bcc.s		.n8
	move.l		(a1)+,(a0)+
	move.l		(a1)+,(a0)+
.n8:	add.b		d1,d1
	bcc.s		.n4
	move.l		(a1)+,(a0)+
.n4:	add.b		d1,d1
	bcc.s		.n2
	move.w		(a1)+,(a0)+
.n2:	add.b		d1,d1
	bcc.s		.n1
	move.b		(a1)+,(a0)+
.n1:
	move.l		4(sp),d0
	rts

	endif
	
; --------------------------------------------------------------
_memset:
; --------------------------------------------------------------
	jmp		_qmemset

	if		(0)
	
	move.l		d2,a1
	move.l		4(sp),a0

	move.w		8+3(sp),d1
	move.b		8+3(sp),d1
	move.w		d1,d2
	swap		d2
	move.w		d1,d2
	
	move.l		12(sp),d1
	lsr.l		#4,d1
	move.l		d1,d0
	swap		d0
	subq.w		#1,d1	
	bmi.s		.ev1mb

.lp1mb:
.lp16b:	move.l		d2,(a0)+
	move.l		d2,(a0)+
	move.l		d2,(a0)+
	move.l		d2,(a0)+
	dbra		d1,.lp16b

.ev1mb:	subq.w		#1,d0
	bpl.s		.lp1mb

	moveq		#16-1,d1
	and.w		12+2(sp),d1
	lsl.b		#4+1,d1
	bcc.s		.n8
	move.l		d2,(a0)+
	move.l		d2,(a0)+
.n8:	add.b		d1,d1
	bcc.s		.n4
	move.l		d2,(a0)+
.n4:	add.b		d1,d1
	bcc.s		.n2
	move.w		d2,(a0)+
.n2:	add.b		d1,d1
	bcc.s		.n1
	move.b		d2,(a0)+
.n1:
	move.l		a1,d2
	move.l		4(sp),d0
	rts

	endif	

	if		(0)		
; --------------------------------------------------------------
_memclr:
; --------------------------------------------------------------
	move.l		4(sp),a0

	move.l		8(sp),d1
	lsr.l		#4,d1
	move.l		d1,d0
	swap		d0
	subq.w		#1,d1	
	bmi.s		.ev1mb

.lp1mb:
.lp16b:	clr.l		(a0)+
	clr.l		(a0)+
	clr.l		(a0)+
	clr.l		(a0)+
	dbra		d1,.lp16b

.ev1mb:	subq.w		#1,d0
	bpl.s		.lp1mb

	moveq		#16-1,d1
	and.w		8+2(sp),d1
	
	lsl.b		#4+1,d1
	bcc.s		.n8
	clr.l		(a0)+
	clr.l		(a0)+
.n8:	add.b		d1,d1
	bcc.s		.n4
	clr.l		(a0)+
.n4:	add.b		d1,d1
	bcc.s		.n2
	clr.w		(a0)+
.n2:	add.b		d1,d1
	bcc.s		.n1
	clr.b		(a0)+
.n1:
	;move.l		4(sp),d0
	rts
	endif
	
	endif
	
; --------------------------------------------------------------

_rand:		XDEF	_rand
___cxa_guard_acquire:
___cxa_guard_release:
		rts
		
; --------------------------------------------------------------
__ZSt17__throw_bad_allocv:
___cxa_pure_virtual:	
; --------------------------------------------------------------
	jmp	_exit

; --------------------------------------------------------------
	bss
; --------------------------------------------------------------
	
	ds.b	SSPS
new_ssp:
	ds.l	1

; --------------------------------------------------------------

