//======================================================================================================================
// 	[A]tari [G]ame [T]ools / dml 2022
//======================================================================================================================
//	planes-to-nibbles conversion
//----------------------------------------------------------------------------------------------------------------------

#ifndef tools_endian_h_
#define tools_endian_h_

#if !defined(tools_common_h_)
#error "requires include: common/common.h"
#endif

//----------------------------------------------------------------------------------------------------------------------

#if defined(__linux__)
#include <stdint.h>		// for uintxx_t
#include <arpa/inet.h>	// for htonl/ntohl
#endif

// grrr!!
#if defined(__MINGW32__)
__stdcall uint32_t htonl(uint32_t hostlong)
{ 
	uint32_t v = hostlong;

	uint32_t vs = 
		((v & 0x000000FF) << 24) | 
		((v & 0x0000FF00) << 8) | 
		((v & 0x00FF0000) >> 8) | 
		((v & 0xFF000000) >> 24)
	;

	return vs;
}
__stdcall uint32_t ntohl(uint32_t netlong)
{ 
	uint32_t v = netlong;

	uint32_t vs = 
		((v & 0x000000FF) << 24) | 
		((v & 0x0000FF00) << 8) | 
		((v & 0x00FF0000) >> 8) | 
		((v & 0xFF000000) >> 24)
	;

	return vs;
}
__stdcall uint16_t htons(uint16_t hostword)
{ 
	uint16_t v = hostword;

	uint16_t vs = 
		((v & 0x00FF) << 8) | 
		((v & 0xFF00) >> 8)
	;

	return vs;
}
__stdcall uint16_t ntohs(uint16_t netword)
{ 
	uint16_t v = netword;

	uint16_t vs = 
		((v & 0x00FF) << 8) | 
		((v & 0xFF00) >> 8)
	;

	return vs;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
#endif // tools_endian_h_
//----------------------------------------------------------------------------------------------------------------------
