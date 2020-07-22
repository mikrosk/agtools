*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Asset structures
*-------------------------------------------------------*

*-------------------------------------------------------*
	.text
*-------------------------------------------------------*

*-------------------------------------------------------*
*	pair
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
lpair_first:		ds.l	1
lpair_second:		ds.l	1

*-------------------------------------------------------*
*	clipping window
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
clip_x1:		ds.w	1
clip_y1:		ds.w	1
clip_x2:		ds.w	1
clip_y2:		ds.w	1
clip_x1p:		ds.w	1
clip_x2p:		ds.w	1

*-------------------------------------------------------*
*	spritesheet .sps main header
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
sps_version:		ds.w	1
sps_framecount:		ds.w	1
sps_planes:		ds.w	1
sps_flags:		ds.w	1
sps_preshift_range:	ds.b	1
sps_preshift_step:	ds.b	1
sps_srcwidth:		ds.w	1
sps_srcheight:		ds.w	1
sps_frameindex:		ds.l	0
*-------------------------------------------------------*
sps_size:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	spritesheet .sps sprite frame header
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
spf_w:			ds.w	1
spf_h:			ds.w	1
spf_xo:			ds.w	1
spf_yo:			ds.w	1
spf_omap:		ds.w	1
spf_data:		ds.l	0
*-------------------------------------------------------*
			.abs	spf_data
;spf_em_psocclusion:	ds.w	16
spf_em_colour:		ds.l	2
spf_em_psmask:		ds.l	16
			.abs
spf_psm_wordwidth:	ds.w	1
spf_psm_bskew:		ds.w	1
spf_psm_bsyi:		ds.w	1
spf_psm_bdyi:		ds.w	1
spf_psm_wordoffset:	ds.w	1
spf_psm_nextcmp:	ds.l	1
spf_psm_data:

*-------------------------------------------------------*
			.abs	spf_data
;spf_emx_psocclusion:	ds.w	16
spf_emx_colour:		ds.l	2
spf_emx_colouropt:	ds.l	2
spf_emx_psmask:		ds.l	16
spf_emx_psmaskopt:	ds.l	16
			.abs
spf_psmo_wordwidth:	ds.w	1
spf_psmo_wordoffset:	ds.w	1
spf_psmo_dstyskip:	ds.w	1
spf_psmo_dstwords:	ds.w	1
spf_psmo_nextcmp:	ds.l	1
spf_psmo_codegen:

*-------------------------------------------------------*
*	slabsheet .sls main header
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
sls_version:		ds.w	1
sls_framecount:		ds.w	1
sls_planes:		ds.w	1
sls_flags:		ds.w	1
sls_preshift_range:	ds.b	1
sls_preshift_step:	ds.b	1
sls_srcwidth:		ds.w	1
sls_srcheight:		ds.w	1
sls_frameindex:
*-------------------------------------------------------*
sls_size:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	slabsheet .sls frame header
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
slf_w:			ds.w	1
slf_h:			ds.w	1
slf_xo:			ds.w	1
slf_yo:			ds.w	1
slf_spanblocks:		ds.w	1
slf_nextcmp:		ds.l	1
slf_indexbase:
*-------------------------------------------------------*
slf_size:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	slabsheet .sls spanblock header
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
ssb_dy:			ds.w	1
ssb_xo:			ds.w	1
ssb_xs:			ds.w	1
ssb_dxw:		ds.w	1
ssb_y:			ds.w	1
ssb_ys:			ds.w	1
ssb_data:		ds.l	1
*-------------------------------------------------------*
ssb_size:
*-------------------------------------------------------*

*-------------------------------------------------------*
*	slabsheet .sls clipper header
*-------------------------------------------------------*
			.abs
*-------------------------------------------------------*
slc_tyc_offset:		ds.w	1
slc_tyc_remain:		ds.w	1
slc_tyc_data:		ds.w	1
slc_byc_remain:		ds.w	1
*-------------------------------------------------------*
slc_size:
*-------------------------------------------------------*

*-------------------------------------------------------*
	.text
*-------------------------------------------------------*
