//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	Shifter display interface: STE specific
//----------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------
//	system headers

// ---------------------------------------------------------------------------------------------------------------------
//	Our AGT headers


#include "common_cpp.h"

// ---------------------------------------------------------------------------------------------------------------------

#include "shifter.h"

// ---------------------------------------------------------------------------------------------------------------------

extern "C" void STE_ConfigureDisplayService();
extern "C" void STE_InstallDisplayService();
extern "C" void STE_RemoveDisplayService();


// ---------------------------------------------------------------------------------------------------------------------

void shifter_ste::configure()
{
	scrollmask_ = 0xf;

	STE_ConfigureDisplayService();
}

void shifter_ste::init()
{	
	init_common();

	STE_InstallDisplayService();
}

void shifter_ste::restore()
{
	restore_common();

	STE_RemoveDisplayService();
}

// ---------------------------------------------------------------------------------------------------------------------

void shifter_ste::setcolour(s16 _buffer, s16 _field, s16 _ci, u16 _c)
{
	volatile shiftgroup& g = g_groups[_buffer];
	volatile shiftfield& s = g.shiftfields_[_field];	

	s.palette_[_ci] = _c;	
}

void shifter_ste::setpalette(s16 _buffer, s16 _field, u16* _colours)
{
	volatile shiftgroup& g = g_groups[_buffer];
	volatile shiftfield& s = g.shiftfields_[_field];	

	for (s16 c = 0; c < 16; ++c)
	{
		u16 col = _colours[c];	

		s.palette_[c] = col;	
	}
}

// ---------------------------------------------------------------------------------------------------------------------
