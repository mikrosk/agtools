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
	bool _raw_size_prefix		// raw version has size prefix, avoids need to derive filesize by other means
);

// ---------------------------------------------------------------------------------------------------------------------

#endif // compress_h_
