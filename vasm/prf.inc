*=======================================================*
*	builtin profiler contexts
*=======================================================*

prf_ctx			=	$40
ctxstack		=	$44
prf_context_max		=	32
prf_context_depth	=	32

*-------------------------------------------------------*

; interrupt-safe
pushctx			macro
	ifd		AGT_CONFIG_PROFILER
	move.l		a0,-(sp)
	ori.w		#$0700,sr
	move.l		ctxstack.w,a0
	move.w		prf_ctx.w,-(a0)
	ifd		prf_\1
	move.w		#prf_\1,prf_ctx.w
	endc
	move.l		a0,ctxstack.w
	andi.w		#$fbff,sr
	move.l		(sp)+,a0
	endc
	endm

; interrupt-safe
popctx			macro
	ifd		AGT_CONFIG_PROFILER
	move.l		a0,-(sp)
	ori.w		#$0700,sr
	move.l		ctxstack.w,a0	
	move.w		(a0)+,prf_ctx.w
	move.l		a0,ctxstack.w
	andi.w		#$fbff,sr
	move.l		(sp)+,a0
	endc
	endm
	
; interrupt-safe
pushctxr		macro
	ifd		AGT_CONFIG_PROFILER
	ori.w		#$0700,sr
	move.l		ctxstack.w,\2
	move.w		prf_ctx.w,-(\2)
	ifd		prf_\1
	move.w		#prf_\1,prf_ctx.w
	endc
	move.l		\2,ctxstack.w
	andi.w		#$fbff,sr
	endc
	endm

; interrupt-safe
popctxr			macro
	ifd		AGT_CONFIG_PROFILER
	ori.w		#$0700,sr
	move.l		ctxstack.w,\1
	move.w		(\1)+,prf_ctx.w
	move.l		\1,ctxstack.w	
	andi.w		#$fbff,sr
	endc
	endm
	
; interrupt-safe
setctx			macro
	ifd		AGT_CONFIG_PROFILER
	ifd		prf_\1
	move.w		#prf_\1,prf_ctx.w
	endc
	endc
	endm
	
*-------------------------------------------------------*

; these definitions are only required for asm-side definitions used by lowest level components
; most of the profiling definitions are on the C side

*-------------------------------------------------------*

		rsreset
e_prf_text:	rs.l	1
e_prf_id:	rs.l	1
e_prf_size_:	rs.l	0
		
prf_context_def	macro
;		text
prf_\2		set	prf_next_
prf_next_	set	prf_\2+1
;		dc.l	prf_\2_t,prf_\2
;		data
;prf_\2_t:	dc.b	\1,0
		endm	
	
*-------------------------------------------------------*


prf_next_			set	0
;prf_context_refs:

	prf_context_def		"undefined...",undefined
	prf_context_def		"-profiling- ",profiling
	prf_context_def		"main:loop   ",mainloop
	prf_context_def		"main:idle   ",idle
	prf_context_def		"arena:misc  ",arena_misc
	prf_context_def		"arena:bgrst ",arena_bgrestore
	prf_context_def		"arena:fill  ",arena_fill
	prf_context_def		"ent:tick    ",ent_tick
	prf_context_def		"ent:spatial ",ent_spatial
	prf_context_def		"ent:interact",ent_interact
	prf_context_def		"ent:links   ",ent_links
	prf_context_def		"ent:draw    ",ent_draw
	
prf_context_num			set	prf_next_

*-------------------------------------------------------*

	data
	even
	text
	even
	
*-------------------------------------------------------*
	