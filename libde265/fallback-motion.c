
#include "fallback-motion.h"
#include "util.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
# include <malloc.h>
#else
# include <alloca.h>
#endif

#include <assert.h>


void put_unweighted_pred_8_fallback(uint8_t *dst, ptrdiff_t dststride,
                                    int16_t *src, ptrdiff_t srcstride,
                                    int width, int height)
{
  int offset8bit = 32;
  int shift8bit = 6;

  assert((width&1)==0);

  for (int y=0;y<height;y++) {
    int16_t* in  = &src[y*srcstride];
    uint8_t* out = &dst[y*dststride];

    for (int x=0;x<width;x+=2) {
      out[0] = Clip1_8bit((in[0] + offset8bit)>>shift8bit);
      out[1] = Clip1_8bit((in[1] + offset8bit)>>shift8bit);
      out+=2; in+=2;
    }
  }
}


void put_weighted_pred_avg_8_fallback(uint8_t *dst, ptrdiff_t dststride,
                                      int16_t *src1, int16_t *src2,
                                      ptrdiff_t srcstride, int width,
                                      int height)
{
  int offset8bit = 64;
  int shift8bit = 7;

  assert((width&1)==0);

  // I had a special case for 8-pixel parallel, unrolled code,
  // but I did not see any speedup.

#if 0
  for (int y=0;y<height;y++) {
    int16_t* in1 = &src1[y*srcstride];
    int16_t* in2 = &src2[y*srcstride];
    uint8_t* out = &dst[y*dststride];

    for (int x=0;x<width;x++) {
      out[0] = Clip1_8bit((in1[0] + in2[0] + offset8bit)>>shift8bit);
      out++; in1++; in2++;
    }
  }
#endif

#if 0
  if ((width&7)==0) {
    for (int y=0;y<height;y++) {
      int16_t* in1 = &src1[y*srcstride];
      int16_t* in2 = &src2[y*srcstride];
      uint8_t* out = &dst[y*dststride];

      for (int x=0;x<width;x+=8) {
        out[0] = Clip1_8bit((in1[0] + in2[0] + offset8bit)>>shift8bit);
        out[1] = Clip1_8bit((in1[1] + in2[1] + offset8bit)>>shift8bit);
        out[2] = Clip1_8bit((in1[2] + in2[2] + offset8bit)>>shift8bit);
        out[3] = Clip1_8bit((in1[3] + in2[3] + offset8bit)>>shift8bit);
        out[4] = Clip1_8bit((in1[4] + in2[4] + offset8bit)>>shift8bit);
        out[5] = Clip1_8bit((in1[5] + in2[5] + offset8bit)>>shift8bit);
        out[6] = Clip1_8bit((in1[6] + in2[6] + offset8bit)>>shift8bit);
        out[7] = Clip1_8bit((in1[7] + in2[7] + offset8bit)>>shift8bit);
        out+=8; in1+=8; in2+=8;
      }
    }
  }
  else
#endif
    {
      for (int y=0;y<height;y++) {
        int16_t* in1 = &src1[y*srcstride];
        int16_t* in2 = &src2[y*srcstride];
        uint8_t* out = &dst[y*dststride];

        for (int x=0;x<width;x+=2) {
          out[0] = Clip1_8bit((in1[0] + in2[0] + offset8bit)>>shift8bit);
          out[1] = Clip1_8bit((in1[1] + in2[1] + offset8bit)>>shift8bit);
          out+=2; in1+=2; in2+=2;
        }
      }
    }
}



void put_epel_8_fallback(int16_t *out, ptrdiff_t out_stride,
                         uint8_t *src, ptrdiff_t src_stride,
                         int width, int height,
                         int mx, int my, int16_t* mcbuffer)
{
  int shift3 = 6;

  for (int y=0;y<height;y++) {
    int16_t* o = &out[y*out_stride];
    uint8_t* i = &src[y*src_stride];

    for (int x=0;x<width;x++) {
      *o = *i << shift3;
      o++;
      i++;
    }
  }
}


void put_epel_hv_8_fallback(int16_t *dst, ptrdiff_t dst_stride,
                            uint8_t *src, ptrdiff_t src_stride,
                            int nPbWC, int nPbHC,
                            int xFracC, int yFracC, int16_t* mcbuffer)
{
  const int shift1 = 0;
  const int shift2 = 6;
  const int shift3 = 6;

  int extra_left = 1;
  int extra_top  = 1;
  int extra_right = 2;
  int extra_bottom= 2;

  int nPbW_extra = extra_left + nPbWC + extra_right;
  int nPbH_extra = extra_top  + nPbHC + extra_bottom;

  int16_t* tmp2buf = (int16_t*)alloca( nPbWC      * nPbH_extra * sizeof(int16_t) );

  /*
  printf("x,y FracC: %d/%d\n",xFracC,yFracC);


  printf("---IN---\n");

  for (int y=-extra_top;y<nPbHC+extra_bottom;y++) {
    uint8_t* p = &src[y*src_stride -extra_left];

    for (int x=-extra_left;x<nPbWC+extra_right;x++) {
      printf("%05d ",*p << 6);
      p++;
    }
    printf("\n");
  }
  */


  // H-filters

  logtrace(LogMotion,"---H---\n");
  //printf("---H---(%d)\n",xFracC);

  for (int y=-extra_top;y<nPbHC+extra_bottom;y++) {
    uint8_t* p = &src[y*src_stride - extra_left];

    for (int x=0;x<nPbWC;x++) {
      int16_t v;
      switch (xFracC) {
      case 0: v = p[1]; break;
      case 1: v = (-2*p[0]+58*p[1]+10*p[2]-2*p[3])>>shift1; break;
      case 2: v = (-4*p[0]+54*p[1]+16*p[2]-2*p[3])>>shift1; break;
      case 3: v = (-6*p[0]+46*p[1]+28*p[2]-4*p[3])>>shift1; break;
      case 4: v = (-4*p[0]+36*p[1]+36*p[2]-4*p[3])>>shift1; break;
      case 5: v = (-4*p[0]+28*p[1]+46*p[2]-6*p[3])>>shift1; break;
      case 6: v = (-2*p[0]+16*p[1]+54*p[2]-4*p[3])>>shift1; break;
      case 7: v = (-2*p[0]+10*p[1]+58*p[2]-2*p[3])>>shift1; break;
      }

      //printf("%d %d %d %d -> %d\n",p[0],p[1],p[2],p[3],v);
        
      tmp2buf[y+extra_top + x*nPbH_extra] = v;
      p++;

      //printf("%05d ",tmp2buf[y+extra_top + x*nPbH_extra]);
    }
    //printf("\n");
  }

  // V-filters

  int vshift = (xFracC==0 ? shift1 : shift2);

  for (int x=0;x<nPbWC;x++) {
    int16_t* p = &tmp2buf[x*nPbH_extra];

    for (int y=0;y<nPbHC;y++) {
      int16_t v;
      //logtrace(LogMotion,"%x %x %x  %x  %x %x %x\n",p[0],p[1],p[2],p[3],p[4],p[5],p[6]);

      switch (yFracC) {
      case 0: v = p[1]; break;
      case 1: v = (-2*p[0]+58*p[1]+10*p[2]-2*p[3])>>vshift; break;
      case 2: v = (-4*p[0]+54*p[1]+16*p[2]-2*p[3])>>vshift; break;
      case 3: v = (-6*p[0]+46*p[1]+28*p[2]-4*p[3])>>vshift; break;
      case 4: v = (-4*p[0]+36*p[1]+36*p[2]-4*p[3])>>vshift; break;
      case 5: v = (-4*p[0]+28*p[1]+46*p[2]-6*p[3])>>vshift; break;
      case 6: v = (-2*p[0]+16*p[1]+54*p[2]-4*p[3])>>vshift; break;
      case 7: v = (-2*p[0]+10*p[1]+58*p[2]-2*p[3])>>vshift; break;
      }
        
      dst[x + y*dst_stride] = v;
      p++;
    }

  }

  /*
  printf("---V---\n");
  for (int y=0;y<nPbHC;y++) {
    for (int x=0;x<nPbWC;x++) {
      printf("%05d ",dst[x+y*dst_stride]);
    }
    printf("\n");
  }
  */
}




void put_qpel_0_0_fallback(int16_t *out, ptrdiff_t out_stride,
                           uint8_t *src, ptrdiff_t srcstride,
                           int nPbW, int nPbH, int16_t* mcbuffer)
{
  const int shift1 = 0; // sps->BitDepth_Y-8;
  const int shift2 = 6;

  // straight copy

  for (int y=0;y<nPbH;y++) {
      uint8_t* p = src + srcstride*y;
      int16_t* o = out + out_stride*y;

      for (int x=0;x<nPbW;x+=4) {
#if 0
        *o = *p << shift2;
        o++; p++;
#else
        // does not seem to be faster...
        int16_t o0,o1,o2,o3;
        o0 = p[0] << shift2;
        o1 = p[1] << shift2;
        o2 = p[2] << shift2;
        o3 = p[3] << shift2;
        o[0]=o0;
        o[1]=o1;
        o[2]=o2;
        o[3]=o3;

        o+=4;
        p+=4;
#endif
      }
  }
}



static int extra_before[4] = { 0,3,3,2 };
static int extra_after [4] = { 0,3,4,4 };

void put_qpel_fallback(int16_t *out, ptrdiff_t out_stride,
                       uint8_t *src, ptrdiff_t srcstride,
                       int nPbW, int nPbH, int16_t* mcbuffer,
                       int xFracL, int yFracL)
{
  int extra_left   = extra_before[xFracL];
  int extra_right  = extra_after [xFracL];
  int extra_top    = extra_before[yFracL];
  int extra_bottom = extra_after [yFracL];

  int nPbW_extra = extra_left + nPbW + extra_right;
  int nPbH_extra = extra_top  + nPbH + extra_bottom;

  const int shift1 = 0; // sps->BitDepth_Y-8;
  const int shift2 = 6;


  // H-filters

  switch (xFracL) {
  case 0:
    for (int y=-extra_top;y<nPbH+extra_bottom;y++) {
      uint8_t* p = src + srcstride*y - extra_left;
      int16_t* o = &mcbuffer[y+extra_top];

      for (int x=0;x<nPbW;x++) {
        *o = *p;
        o += nPbH_extra;
        p++;
      }
    }
    break;
  case 1:
    for (int y=-extra_top;y<nPbH+extra_bottom;y++) {
      uint8_t* p = src + srcstride*y - extra_left;
      int16_t* o = &mcbuffer[y+extra_top];

      for (int x=0;x<nPbW;x++) {
        *o = (-p[0]+4*p[1]-10*p[2]+58*p[3]+17*p[4] -5*p[5]  +p[6])>>shift1;
        o += nPbH_extra;
        p++;
      }
    }
    break;
  case 2:
    for (int y=-extra_top;y<nPbH+extra_bottom;y++) {
      uint8_t* p = src + srcstride*y - extra_left;
      int16_t* o = &mcbuffer[y+extra_top];

      for (int x=0;x<nPbW;x++) {
        *o = (-p[0]+4*p[1]-11*p[2]+40*p[3]+40*p[4]-11*p[5]+4*p[6]-p[7])>>shift1;
        o += nPbH_extra;
        p++;
      }
    }
    break;
  case 3:
    for (int y=-extra_top;y<nPbH+extra_bottom;y++) {
      uint8_t* p = src + srcstride*y - extra_left;
      int16_t* o = &mcbuffer[y+extra_top];

      for (int x=0;x<nPbW;x++) {
        *o = ( p[0]-5*p[1]+17*p[2]+58*p[3]-10*p[4] +4*p[5]  -p[6])>>shift1;
        o += nPbH_extra;
        p++;
      }
    }
    break;
  }


  logtrace(LogMotion,"---H---\n");

  for (int y=-extra_top;y<nPbH+extra_bottom;y++) {
    for (int x=0;x<nPbW;x++) {
      logtrace(LogMotion,"%04x ",mcbuffer[y+extra_top + x*nPbH_extra]);
    }
    logtrace(LogMotion,"\n");
  }

  // V-filters

  int vshift = (xFracL==0 ? shift1 : shift2);

  switch (yFracL) {
  case 0:
    for (int x=0;x<nPbW;x++) {
      int16_t* p = &mcbuffer[x*nPbH_extra];
      int16_t* o = &out[x];
              
      for (int y=0;y<nPbH;y++) {
        *o = *p;
        o+=out_stride;
        p++;
      }
    }
    break;
  case 1:
    for (int x=0;x<nPbW;x++) {
      int16_t* p = &mcbuffer[x*nPbH_extra];
      int16_t* o = &out[x];
              
      for (int y=0;y<nPbH;y++) {
        *o = (-p[0]+4*p[1]-10*p[2]+58*p[3]+17*p[4] -5*p[5]  +p[6])>>vshift;
        o+=out_stride;
        p++;
      }
    }
    break;
  case 2:
    for (int x=0;x<nPbW;x++) {
      int16_t* p = &mcbuffer[x*nPbH_extra];
      int16_t* o = &out[x];
              
      for (int y=0;y<nPbH;y++) {
        *o = (-p[0]+4*p[1]-11*p[2]+40*p[3]+40*p[4]-11*p[5]+4*p[6]-p[7])>>vshift;
        o+=out_stride;
        p++;
      }
    }
    break;
  case 3:
    for (int x=0;x<nPbW;x++) {
      int16_t* p = &mcbuffer[x*nPbH_extra];
      int16_t* o = &out[x];
              
      for (int y=0;y<nPbH;y++) {
        *o = ( p[0]-5*p[1]+17*p[2]+58*p[3]-10*p[4] +4*p[5]  -p[6])>>vshift;
        o+=out_stride;
        p++;
      }
    }
    break;
  }


  logtrace(LogMotion,"---V---\n");
  for (int y=0;y<nPbH;y++) {
    for (int x=0;x<nPbW;x++) {
      logtrace(LogMotion,"%04x ",out[x+y*out_stride]);
    }
    logtrace(LogMotion,"\n");
  }
}


#define QPEL(x,y) void put_qpel_ ## x ## _ ## y ## _fallback(int16_t *out, ptrdiff_t out_stride,    \
                                             uint8_t *src, ptrdiff_t srcstride,     \
                                             int nPbW, int nPbH, int16_t* mcbuffer) \
{ put_qpel_fallback(out,out_stride, src,srcstride, nPbW,nPbH,mcbuffer,x,y ); }

/*     */ QPEL(0,1) QPEL(0,2) QPEL(0,3)
QPEL(1,0) QPEL(1,1) QPEL(1,2) QPEL(1,3)
QPEL(2,0) QPEL(2,1) QPEL(2,2) QPEL(2,3)
QPEL(3,0) QPEL(3,1) QPEL(3,2) QPEL(3,3)
