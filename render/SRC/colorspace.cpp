#include "colorspace.h"

#include <string.h>

inline void yuv2yv12(
	unsigned char *py,
	unsigned char *pu,
	unsigned char *pv,
	unsigned char *dst,
	unsigned long pitch,
	int width,
	int height)
{
    register unsigned char *tmpdst;
	register unsigned char *y, *u, *v;
	
    register int i;
	register int halfh,halfw,halfp;

    y = py;
    u = pu;
    v = pv;
	
    halfw = (width>>1);
    halfh = (height>>1);
    halfp = (pitch>>1);
 
    tmpdst = dst;
    for (i=0;i<height;++i) 
	{
        memcpy(tmpdst, y, width);
		y+=width;
        tmpdst+=pitch;
    }

	tmpdst = dst + height*pitch;
	for(i=0;i<halfh;++i) 
	{
        memcpy(tmpdst,v,halfw);
		v+=halfw;
        tmpdst+=halfp;
    }

	tmpdst = dst + height*pitch*5/4;
	for (i=0;i<halfh;++i) 
	{
        memcpy(tmpdst,u,halfw);
		u+=halfw;
        tmpdst+=halfp;
    }

}

inline void yuv2uyvy16_mmx(
	unsigned char *py,		// [esp+4+16]
	unsigned char *pu,		// [esp+8+16]
	unsigned char *pv,		// [esp+12+16]
	unsigned char *dst,		// [esp+16+16]
	unsigned long pitch,	// [esp+20+16]
	int w,					// [esp+24+16]
	int h)					// [esp+28+16]
{	
	int width = w/2;

	__asm {
		push		ebp
		push		edi
		push		esi
		push		ebx
			
		mov			edx,[width];			;load width (mult of 8)
		mov			ebx,[pu]			;load source U ptr
		mov			ecx,[pv]			;load source V ptr
		mov			eax,[py]			;load source Y ptr
		mov			edi,[dst]			;load destination ptr
		mov			esi,[pitch]			;load destination pitch
		mov			ebp,[h]				;load height
			
		lea			ebx,[ebx+edx]			;bias pointers
		lea			ecx,[ecx+edx]			;(we count from -n to 0)
		lea			eax,[eax+edx*2]
		lea			edi,[edi+edx*4]
			
		neg			edx
		mov			[esp+24+16],edx
xyloop:
		movq		mm0,[ebx+edx]			;U0-U7
			
		movq		mm7,[ecx+edx]			;V0-V7
		movq		mm2,mm0					;U0-U7
			
		movq		mm4,[eax+edx*2]
		punpcklbw	mm0,mm7					;[V3][U3][V2][U2][V1][U1][V0][U0]
			
		movq		mm5,[eax+edx*2+8]
		punpckhbw	mm2,mm7					;[V7][U7][V6][U6][V5][U5][V4][U4]
			
		movq		mm1,mm0
		punpcklbw	mm0,mm4					;[Y3][V1][Y2][U1][Y1][V0][Y0][U0]
			
		punpckhbw	mm1,mm4					;[Y7][V3][Y6][U3][Y5][V2][Y4][U2]
		movq		mm3,mm2
			
		movq		[edi+edx*4+ 0],mm0
		punpcklbw	mm2,mm5					;[YB][V5][YA][U5][Y9][V4][Y8][U4]
			
		movq		[edi+edx*4+ 8],mm1
		punpckhbw	mm3,mm5					;[YF][V7][YE][U7][YD][V6][YC][U6]
			
		movq		[edi+edx*4+16],mm2
		movq		[edi+edx*4+24],mm3
			
		add			edx,8
		jnc			xyloop
			
		mov			edx,[esp+24+16]			;reload width counter
			
		test		ebp,1					;update U/V row every other row only
		jz			oddline
			
		sub			ebx,edx					;advance U pointer
		sub			ecx,edx					;advance V pointer
			
oddline:
		sub			eax,edx					;advance Y pointer
		sub			eax,edx					;advance Y pointer
			
		add			edi,esi					;advance dest ptr
			
		dec			ebp
		jne			xyloop
		
		pop			ebx
		pop			esi
		pop			edi
		pop			ebp
		emms
	}
}

inline void yuv2yuyv16_mmx(
	unsigned char *py,		// [esp+4+16]
	unsigned char *pu,		// [esp+8+16]
	unsigned char *pv,		// [esp+12+16]
	unsigned char *dst,		// [esp+16+16]
	unsigned long pitch,	// [esp+20+16]
	int w,					// [esp+24+16]
	int h)					// [esp+28+16]
{
	
	int width = w/2;

	__asm {
		push		ebp
		push		edi
		push		esi
		push		ebx
			
		mov			edx,[width];			;load width (mult of 8)
		mov			ebx,[pu]			;load source U ptr
		mov			ecx,[pv]			;load source V ptr
		mov			eax,[py]			;load source Y ptr
		mov			edi,[dst]			;load destination ptr
		mov			esi,[pitch]			;load destination pitch
		mov			ebp,[h]				;load height
			
		lea			ebx,[ebx+edx]			;bias pointers
		lea			ecx,[ecx+edx]			;(we count from -n to 0)
		lea			eax,[eax+edx*2]
		lea			edi,[edi+edx*4]
			
		neg			edx
		mov			[esp+24+16],edx
xyloop:
		movq		mm0,[ebx+edx]			;U0-U7
			
		movq		mm7,[ecx+edx]			;V0-V7
		movq		mm1,mm0					;U0-U7
			
		movq		mm2,[eax+edx*2]			;Y0-Y7
		punpcklbw	mm0,mm7					;[V3][U3][V2][U2][V1][U1][V0][U0]
			
		movq		mm4,[eax+edx*2+8]		;Y8-YF
		punpckhbw	mm1,mm7					;[V7][U7][V6][U6][V5][U5][V4][U4]
			
		movq		mm3,mm2
		punpcklbw	mm2,mm0					;[V1][Y3][U1][Y2][V0][Y1][U0][Y0]
			
		movq		mm5,mm4
		punpckhbw	mm3,mm0					;[V3][Y7][U3][Y6][V2][Y5][U2][Y4]
			
		movq		[edi+edx*4+ 0],mm2
		punpcklbw	mm4,mm1					;[V5][YB][U5][YA][V4][Y9][U4][Y8]
			
		movq		[edi+edx*4+ 8],mm3
		punpckhbw	mm5,mm1					;[V7][YF][U7][YE][V6][YD][U6][YC]
			
		movq		[edi+edx*4+16],mm4
			
		movq		[edi+edx*4+24],mm5
		add			edx,8
			
		jnc			xyloop
			
		mov			edx,[esp+24+16]			;reload width counter
			
		test		ebp,1					;update U/V row every other row only
		jz			oddline
			
		sub			ebx,edx					;advance U pointer
		sub			ecx,edx					;advance V pointer
			
oddline:
		sub			eax,edx					;advance Y pointer
		sub			eax,edx					;advance Y pointer
			
		add			edi,esi					;advance dest ptr
			
		dec			ebp
		jne			xyloop
			
		pop			ebx
		pop			esi
		pop			edi
		pop			ebp
		emms
	}
}

/*

  
	inline void yuv2yv12(
	unsigned char *py,unsigned char *pu,unsigned char *pv,
	unsigned char *dst,
	unsigned long pitch,
	int width,int height);
	
	  inline void yuv2uyvy16_mmx(
	  unsigned char *py,unsigned char *pu,unsigned char *pv,
	  unsigned char *dst,
	  unsigned long pitch,
	  int width,int height);
	  
		inline void yuv2yuyv16_mmx(
		unsigned char *py,unsigned char *pu,unsigned char *pv,
		unsigned char *dst,
		unsigned long pitch,
		int width,int height);
		
*/

color_convert_func ccfunc[4] = {
	yuv2uyvy16_mmx,
	yuv2yuyv16_mmx,	
	yuv2yv12,	
	0
};

#define MAXIMUM_Y_WIDTH 800

#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  __int64
#define uint64_t unsigned __int64

/* colourspace conversion matrix values */
static uint64_t mmw_mult_Y    = 0x2568256825682568;
static uint64_t mmw_mult_U_G  = 0xf36ef36ef36ef36e;
static uint64_t mmw_mult_U_B  = 0x40cf40cf40cf40cf;
static uint64_t mmw_mult_V_R  = 0x3343334333433343;
static uint64_t mmw_mult_V_G  = 0xe5e2e5e2e5e2e5e2;


/* various masks and other constants */
static uint64_t mmb_0x10      = 0x1010101010101010;
static uint64_t mmw_0x0080    = 0x0080008000800080;
static uint64_t mmw_0x00ff    = 0x00ff00ff00ff00ff;

static uint64_t mmw_cut_red   = 0x7c007c007c007c00;
static uint64_t mmw_cut_green = 0x03e003e003e003e0;
static uint64_t mmw_cut_blue  = 0x001f001f001f001f;

void YUV_TO_RGB24(unsigned char *py,  
	unsigned char *pu, unsigned char *pv, int stride_y,
	unsigned char *puc_out, int width_y, int height_y,int stride_out) {

	int y, horiz_count;
	unsigned char *puc_out_remembered;

	int stride_uv = stride_y/2;

	if (height_y < 0) 
	{
		/* we are flipping our output upside-down */
		height_y  = -height_y;
		py     += (height_y   - 1) * stride_y ;
		pu     += (height_y/2 - 1) * stride_uv;
		pv     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}

	horiz_count = -(width_y >> 3);

	for (y=0; y<height_y; y++) {

		if (y == height_y-1) {
			/* this is the last output line
			- we need to be careful not to overrun the end of this line */
			unsigned char temp_buff[3*MAXIMUM_Y_WIDTH+1];
			puc_out_remembered = puc_out;
			puc_out = temp_buff; /* write the RGB to a temporary store */
		}

		_asm {
			push eax
			push ebx
			push ecx
			push edx
			push edi

			mov eax, puc_out       
			mov ebx, py       
			mov ecx, pu       
			mov edx, pv
			mov edi, horiz_count
			
		horiz_loop:

			movd mm2, [ecx]
			pxor mm7, mm7

			movd mm3, [edx]
			punpcklbw mm2, mm7       ; mm2 = __u3__u2__u1__u0

			movq mm0, [ebx]          ; mm0 = y7y6y5y4y3y2y1y0  
			punpcklbw mm3, mm7       ; mm3 = __v3__v2__v1__v0

			movq mm1, mmw_0x00ff     ; mm1 = 00ff00ff00ff00ff 

			psubusb mm0, mmb_0x10    ; mm0 -= 16

			psubw mm2, mmw_0x0080    ; mm2 -= 128
			pand mm1, mm0            ; mm1 = __y6__y4__y2__y0

			psubw mm3, mmw_0x0080    ; mm3 -= 128
			psllw mm1, 3             ; mm1 *= 8

			psrlw mm0, 8             ; mm0 = __y7__y5__y3__y1
			psllw mm2, 3             ; mm2 *= 8

			pmulhw mm1, mmw_mult_Y   ; mm1 *= luma coeff 
			psllw mm0, 3             ; mm0 *= 8

			psllw mm3, 3             ; mm3 *= 8
			movq mm5, mm3            ; mm5 = mm3 = v

			pmulhw mm5, mmw_mult_V_R ; mm5 = red chroma
			movq mm4, mm2            ; mm4 = mm2 = u

			pmulhw mm0, mmw_mult_Y   ; mm0 *= luma coeff 
			movq mm7, mm1            ; even luma part

			pmulhw mm2, mmw_mult_U_G ; mm2 *= u green coeff 
			paddsw mm7, mm5          ; mm7 = luma + chroma    __r6__r4__r2__r0

			pmulhw mm3, mmw_mult_V_G ; mm3 *= v green coeff  
			packuswb mm7, mm7        ; mm7 = r6r4r2r0r6r4r2r0

			pmulhw mm4, mmw_mult_U_B ; mm4 = blue chroma
			paddsw mm5, mm0          ; mm5 = luma + chroma    __r7__r5__r3__r1

			packuswb mm5, mm5        ; mm6 = r7r5r3r1r7r5r3r1
			paddsw mm2, mm3          ; mm2 = green chroma

			movq mm3, mm1            ; mm3 = __y6__y4__y2__y0
			movq mm6, mm1            ; mm6 = __y6__y4__y2__y0

			paddsw mm3, mm4          ; mm3 = luma + chroma    __b6__b4__b2__b0
			paddsw mm6, mm2          ; mm6 = luma + chroma    __g6__g4__g2__g0
			
			punpcklbw mm7, mm5       ; mm7 = r7r6r5r4r3r2r1r0
			paddsw mm2, mm0          ; odd luma part plus chroma part    __g7__g5__g3__g1

			packuswb mm6, mm6        ; mm2 = g6g4g2g0g6g4g2g0
			packuswb mm2, mm2        ; mm2 = g7g5g3g1g7g5g3g1

			packuswb mm3, mm3        ; mm3 = b6b4b2b0b6b4b2b0
			paddsw mm4, mm0          ; odd luma part plus chroma part    __b7__b5__b3__b1

			packuswb mm4, mm4        ; mm4 = b7b5b3b1b7b5b3b1
			punpcklbw mm6, mm2       ; mm6 = g7g6g5g4g3g2g1g0

			punpcklbw mm3, mm4       ; mm3 = b7b6b5b4b3b2b1b0

			/* 32-bit shuffle.... */
			pxor mm0, mm0            ; is this needed?

			movq mm1, mm6            ; mm1 = g7g6g5g4g3g2g1g0
			punpcklbw mm1, mm0       ; mm1 = __g3__g2__g1__g0

			movq mm0, mm3            ; mm0 = b7b6b5b4b3b2b1b0
			punpcklbw mm0, mm7       ; mm0 = r3b3r2b2r1b1r0b0

			movq mm2, mm0            ; mm2 = r3b3r2b2r1b1r0b0

			punpcklbw mm0, mm1       ; mm0 = __r1g1b1__r0g0b0
			punpckhbw mm2, mm1       ; mm2 = __r3g3b3__r2g2b2

			/* 24-bit shuffle and save... */
			movd   [eax], mm0        ; eax[0] = __r0g0b0
			psrlq mm0, 32            ; mm0 = __r1g1b1

			movd  3[eax], mm0        ; eax[3] = __r1g1b1

			movd  6[eax], mm2        ; eax[6] = __r2g2b2
			

			psrlq mm2, 32            ; mm2 = __r3g3b3
	
			movd  9[eax], mm2        ; eax[9] = __r3g3b3

			/* 32-bit shuffle.... */
			pxor mm0, mm0            ; is this needed?

			movq mm1, mm6            ; mm1 = g7g6g5g4g3g2g1g0
			punpckhbw mm1, mm0       ; mm1 = __g7__g6__g5__g4

			movq mm0, mm3            ; mm0 = b7b6b5b4b3b2b1b0
			punpckhbw mm0, mm7       ; mm0 = r7b7r6b6r5b5r4b4

			movq mm2, mm0            ; mm2 = r7b7r6b6r5b5r4b4

			punpcklbw mm0, mm1       ; mm0 = __r5g5b5__r4g4b4
			punpckhbw mm2, mm1       ; mm2 = __r7g7b7__r6g6b6

			/* 24-bit shuffle and save... */
			movd 12[eax], mm0        ; eax[12] = __r4g4b4
			psrlq mm0, 32            ; mm0 = __r5g5b5
			
			movd 15[eax], mm0        ; eax[15] = __r5g5b5
			add ebx, 8               ; py   += 8;

			movd 18[eax], mm2        ; eax[18] = __r6g6b6
			psrlq mm2, 32            ; mm2 = __r7g7b7
			
			add ecx, 4               ; pu   += 4;
			add edx, 4               ; pv   += 4;

			movd 21[eax], mm2        ; eax[21] = __r7g7b7
			add eax, 24              ; puc_out += 24

			inc edi
			jne horiz_loop			

			pop edi 
			pop edx 
			pop ecx
			pop ebx 
			pop eax

			emms
						
		}

		if (y == height_y-1) {
			/* last line of output - we have used the temp_buff and need to copy... */
			int x = 3 * width_y;                  /* interation counter */
			unsigned char *ps = puc_out;                /* source pointer (temporary line store) */
			unsigned char *pd = puc_out_remembered;     /* dest pointer       */
			while (x--) *(pd++) = *(ps++);	      /* copy the line      */
		}

		py   += stride_y;
		if (y%2) {
			pu   += stride_uv;
			pv   += stride_uv;
		}
		puc_out += stride_out; 

	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
void YUV_TO_RGB32(unsigned char *py,  
	unsigned char *pu, unsigned char *pv, int stride_y,
	unsigned char *puc_out, int width_y, int height_y,int stride_out) {

	int y, horiz_count;
	unsigned char *puc_out_remembered;

	int stride_uv = stride_y/2;

	if (height_y < 0) {
		/* we are flipping our output upside-down */
		height_y  = -height_y;
		py     += (height_y   - 1) * stride_y ;
		pu     += (height_y/2 - 1) * stride_uv;
		pv     += (height_y/2 - 1) * stride_uv;
		stride_y  = -stride_y;
		stride_uv = -stride_uv;
	}

	horiz_count = -(width_y>>3);

	for (y=0; y<height_y; y++) {

		if (y == height_y-1) {
			/* this is the last output line
			- we need to be careful not to overrun the end of this line */
			unsigned char temp_buff[4*MAXIMUM_Y_WIDTH+1];
			puc_out_remembered = puc_out;
			puc_out = temp_buff; /* write the RGB to a temporary store */
		}

		_asm {
			push eax
			push ebx
			push ecx
			push edx
			push edi

			mov eax, puc_out       
			mov ebx, py       
			mov ecx, pu       
			mov edx, pv
			mov edi, horiz_count
			
		horiz_loop:

			movd mm2, [ecx]
			pxor mm7, mm7

			movd mm3, [edx]
			punpcklbw mm2, mm7       ; mm2 = __u3__u2__u1__u0

			movq mm0, [ebx]          ; mm0 = y7y6y5y4y3y2y1y0  
			punpcklbw mm3, mm7       ; mm3 = __v3__v2__v1__v0

			movq mm1, mmw_0x00ff     ; mm1 = 00ff00ff00ff00ff 

			psubusb mm0, mmb_0x10    ; mm0 -= 16

			psubw mm2, mmw_0x0080    ; mm2 -= 128
			pand mm1, mm0            ; mm1 = __y6__y4__y2__y0

			psubw mm3, mmw_0x0080    ; mm3 -= 128
			psllw mm1, 3             ; mm1 *= 8

			psrlw mm0, 8             ; mm0 = __y7__y5__y3__y1
			psllw mm2, 3             ; mm2 *= 8

			pmulhw mm1, mmw_mult_Y   ; mm1 *= luma coeff 
			psllw mm0, 3             ; mm0 *= 8

			psllw mm3, 3             ; mm3 *= 8
			movq mm5, mm3            ; mm5 = mm3 = v

			pmulhw mm5, mmw_mult_V_R ; mm5 = red chroma
			movq mm4, mm2            ; mm4 = mm2 = u

			pmulhw mm0, mmw_mult_Y   ; mm0 *= luma coeff 
			movq mm7, mm1            ; even luma part

			pmulhw mm2, mmw_mult_U_G ; mm2 *= u green coeff 
			paddsw mm7, mm5          ; mm7 = luma + chroma    __r6__r4__r2__r0

			pmulhw mm3, mmw_mult_V_G ; mm3 *= v green coeff  
			packuswb mm7, mm7        ; mm7 = r6r4r2r0r6r4r2r0

			pmulhw mm4, mmw_mult_U_B ; mm4 = blue chroma
			paddsw mm5, mm0          ; mm5 = luma + chroma    __r7__r5__r3__r1

			packuswb mm5, mm5        ; mm6 = r7r5r3r1r7r5r3r1
			paddsw mm2, mm3          ; mm2 = green chroma

			movq mm3, mm1            ; mm3 = __y6__y4__y2__y0
			movq mm6, mm1            ; mm6 = __y6__y4__y2__y0

			paddsw mm3, mm4          ; mm3 = luma + chroma    __b6__b4__b2__b0
			paddsw mm6, mm2          ; mm6 = luma + chroma    __g6__g4__g2__g0
			
			punpcklbw mm7, mm5       ; mm7 = r7r6r5r4r3r2r1r0
			paddsw mm2, mm0          ; odd luma part plus chroma part    __g7__g5__g3__g1

			packuswb mm6, mm6        ; mm2 = g6g4g2g0g6g4g2g0
			packuswb mm2, mm2        ; mm2 = g7g5g3g1g7g5g3g1

			packuswb mm3, mm3        ; mm3 = b6b4b2b0b6b4b2b0
			paddsw mm4, mm0          ; odd luma part plus chroma part    __b7__b5__b3__b1

			packuswb mm4, mm4        ; mm4 = b7b5b3b1b7b5b3b1
			punpcklbw mm6, mm2       ; mm6 = g7g6g5g4g3g2g1g0

			punpcklbw mm3, mm4       ; mm3 = b7b6b5b4b3b2b1b0

			/* 32-bit shuffle.... */
			pxor mm0, mm0            ; is this needed?

			movq mm1, mm6            ; mm1 = g7g6g5g4g3g2g1g0
			punpcklbw mm1, mm0       ; mm1 = __g3__g2__g1__g0

			movq mm0, mm3            ; mm0 = b7b6b5b4b3b2b1b0
			punpcklbw mm0, mm7       ; mm0 = r3b3r2b2r1b1r0b0

			movq mm2, mm0            ; mm2 = r3b3r2b2r1b1r0b0

			punpcklbw mm0, mm1       ; mm0 = __r1g1b1__r0g0b0
			punpckhbw mm2, mm1       ; mm2 = __r3g3b3__r2g2b2

			/* 24-bit shuffle and save... */
			movd   [eax], mm0        ; eax[0] = __r0g0b0
			psrlq mm0, 32            ; mm0 = __r1g1b1

			movd  4[eax], mm0        ; eax[3] = __r1g1b1

			movd  8[eax], mm2        ; eax[6] = __r2g2b2

			psrlq mm2, 32            ; mm2 = __r3g3b3
	
			movd  12[eax], mm2        ; eax[9] = __r3g3b3

			/* 32-bit shuffle.... */
			pxor mm0, mm0            ; is this needed?

			movq mm1, mm6            ; mm1 = g7g6g5g4g3g2g1g0
			punpckhbw mm1, mm0       ; mm1 = __g7__g6__g5__g4

			movq mm0, mm3            ; mm0 = b7b6b5b4b3b2b1b0
			punpckhbw mm0, mm7       ; mm0 = r7b7r6b6r5b5r4b4

			movq mm2, mm0            ; mm2 = r7b7r6b6r5b5r4b4

			punpcklbw mm0, mm1       ; mm0 = __r5g5b5__r4g4b4
			punpckhbw mm2, mm1       ; mm2 = __r7g7b7__r6g6b6

			/* 24-bit shuffle and save... */
			movd 16[eax], mm0        ; eax[12] = __r4g4b4
			psrlq mm0, 32            ; mm0 = __r5g5b5
			
			movd 20[eax], mm0        ; eax[15] = __r5g5b5
			add ebx, 8               ; py   += 8;

			movd 24[eax], mm2        ; eax[18] = __r6g6b6
			psrlq mm2, 32            ; mm2 = __r7g7b7
			
			add ecx, 4               ; pu   += 4;
			add edx, 4               ; pv   += 4;

			movd 28[eax], mm2        ; eax[21] = __r7g7b7
			add eax, 32              ; puc_out += 24

			inc edi
			jne horiz_loop			

			pop edi 
			pop edx 
			pop ecx
			pop ebx 
			pop eax

			emms
						
		}

		if (y == height_y-1) {
			/* last line of output - we have used the temp_buff and need to copy... */
			int x = 4 * width_y;                  /* interation counter */
			unsigned char *ps = puc_out;                /* source pointer (temporary line store) */
			unsigned char *pd = puc_out_remembered;     /* dest pointer       */
			while (x--) *(pd++) = *(ps++);	      /* copy the line      */
		}

		py   += stride_y;
		if (y%2) {
			pu   += stride_uv;
			pv   += stride_uv;
		}
		puc_out += stride_out; 

	}
}
/************************************************************************/
/*                                                                      */
/************************************************************************/
long u[256],v[256],y1[256],y2[256];

static void	init_color_table()
{
	static int inited = 0;
	
	if (inited ==0) {
		for	(int i=0;i<256;i++)	{
			v[i]=15938*i-2221300;
			u[i]=20238*i-2771300;
			y1[i]=11644*i;
			y2[i]=19837*i-311710;
		}
		
		inited = 1;
	}
}

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

void YUV_TO_RGB24_C(
	unsigned char *py, unsigned char *pu, unsigned char *pv, 
	int width, int height,unsigned char *out, unsigned outsize)
{
	unsigned char *pY, *pU, *pV;
	unsigned char *pR, *pG, *pB;
	
	int w, h;
	
	init_color_table();
	
	w = width;
	h = height;
	
	pY = py;
	pU = pu;
	pV = pv;
	
	unsigned char *rgbbuf1 = out;
	
	int i,j;
	for	(i=0;i<h;i++) {
		pB=rgbbuf1+outsize-4*w*(i+1);
		pG=rgbbuf1+outsize-4*w*(i+1)+1;
		pR=rgbbuf1+outsize-4*w*(i+1)+2;
		
		for	(j=0;j<w;j+=2) {
			*pR=MAX(0,MIN(255,(v[*pV]+y1[*pY])/10000));
			*pB=MAX(0,MIN(255,(u[*pU]+y1[*pY])/10000));
			*pG=MAX(0,MIN(255,(y2[*pY]-5094*(*pR)-1942*(*pB))/10000));
			
			pR+=3;
			pB+=3;
			pG+=3;
			
			pY++;
			
			*pR=MAX(0,MIN(255,(v[*pV]+y1[*pY])/10000));
			*pB=MAX(0,MIN(255,(u[*pU]+y1[*pY])/10000));
			*pG=MAX(0,MIN(255,(y2[*pY]-5094*(*pR)-1942*(*pB))/10000));
			
			pR+=3;
			pB+=3;
			pG+=3;
			
			pY++;
			
			pU++;
			pV++;
		}
		
		if ((i%2==0)){
			pU-=(w>>1);
			pV-=(w>>1);
		}
	}
}