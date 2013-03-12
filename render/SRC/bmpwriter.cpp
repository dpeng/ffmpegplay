#include "bmpwriter.h"

#include <stdio.h>

#include "colorspace.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
	
#include "jpeglib.h"

struct my_error_mgr {
struct jpeg_error_mgr pub;	/* "public" fields */
};
	
typedef struct my_error_mgr * my_error_ptr;

void my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	
	char buffer[JMSG_LENGTH_MAX];
	
	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#define	CBITS	3

BOOL BmpWriter::SaveToJpegFile(FILE * fp, unsigned char *rgb, int rgbSize, int width, int height)
{
	struct jpeg_compress_struct cinfo;
	
	int row_stride;
	
	struct my_error_mgr jerr;
	
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	
	jpeg_create_compress(&cinfo);
	
	jpeg_stdio_dest(&cinfo, fp);
	
	cinfo.image_width = width;
	cinfo.image_height = height;
	
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	
	jpeg_set_defaults(&cinfo);
	
	jpeg_set_quality(&cinfo, 75, TRUE /* limit to baseline-JPEG values */);
	
	jpeg_start_compress(&cinfo, TRUE);
	
	row_stride = width * 3;
	
	unsigned char *dataBuf = rgb;
	
	VertFlipBuf(dataBuf, width*3, height);
	BGRFromRGB(dataBuf, width, height);
	
	while (cinfo.next_scanline < cinfo.image_height) {
		LPBYTE outRow;
		outRow = dataBuf + (cinfo.next_scanline * width* 3);
		(void) jpeg_write_scanlines(&cinfo, &outRow, 1);
	}
	
	jpeg_finish_compress(&cinfo);
	
	jpeg_destroy_compress(&cinfo);

	return TRUE;
}

BOOL BmpWriter::SaveToBmpFile(FILE * fp, unsigned char *rgb, int rgbSize, int width, int height)
{
	BITMAPFILEHEADER bmpHeader;
	BITMAPINFO bmpInfo;
	
	bmpHeader.bfType='MB';
	bmpHeader.bfSize=rgbSize+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	bmpHeader.bfReserved1=0;
	bmpHeader.bfReserved2=0;
	bmpHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	
	bmpInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth=width;
	bmpInfo.bmiHeader.biHeight=height;
	bmpInfo.bmiHeader.biPlanes=1;
	bmpInfo.bmiHeader.biBitCount=24;
	bmpInfo.bmiHeader.biCompression=BI_RGB;
	bmpInfo.bmiHeader.biSizeImage=rgbSize;
	
	bmpInfo.bmiHeader.biXPelsPerMeter=0;
	bmpInfo.bmiHeader.biYPelsPerMeter=0;
	bmpInfo.bmiHeader.biClrUsed=0;
	bmpInfo.bmiHeader.biClrImportant=0;
		
	unsigned long count=0;
	
	count = fwrite(&bmpHeader,1,sizeof(BITMAPFILEHEADER),fp);
	
	if (count!=sizeof(BITMAPFILEHEADER)) {
		return FALSE;
	} else {
		count = fwrite(&(bmpInfo.bmiHeader),1,sizeof(BITMAPINFOHEADER),fp);
		if (count!= sizeof(BITMAPINFOHEADER)){
			return FALSE;
		}else{
			count=fwrite(rgb,1,rgbSize,fp);
			if (count!= rgbSize) {
				return FALSE;
			}
		}
	}

	return TRUE;
}

BOOL BmpWriter::execute(const char *fname, int type,unsigned char *py,unsigned char *pu,unsigned char *pv,int width, int height)
{
	unsigned char *pY, *pU, *pV, *pUbase, *pVbase;

	unsigned char *vgaBuffer = 0;

	int	i, j;
	int w, h;
	
	w = width;
	h = height;

	pY = py;
	pU = pUbase	= pu;
	pV = pVbase	= pv;
	
	bool vx2 = false;
	bool hx2 = false;

	unsigned char *rgbbuf1, *rgbbuf2;

	if (w==640&&h==480) {
		vgaBuffer = (unsigned char *)new char[704*576*3/2];
		if (!vgaBuffer)
			return FALSE;

		unsigned char *d1y = vgaBuffer;
		unsigned char *d1u = d1y + 704*576;
		unsigned char *d1v = d1u + 704*576/4;

		YResizeCubic(py, d1y, h, w, 576, 704);
		UVResize(pu, d1u, h/2, w/2, 288, 352);
		UVResize(pv, d1v, h/2, w/2, 288, 352);

		w = 704;
		h = 576;
		
		pY = d1y;
		pU = pUbase	= d1u;
		pV = pVbase	= d1v;

	}

	// vx2和hx2主要是针对HalfD1或者别的非正常大小图片设置.
	if ((w>=640&&w<=720)&&(h<=288&&h>=240)) vx2 = true;
	if ((w<=352&&w>=320) &&	(h<=576&&h>=480)) hx2 = true;

	unsigned long rgbSize =	w*h*3;

	rgbbuf1 = (unsigned char *)new char[rgbSize];
	if (!rgbbuf1) return FALSE;

	rgbbuf2 = (unsigned char *)new char[rgbSize*2];
	if (!rgbbuf2) {
		delete rgbbuf1;
		return FALSE;
	}
	
	YUV_TO_RGB24(pY,pU,pV,w,(unsigned char*)&rgbbuf1,w,h,rgbSize);

	if(vx2){
		for	(i=0;i<h;i++) {
			memcpy(rgbbuf2+i*2*w*3,rgbbuf1+i*w*3,w*3);
			memcpy(rgbbuf2+(i*2+1)*w*3,rgbbuf1+i*w*3,w*3);
		}
	}

	if(hx2)	{
		for	(i=0;i<h;i++) {
			for(j=0;j<w;j++) {
				*(rgbbuf2+i*2*w*3+0+j*6)=*(rgbbuf1+i*w*3+0+j*3);
				*(rgbbuf2+i*2*w*3+1+j*6)=*(rgbbuf1+i*w*3+1+j*3);
				*(rgbbuf2+i*2*w*3+2+j*6)=*(rgbbuf1+i*w*3+2+j*3);

				*(rgbbuf2+i*2*w*3+3+j*6)=*(rgbbuf1+i*w*3+0+j*3);
				*(rgbbuf2+i*2*w*3+4+j*6)=*(rgbbuf1+i*w*3+1+j*3);
				*(rgbbuf2+i*2*w*3+5+j*6)=*(rgbbuf1+i*w*3+2+j*3);
			}
		}
	}

	FILE *fp;
	
	if ((fp	= fopen(fname, "wb")) <= 0 ) {
		if (rgbbuf1) delete	rgbbuf1;
		if (rgbbuf2) delete	rgbbuf2;
		return FALSE;
	}

	if (type==0) {
		if(vx2||hx2) {
			rgbSize*=2;
			if (vx2) h*=2;
			if (hx2) w*=2;
			SaveToBmpFile(fp,rgbbuf2,rgbSize,w,h);
		} else {
			SaveToBmpFile(fp,rgbbuf1,rgbSize,w,h);
		}
	} else if (type==1){ // JPEG
		if(vx2||hx2) {
			rgbSize*=2;
			if (vx2) h*=2;
			if (hx2) w*=2;
			SaveToJpegFile(fp,rgbbuf2,rgbSize,w,h);
		} else {
			SaveToJpegFile(fp,rgbbuf1,rgbSize,w,h);
		}
	}

	fclose(fp);

	if (rgbbuf1) delete	rgbbuf1;
	if (rgbbuf2) delete	rgbbuf2;
	if (vgaBuffer) delete vgaBuffer;

	return TRUE;
}

inline BOOL BmpWriter::BGRFromRGB(BYTE *buf, UINT widthPix, UINT height)
{
	if (buf==NULL)
		return FALSE;
	
	UINT col, row;
	for (row=0;row<height;row++) {
		for (col=0;col<widthPix;col++) {
			LPBYTE pRed, pGrn, pBlu;
			pRed = buf + row * widthPix * 3 + col * 3;
			pGrn = buf + row * widthPix * 3 + col * 3 + 1;
			pBlu = buf + row * widthPix * 3 + col * 3 + 2;
			
			// swap red and blue
			BYTE tmp;
			tmp = *pRed;
			*pRed = *pBlu;
			*pBlu = tmp;
		}
	}
	
	return TRUE;
}

inline BOOL BmpWriter::VertFlipBuf(BYTE  * inbuf, 
						   unsigned int widthBytes, 
						   unsigned int height)
{   
	BYTE  *tb1;
	BYTE  *tb2;
	
	if (inbuf==NULL)
		return FALSE;
	
	UINT bufsize;
	
	bufsize=widthBytes;
	
	tb1= (BYTE *)new BYTE[bufsize];
	if (tb1==NULL) {
		return FALSE;
	}
	
	tb2= (BYTE *)new BYTE [bufsize];
	if (tb2==NULL) {
		delete [] tb1;
		return FALSE;
	}
	
	UINT row_cnt;     
	ULONG off1=0;
	ULONG off2=0;
	
	for (row_cnt=0;row_cnt<(height+1)/2;row_cnt++) {
		off1=row_cnt*bufsize;
		off2=((height-1)-row_cnt)*bufsize;   
		
		memcpy(tb1,inbuf+off1,bufsize);
		memcpy(tb2,inbuf+off2,bufsize);	
		memcpy(inbuf+off1,tb2,bufsize);
		memcpy(inbuf+off2,tb1,bufsize);
	}	
	
	delete [] tb1;
	delete [] tb2;
	
	return TRUE;
}        

int params_bic[32][6] = {{0, -3, 256, 4,   -1,  0},  {1, -5,  255, 6,   -1,  0},
{1,  -9, 254, 12,  -2,  0},  {2, -13, 251, 19,  -3,  0},
{2, -17, 247, 27,  -4,  1},  {2, -19, 243, 36,  -6,  0},
{3, -22, 237, 45,  -8,  1},  {3, -24, 231, 54,  -9,  1},
{3, -25, 224, 64,  -11, 1},  {3, -25, 216, 74,  -13, 1},
{3, -27, 208, 86,  -15, 1},  {3, -27, 199, 95,  -16, 2},
{3, -27, 190, 106, -18, 2},  {3, -27, 181, 117, -20, 2},
{3, -26, 170, 128, -21, 2},  {3, -25, 160, 139, -23, 2},
{3, -24, 149, 149, -24, 3},  {2, -23, 139, 160, -25, 3},
{2, -21, 128, 170, -26, 3},  {2, -20, 117, 180, -26, 3},
{2, -18, 106, 190, -27, 3},  {2, -16, 95,  199, -27, 3},
{1, -15, 85,  208, -26, 3},  {1, -13, 75,  216, -26, 3},
{1, -11, 64,  224, -25, 3},  {1, -9,  54,  231, -24, 3},
{1, -8,  45,  237, -22, 3},  {0, -6,  36,  243, -19, 2},
{1, -4,  27,  247, -17, 2},  {0, -3,  19,  251, -13, 2},
{0, -2,  12,  254, -9,  1},  {0, -1,  6,   255, -5,  1}
};

int params_uv[32][2] = {{0,   256}, {8,   248}, {16,  240}, {24,  232}, 
{32,  224},	{40,  216}, {48,  208}, {56,  200}, 
{64,  192}, {72,  184}, {80,  176}, {88,  168}, 
{96,  160}, {104, 152}, {112, 144},	{120, 136}, 
{128, 128}, {136, 120}, {144, 112}, {152, 104}, 
{160,  96}, {168,  88}, {176,  80}, {184,  72}, 
{192,  64}, {200,  56}, {208,  48}, {216,  40},
{224,  32}, {232,  24}, {240,  16}, {248,   8}
};

int params_bil[32][4] ={{40, 176, 40, 0}, {36, 176, 43, 1},
{32, 175, 48, 1}, {30, 174, 51, 1},
{27, 172, 56, 1}, {24, 170, 61, 1},
{22, 167, 66, 1}, {19, 164, 71, 2},
{17, 161, 76, 2}, {15, 157, 82, 2},
{14, 152, 87, 3}, {12, 148, 93, 3},
{11, 143, 99, 3}, {9, 138, 105, 4},
{8, 133, 110, 5}, {7, 128, 116, 5},
{6, 122, 122, 6}, {5, 117, 127, 7},
{5, 110, 133, 8}, {4, 105, 138, 9},
{3, 99, 143, 11}, {3, 93, 148, 12},
{3, 87, 152, 14}, {2, 82, 157, 15},
{2, 76, 161, 17}, {2, 71, 164, 19},
{1, 66, 167, 22}, {1, 61, 170, 24},
{1, 56, 172, 27}, {1, 52, 173, 30},
{1, 47, 175, 33}, {0, 44, 176, 36}
};

void BmpWriter::YResizeCubic(unsigned char* ptr_in, unsigned char* ptr_rz, 
				  int old_rows, int old_cols, int rsz_rows, int rsz_cols)
{
	unsigned char* ptr_temp;
	unsigned char* ptr_line;
	ptr_temp = (unsigned char*)malloc((old_rows + 6) * rsz_cols);
	ptr_line = (unsigned char*)malloc(old_cols + 6);

	int i, j, m, idx, tmp_data;
	int* ptr_flt;
	unsigned long ratio;
	
	ratio = old_cols * 1024 / rsz_cols;

	for(i = 0; i < old_rows; i++)
	{
		memcpy(ptr_line + 3, ptr_in + i * old_cols, old_cols);
		memset(ptr_line, ptr_in[i * old_cols], 3);
		memset(ptr_line + old_cols + 3, ptr_in[(i + 1) * old_cols - 1], 3);
		
		for(j = 0; j < rsz_cols; j++)
		{
			idx     = ((j * ratio) % 1024 * 32) / 1024;
			ptr_flt = params_bic[idx];

			idx      = j * ratio / 1024 + 3;
			tmp_data = 0;
			for(m = 0; m < 6; m++)
				tmp_data += ptr_line[idx + m - 2] * ptr_flt[m];

			tmp_data /= 256;
			if(tmp_data < 0) tmp_data = 0;
			if(tmp_data > 255) tmp_data = 255;
			ptr_temp[(i + 3) * rsz_cols + j] = (unsigned char)tmp_data;
		}
	}

	memcpy(ptr_temp, ptr_temp + 3 * rsz_cols, rsz_cols);
	memcpy(ptr_temp + rsz_cols, ptr_temp + 3 * rsz_cols, rsz_cols);
	memcpy(ptr_temp + rsz_cols * 2, ptr_temp + 3 * rsz_cols, rsz_cols);
	memcpy(ptr_temp + rsz_cols * (old_rows + 3), ptr_temp + (old_rows + 2) * rsz_cols, rsz_cols);
	memcpy(ptr_temp + rsz_cols * (old_rows + 4), ptr_temp + (old_rows + 2) * rsz_cols, rsz_cols);
	memcpy(ptr_temp + rsz_cols * (old_rows + 5), ptr_temp + (old_rows + 2) * rsz_cols, rsz_cols);

	ratio = old_rows * 1024 / rsz_rows;
	for(j = 0; j < rsz_cols; j++)
		for(i = 0; i < rsz_rows; i++)
		{
			idx     = ((i * ratio) % 1024 * 32) / 1024;
			ptr_flt = params_bic[idx];

			idx      = i * ratio / 1024 + 3;
			tmp_data = 0;
			for(m = 0; m < 6; m++)
				tmp_data += ptr_temp[(idx + m - 2) * rsz_cols + j] * ptr_flt[m];
			tmp_data /= 256;
			if(tmp_data < 0) tmp_data = 0;
			if(tmp_data > 255) tmp_data = 255;
			ptr_rz[i * rsz_cols + j] = (unsigned char)tmp_data;
		}

	free(ptr_temp);
	free(ptr_line);
}

////////////////////////////////////////////////////////////////////////////
// U and V component resize using bilinear interpolation
void BmpWriter::UVResize(unsigned char* ptr_in, unsigned char* ptr_rz, 
			  int old_rows, int old_cols, int rsz_rows, int rsz_cols)
{
	unsigned char* ptr_temp;
	unsigned char* ptr_line;
	ptr_temp = (unsigned char*)malloc((old_rows + 2) * rsz_cols);
	ptr_line = (unsigned char*)malloc(old_cols + 2);
	
	int i, j, idx, tmp_data;
	int* ptr_flt;
	unsigned long ratio;
		
	ratio = 1024 * old_cols / rsz_cols;
	for(i = 0; i < old_rows; i++)
	{
		memcpy(ptr_line + 1, ptr_in + i * old_cols, old_cols);
		ptr_line[0] = ptr_line[1];
		ptr_line[old_cols + 1] = ptr_line[old_cols];

		for(j = 0; j < rsz_cols; j++)
		{
			idx     = ((j * ratio) % 1024 * 32) / 1024;
			ptr_flt = params_uv[idx];

			idx      = j * ratio / 1024 + 1;
			tmp_data = (ptr_line[idx] * ptr_flt[0] + ptr_line[idx + 1] * ptr_flt[1]) / 256;
			if(tmp_data > 255) tmp_data = 255;
			ptr_temp[(i + 1) * rsz_cols + j] = (unsigned char)tmp_data;
		}
	}
	memcpy(ptr_temp, ptr_temp + rsz_cols, rsz_cols);
	memcpy(ptr_temp + rsz_cols * (old_rows + 1), ptr_temp + old_rows * rsz_cols, rsz_cols);	

	ratio = 1024 * old_rows / rsz_rows;
	for(j = 0; j < rsz_cols; j++)
		for(i = 0; i < rsz_rows; i++)
		{
			idx     = ((i * ratio) % 1024 * 32) / 1024;	
			ptr_flt = params_uv[idx];

			idx      = i * ratio / 1024 + 1;
			tmp_data = (ptr_temp[idx * rsz_cols + j] * ptr_flt[0] + 
				ptr_temp[(idx + 1) * rsz_cols + j] * ptr_flt[1]) / 256;
			if(tmp_data > 255) tmp_data = 255;
			ptr_rz[i * rsz_cols + j] = (unsigned char)tmp_data;
		}
		
		free(ptr_temp);
		free(ptr_line);
}