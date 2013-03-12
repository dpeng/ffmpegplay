#ifndef BMPWRITER_H
#define BMPWRITER_H

#include <windows.h>
#include <stdio.h>

class BmpWriter
{
public:
	static BOOL execute(
		const char *fname, int type,
		unsigned char *py,unsigned char *pu,unsigned char *pv,
		int width,int height);
private:

	static BOOL SaveToJpegFile(
		FILE * fp, 
		unsigned char *rgb, int rgbSize, 
		int width, int height);

	static BOOL SaveToBmpFile(
		FILE * fp, 
		unsigned char *rgb, int rgbSize, 
		int width, int height);

	static BOOL VertFlipBuf(
		BYTE  *inbuf, unsigned int widthBytes, 
		unsigned int height);

	static BOOL BGRFromRGB(BYTE *buf, unsigned int widthPix, unsigned int height);

	static void YResizeCubic(unsigned char*, unsigned char*, int, int, int, int);
	static void UVResize(unsigned char *, unsigned char *, int, int, int, int);
};

#endif /* BMPWRITER_H */
