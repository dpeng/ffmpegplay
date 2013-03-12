#ifndef COLORSPACE_H
#define COLORSPACE_H

typedef void (*color_convert_func)( \
	unsigned char *py, unsigned char *pu, unsigned char *pv, \
	unsigned char *dst,\
	unsigned long pitch, \
	int width,int height);

extern color_convert_func ccfunc[4];

void YUV_TO_RGB24(unsigned char *puc_y, 
	unsigned char *puc_u, unsigned char *puc_v,int stride_y, 
	unsigned char *puc_out, int width_y, int height_y,int stride_out);

void YUV_TO_RGB32(unsigned char *puc_y, 
	unsigned char *puc_u, unsigned char *puc_v,int stride_y, 
	unsigned char *puc_out, int width_y, int height_y,int stride_out);


void YUV_TO_RGB24_C(
	unsigned char *py, unsigned char *pu, unsigned char *pv, 
	int width, int height,unsigned char *out, unsigned outsize);

#endif // COLORSPACE_H
