//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
//	playfield region update implementation
//----------------------------------------------------------------------------------------------------------------------

#include "pf_specr16.inl"
#include "pf_specr8.inl"

//----------------------------------------------------------------------------------------------------------------------

inline void PQRS_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
	if (c_tilestep_ == 4)
		PQRS16_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
	//else
	//	PQRS8_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
}

inline void PQ_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
	if (c_tilestep_ == 4)
		PQ16_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
	//else
	//	PQ8_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
}

inline void PR_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
	if (c_tilestep_ == 4)
		PR16_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
	else
		PR8_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
}

inline void P_fill_specrow_direct(s16 _mi, s16 _mj, s16 pi, s16 pj, s16 pi2, s16 pjy1, s16 pjy2)
{
	if (c_tilestep_ == 4)
		P16_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
	//else
	//	P8_fill_specrow_direct(_mi, _mj, pi, pj, pi2, pjy1, pjy2);
}
