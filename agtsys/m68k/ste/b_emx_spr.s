*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
* 	EMCSPR blitter sprite implementation
*-------------------------------------------------------*

emxquick_path	set	0

	.if		(emxquick_path)
	include		"ste/b_emxg_spr.s"
	.else
	include		"ste/b_emxc_spr.s"
	.endif
