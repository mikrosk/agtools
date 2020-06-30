*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	EMSPR blitter sprite implementation
*-------------------------------------------------------*

emsquick_path	set	0

	.if		(emsquick_path)
	include		"ste/b_emg_spr.s"
	.else
	include		"ste/b_emc_spr.s"
	.endif
