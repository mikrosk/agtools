//======================================================================================================================
//	[A]tari [G]ame [T]ools / dml 2016
//======================================================================================================================
// 	compression interface
//----------------------------------------------------------------------------------------------------------------------

#ifndef compress_h_
#define compress_h_

//----------------------------------------------------------------------------------------------------------------------

typedef struct wrapheader_s
{
	uint32_t prefix;			/* \ full header excluded from file_size */
	uint32_t wrapcode;			/* / allows wrapped assets to be identified */
	uint32_t data_size;			/* \ with fallback to raw loading */
	uint32_t unpacked_size;
	uint32_t pcrc;
	uint32_t ucrc;

} wrapheader_t;

enum AssetFlagsIdx
{
	afi_load_unwrapped,
	afi_asset_scratch,
	afi_size_prefix,
	afi_size_lendian,
	afi_size_exclusive
};

#define ASSETFLAG(_F_) af_##_F_ = (1 << afi_##_F_)

enum AssetFlags
{
	af_none = 0,
	//
	ASSETFLAG(load_unwrapped),	// expect asset may not be wrapped & load anyway (otherwise return -1 so caller can attempt load)
	ASSETFLAG(asset_scratch),	// may use scratch for the allocated asset (consumed/discarded immediately by user)
	ASSETFLAG(size_prefix),		// expect unwrapped assets to have a size prefix word for space allocation
	ASSETFLAG(size_lendian),	// expect unwrapped assets with size prefix to have little-endian prefix word
	ASSETFLAG(size_exclusive)	// expect unwrapped assets with size prefix to exclude the prefix word itself from recorded size
};

//----------------------------------------------------------------------------------------------------------------------

// low level decompression, direct access
//
// lz77
extern "C" int DeLz77_68k(u8 *src, u8 *dst);
// pack2e/nrv2e
extern "C" int DePack2e_68k(u8 *src, u8 *dst);
// arj4
extern "C" int DeArj4_68k(u8 *src, u8 *dst);
// arj7
extern "C" int DeArj7_68k(u8 *src, u8 *dst);

//----------------------------------------------------------------------------------------------------------------------

// compressed file loader/unwrapper: load content, returning new buffer and size 
//bool load_asset(const char *const _name, AssetFlags _flags, u8 **_ppplace, int *_psize);

// compressed file loader/unwrapper: decompress known content/overlay to specified [place]
bool place_asset
(
	const char *const _name, 
	AssetFlags _flags, 
	u8 *_place, 
	u32 *_psize = 0, 
	u32 _smax = 0
);

u8 * load_asset
(
	const char *const _name, 
	AssetFlags _flags,
	u32 *_psize = 0, 
	u32 _smax = 0
);


// unwrap packed asset to memory buffer
// - if wrapping header is present, will be used to allocate space & unpack as required
// - if header not present, _raw_size_prefix hint can read a size prefix word to allocate space for reading
// - otherwise filesize will be tested with GEMDOS
/*u8* unwrap_asset
(
	const char *const _name,	// filename
	AssetFlags _flags,
	u32 *_psize = 0				// unpacked size (or raw size)
);
*/

static const u32 c_FNVOBase = 2166136261UL;

u32 gen_crc_lite(u16 *_data, u32 _words);

// generate 32bit CRC (FNV hash) based on 16 bit words
// todo: 68k version of this needed
u32 gen_crc(u16 *_data, u32 _words, u32 hash = c_FNVOBase);

// ---------------------------------------------------------------------------------------------------------------------

#endif // compress_h_
