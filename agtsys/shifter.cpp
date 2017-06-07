//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	Shifter display interface: generic
//----------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//	system headers

#include <mint/sysbind.h>

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers

#include "common_cpp.h"

// ---------------------------------------------------------------------------------------------------------------------

#include "shifter.h"

// ---------------------------------------------------------------------------------------------------------------------

void shifter_common::save()
{
	save_physbase = Physbase();
//	save_logbase = Logbase();
//	save_res = Getrez();
	
	get_hwpalette(save_palette_);
}

void shifter_common::init_common()
{	
	for (int i = 0; i < c_max_shiftgroups; i++)
	{
		g_pgroups[i] = &g_groups[i];
		g_groups[i].pshiftfields_[0] = &g_groups[i].shiftfields_[0];
		g_groups[i].pshiftfields_[1] = &g_groups[i].shiftfields_[1];
	}

	s_shifter = this;
}

void shifter_common::restore_common()
{
	// todo: should save/restore regs instead of using this API - lazy today
	for (int f = 0; f < c_max_shiftfields; f++)
	{
		setdisplay(f, (u32)save_physbase, 160, 160, 0, 0); //update();	
		//setdisplay(1, (u32)save_physbase, 160, 160, 0, 0); //update();
	}
	advance();
	
	// VSync
	s16 field_index = reg16(464);
	while (reg16(464) == field_index) { }
				
	set_hwpalette(save_palette_);

//	Setscreen(save_logbase, save_physbase, save_res);
}

// ---------------------------------------------------------------------------------------------------------------------

// global shifter instance for VBL callback
shifter_common* shifter_common::s_shifter = 0;

extern "C" 
{
	volatile shiftgroup g_groups[c_max_shiftgroups];
	volatile shiftgroup *g_pgroups[c_max_shiftgroups]; // *g_pgroups[] = { &g_groups[0], &g_groups[1] };

	volatile s16 g_active_buffer = -1;    
	volatile shiftgroup* volatile gp_active_group = 0;
	volatile s16 g_vbl = 0;
	volatile s16 g_fieldmask = 0;
	s16 g_frame = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
