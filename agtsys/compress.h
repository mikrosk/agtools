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

// ---------------------------------------------------------------------------------------------------------------------

#endif // compress_h_
