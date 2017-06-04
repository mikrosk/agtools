
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

	; sprite clipping window
;	lea		_g_spr_window,a2
	
	sub.w		_g_xpreshift,d1
	
	; constants
	moveq		#16-1,d6
	moveq		#-16,d7
		
	jsr		STF_ISSprOpt_impl_
