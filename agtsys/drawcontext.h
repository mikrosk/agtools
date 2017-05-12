//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
// drawcontext holds copy of all relevant fields for the active workbuffer, to permit drawing
// this structure is passed to drawing functions to minimise argument exchanges over the drawing APIs
//----------------------------------------------------------------------------------------------------------------------

#ifndef drawcontext_h_
#define drawcontext_h_

// ---------------------------------------------------------------------------------------------------------------------

typedef struct restorestate_s
{
	s32 *pstack;
	s32 *ptide;

} restorestate_t;

typedef struct drawcontext_s
{
	// playfield buffers, arrays
	u16 *p_pfframebuffer;
	s32 *p_pflineidx;
	s16	*p_pflinemod;

	// playfield info fields
	s32 pfvpagebytes;
	s16 pfvpageh;
	s16 pfviewy;
	s16 pfsnapadjx;
	s16 pfsnapadjy;
	s16 vpagex;
	s16 vpagey;

	restorestate_t *prestorestate;

	//void *pspriteclear_stack_prv;
	//void *pspriteclear_stack_cur;
	//void *pspriteclear_stack_tide;
	//void *pspriteclear_stack_prvtide;

	// vpage-adjusted clipping window
	s16 clip_window_x1;
	s16 clip_window_y1;
	s16 clip_window_x2;
	s16 clip_window_y2;

} drawcontext_t;

extern "C" drawcontext_t drawcontext;

//----------------------------------------------------------------------------------------------------------------------

#endif // drawcontext_h_

