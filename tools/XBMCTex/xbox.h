// Some XBox specific stuff

#pragma once

#ifdef _LINUX
#include "PlatformDefs.h"
typedef struct tagPALETTEENTRY { 
    BYTE peRed; 
    BYTE peGreen; 
    BYTE peBlue; 
    BYTE peFlags; 
} PALETTEENTRY; 
#else
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int BOOL;
typedef const void* LPCVOID;
typedef void* LPVOID;
typedef unsigned char BYTE;
#endif 

struct D3DTexture 
{
	DWORD Common;
	DWORD Data;
	DWORD Lock;

	DWORD Format;   // Format information about the texture.
	DWORD Size;     // Size of a non power-of-2 texture, must be zero otherwise
};

struct D3DPalette
{
	DWORD Common;
	DWORD Data;
	DWORD Lock;

};

typedef enum _XB_D3DFORMAT
{
	XB_D3DFMT_UNKNOWN              = 0xFFFFFFFF,

	/* Swizzled formats */

	XB_D3DFMT_A8R8G8B8             = 0x00000006,
	XB_D3DFMT_X8R8G8B8             = 0x00000007,
	XB_D3DFMT_R5G6B5               = 0x00000005,
	XB_D3DFMT_R6G5B5               = 0x00000027,
	XB_D3DFMT_X1R5G5B5             = 0x00000003,
	XB_D3DFMT_A1R5G5B5             = 0x00000002,
	XB_D3DFMT_A4R4G4B4             = 0x00000004,
	XB_D3DFMT_A8                   = 0x00000019,
	XB_D3DFMT_A8B8G8R8             = 0x0000003A,
	XB_D3DFMT_B8G8R8A8             = 0x0000003B,
	XB_D3DFMT_R4G4B4A4             = 0x00000039,
	XB_D3DFMT_R5G5B5A1             = 0x00000038,
	XB_D3DFMT_R8G8B8A8             = 0x0000003C,
	XB_D3DFMT_R8B8                 = 0x00000029,
	XB_D3DFMT_G8B8                 = 0x00000028,

	XB_D3DFMT_P8                   = 0x0000000B,

	XB_D3DFMT_L8                   = 0x00000000,
	XB_D3DFMT_A8L8                 = 0x0000001A,
	XB_D3DFMT_AL8                  = 0x00000001,
	XB_D3DFMT_L16                  = 0x00000032,

	XB_D3DFMT_V8U8                 = 0x00000028,
	XB_D3DFMT_L6V5U5               = 0x00000027,
	XB_D3DFMT_X8L8V8U8             = 0x00000007,
	XB_D3DFMT_Q8W8V8U8             = 0x0000003A,
	XB_D3DFMT_V16U16               = 0x00000033,

	XB_D3DFMT_D16_LOCKABLE         = 0x0000002C,
	XB_D3DFMT_D16                  = 0x0000002C,
	XB_D3DFMT_D24S8                = 0x0000002A,
	XB_D3DFMT_F16                  = 0x0000002D,
	XB_D3DFMT_F24S8                = 0x0000002B,

	/* YUV formats */

	XB_D3DFMT_YUY2                 = 0x00000024,
	XB_D3DFMT_UYVY                 = 0x00000025,

	/* Compressed formats */

	XB_D3DFMT_DXT1                 = 0x0000000C,
	XB_D3DFMT_DXT2                 = 0x0000000E,
	XB_D3DFMT_DXT3                 = 0x0000000E,
	XB_D3DFMT_DXT4                 = 0x0000000F,
	XB_D3DFMT_DXT5                 = 0x0000000F,

	/* Linear formats */

	XB_D3DFMT_LIN_A1R5G5B5         = 0x00000010,
	XB_D3DFMT_LIN_A4R4G4B4         = 0x0000001D,
	XB_D3DFMT_LIN_A8               = 0x0000001F,
	XB_D3DFMT_LIN_A8B8G8R8         = 0x0000003F,
	XB_D3DFMT_LIN_A8R8G8B8         = 0x00000012,
	XB_D3DFMT_LIN_B8G8R8A8         = 0x00000040,
	XB_D3DFMT_LIN_G8B8             = 0x00000017,
	XB_D3DFMT_LIN_R4G4B4A4         = 0x0000003E,
	XB_D3DFMT_LIN_R5G5B5A1         = 0x0000003D,
	XB_D3DFMT_LIN_R5G6B5           = 0x00000011,
	XB_D3DFMT_LIN_R6G5B5           = 0x00000037,
	XB_D3DFMT_LIN_R8B8             = 0x00000016,
	XB_D3DFMT_LIN_R8G8B8A8         = 0x00000041,
	XB_D3DFMT_LIN_X1R5G5B5         = 0x0000001C,
	XB_D3DFMT_LIN_X8R8G8B8         = 0x0000001E,

	XB_D3DFMT_LIN_A8L8             = 0x00000020,
	XB_D3DFMT_LIN_AL8              = 0x0000001B,
	XB_D3DFMT_LIN_L16              = 0x00000035,
	XB_D3DFMT_LIN_L8               = 0x00000013,

	XB_D3DFMT_LIN_V16U16           = 0x00000036,
	XB_D3DFMT_LIN_V8U8             = 0x00000017,
	XB_D3DFMT_LIN_L6V5U5           = 0x00000037,
	XB_D3DFMT_LIN_X8L8V8U8         = 0x0000001E,
	XB_D3DFMT_LIN_Q8W8V8U8         = 0x00000012,

	XB_D3DFMT_LIN_D24S8            = 0x0000002E,
	XB_D3DFMT_LIN_F24S8            = 0x0000002F,
	XB_D3DFMT_LIN_D16              = 0x00000030,
	XB_D3DFMT_LIN_F16              = 0x00000031,

	XB_D3DFMT_VERTEXDATA           = 100,
	XB_D3DFMT_INDEX16              = 101,

	XB_D3DFMT_FORCE_DWORD          =0x7fffffff
} XB_D3DFORMAT;

#define D3DCOMMON_TYPE_MASK 0x0070000
#define D3DCOMMON_TYPE_TEXTURE 0x0040000

// XPR header
struct XPR_FILE_HEADER
{
  DWORD dwMagic;
  DWORD dwTotalSize;
  DWORD dwHeaderSize;
};

#define XPR_MAGIC_HEADER_VALUE (('0' << 24) | ('R' << 16) | ('P' << 8) | 'X')

DWORD BytesPerPixelFromFormat(XB_D3DFORMAT format);
BOOL IsSwizzledFormat(XB_D3DFORMAT format);
void SetTextureHeader(UINT Width, UINT Height, UINT Levels, UINT Usage, XB_D3DFORMAT Format, D3DTexture *pTexture, UINT Data, UINT Pitch);
void SwizzleRect(LPCVOID pSource, DWORD Pitch, LPVOID pDest, DWORD Width, DWORD Height, DWORD BytesPerPixel);

DWORD inline PadPow2(DWORD x) 
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}
