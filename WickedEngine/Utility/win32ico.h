#ifndef WIN32_ICO_H
#define WIN32_ICO_H

// ICO image format is for storing multiple images in a single file with max 256 dimension, to be used with icons and cursors
//	Structure:
//		ICONDIR
//		ICONDIRENTRY * imageCount
//		(BITMAPINFOHEADER, BGRA pixel data, opacity bitmask) * imageCount

namespace ico
{
	struct ICONDIR
	{
		unsigned short idReserved;		// Reserved (must be 0)
		unsigned short idType;			// Resource Type (1 for icon, 2 for cursor)
		unsigned short idCount;			// Number of images
	};
	struct ICONDIRENTRY
	{
		unsigned char  bWidth;			// Width, in pixels
		unsigned char  bHeight;			// Height, in pixels
		unsigned char  bColorCount;		// Number of colors (0 if >= 8bpp)
		unsigned char  bReserved;		// Reserved (must be 0)
		unsigned short wPlanes;			// Color Planes	 | hotspotX for cursor
		unsigned short wBitCount;		// Bits per pixel | hotspotY for cursor
		unsigned int dwBytesInRes;		// Size of image data
		unsigned int dwImageOffset;		// Offset to image data
	};
	struct BITMAPINFOHEADER
	{
		unsigned int biSize;			// Size of this header
		int  biWidth;					// Width in pixels
		int  biHeight;					// Height in pixels (doubled for icon)
		unsigned short biPlanes;		// Number of color planes
		unsigned short biBitCount;		// Bits per pixel
		unsigned int biCompression;		// Compression method
		unsigned int biSizeImage;		// Size of image data
		int  biXPelsPerMeter;			// Horizontal resolution
		int  biYPelsPerMeter;			// Vertical resolution
		unsigned int biClrUsed;			// Colors used
		unsigned int biClrImportant;	// Important colors
	};
}

#endif // WIN32_ICO_H
