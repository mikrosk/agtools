*=======================================================*
*	[A]tari [G]ame [T]ools / dml 2016
*=======================================================*
*	Entity structurea
*-------------------------------------------------------*

*-------------------------------------------------------*
	text
*-------------------------------------------------------*


*-------------------------------------------------------*
			rsreset
*-------------------------------------------------------*
ent_pnext_tick:		rs.l	1
ent_pprev_tick:		rs.l	1
ent_pnext_x:		rs.l	1
ent_pprev_x:		rs.l	1
ent_pnext_y:		rs.l	1
ent_pprev_y:		rs.l	1
ent_pparent:		rs.l	1
ent_layer_:		rs.l	1
ent_id:			rs.w	1
ent_type:		rs.w	1
*-------------------------------------------------------*
ent_fwx:		rs.l	1
ent_fwy:		rs.l	1
ent_fwz:		rs.l	1
ent_fvz:		rs.l	1
*-------------------------------------------------------*
ent_fntick:		rs.l	1
ent_fncollide:		rs.l	1
ent_passet:		rs.l	1
ent_frx:		rs.l	1
ent_fry:		rs.l	1
ent_fvx:		rs.l	1
ent_fvy:		rs.l	1
*-------------------------------------------------------*
ent_sx:			rs.w	1
ent_sy:			rs.w	1
ent_ox:			rs.w	1
ent_oy:			rs.w	1
*-------------------------------------------------------*
ent_tick_priority:	rs.w	1
ent_draw_hitflash:	rs.w	1
ent_health:		rs.w	1
ent_damage:		rs.w	1
*-------------------------------------------------------*
ent_frame:		rs.w	1
ent_counter:		rs.w	1
ent_drawtype:		rs.w	1
ent_drawlayer:		rs.w	1
*-------------------------------------------------------*
ent_self:		rs.w	1
ent_interactswith:	rs.w	1
*-------------------------------------------------------*
ent_header_:		rs.b	0
*-------------------------------------------------------*
			text
*-------------------------------------------------------*

EntityDraw_NONE		=	0
EntityDraw_SPRITE	=	1
EntityDraw_EMSPR	=	2
EntityDraw_EMXSPR	=	3
EntityDraw_SLAB		=	4
EntityDraw_CUSTOM	=	5
EntityDraw_RFILL	=	6
