//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
// 	compression interface
//----------------------------------------------------------------------------------------------------------------------

#ifndef compress_h_
#define compress_h_

//----------------------------------------------------------------------------------------------------------------------

// low level decompression, direct access
extern "C" int DePack2e_68k(u8 *src, u8 *dst);

//----------------------------------------------------------------------------------------------------------------------

// compressed file loader/unwrapper: load content, returning new buffer and size 
bool load_p2e(const char *const _name, u8 **_ppplace, int *_psize);

// compressed file loader/unwrapper: decompress known content/overlay to specified [place]
bool place_p2e(const char *const _name, u8 *_pplace);

// unwrap p2e asset to memory buffer
// file must have either p2e wrapping header or 32bit allocation size prefix to allocate the correct amount of pace
u8* unwrap_p2e_asset
(
	const char *const _name,	// filename
	bool _load_raw,				// load raw version anyway, if not p2e wrapped (otherwise return -1 so caller can attempt load)
	bool _raw_size_prefix,		// raw version has size prefix, avoids need to derive filesize by other means
	u32 *_psize = 0
);

static const u32 c_FNVOBase = 2166136261;


static inline u32 mulu_32_32(u32 _a, u32 _b)
{
//	P = A0*B0
//	Q = (A0*B1) << 16
//	R = (A1*B0) << 16

	u32 c,d,e;

	__asm__
	(
"										\
		move.w %0,%2;					\
		move.w %0,%4;					\
										\
		mulu.w %1,%2;					\
										\
		swap   %0;						\
		mulu.w %1,%0;					\
										\
		swap   %1;						\
		mulu.w %4,%1;					\
										\
		add.l  %1,%0;					\
		swap   %0;						\
		clr.w  %0;						\
										\
		add.l  %2,%0;					\
"
		: "+d"(_a), "+d"(_b), "+d"(c), "+d"(d), "+d"(e)
	    : 
		: "cc"
	);

	return _a;
}

// generate 32bit CRC (FNV hash) based on 16 bit words
static u32 gen_crc(u16 *_data, u32 _words, u32 hash = c_FNVOBase)
{
	// perform hash
	// note: 32bit CRC position (and anything before it) will not be included
	u32 CRC = 0;
	{
		static const u32 c_FNVPrime = 16777619;

		size_t read = 0;
		do
		{
			u16 data = *_data++;

			hash ^= data;

			// todo: can bake constant version for better code
			hash = mulu_32_32(hash, c_FNVPrime);

			//hash *= c_FNVPrime; // todo: use 68k mulu

		} while ((--_words) != 0);

		CRC = hash;
	}

	return CRC;
}

// ---------------------------------------------------------------------------------------------------------------------

#endif // compress_h_
