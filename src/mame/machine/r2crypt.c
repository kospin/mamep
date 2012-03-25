/* Raiden 2 Sprite Decryption */

#include "emu.h"
#include "includes/raiden2.h"

/* INIT */

static const int swx[32] = {
  25, 28, 15, 19,  6,  0,  3, 24,
  11,  1,  2, 30, 16,  7, 22, 17,
  31, 14, 23,  9, 27, 18,  4, 10,
  13, 20,  5, 12,  8, 29, 26, 21,
};

static UINT32 sw(UINT32 v)
{
  UINT32 r = 0;
  int i;
  for(i=0; i<32; i++)
    if(v & (1 << swx[i]))
      r |= 1 << (31-i);
  return r;
}

static const UINT8 rotate[512] = {
  0x11, 0x17, 0x0d, 0x03, 0x17, 0x1f, 0x08, 0x1a, 0x0f, 0x04, 0x1e, 0x13, 0x19, 0x0e, 0x0e, 0x05,
  0x06, 0x07, 0x08, 0x08, 0x0d, 0x18, 0x11, 0x1a, 0x0b, 0x06, 0x12, 0x0c, 0x1f, 0x0b, 0x1c, 0x19,
  0x00, 0x1b, 0x0c, 0x09, 0x1d, 0x18, 0x1a, 0x16, 0x1a, 0x08, 0x03, 0x04, 0x0f, 0x1d, 0x16, 0x07,
  0x1a, 0x12, 0x01, 0x0b, 0x00, 0x0f, 0x1e, 0x10, 0x09, 0x0f, 0x10, 0x09, 0x0a, 0x1c, 0x0d, 0x08,
  0x06, 0x1a, 0x06, 0x02, 0x11, 0x1e, 0x0c, 0x1c, 0x11, 0x0f, 0x19, 0x0a, 0x16, 0x14, 0x18, 0x11,
  0x0b, 0x0d, 0x1c, 0x1f, 0x0d, 0x1f, 0x0d, 0x19, 0x0d, 0x04, 0x19, 0x0f, 0x06, 0x13, 0x0c, 0x1b,
  0x1f, 0x12, 0x15, 0x1a, 0x04, 0x02, 0x06, 0x03, 0x0a, 0x0d, 0x12, 0x09, 0x17, 0x1d, 0x12, 0x10,
  0x05, 0x07, 0x03, 0x00, 0x14, 0x07, 0x14, 0x1a, 0x1c, 0x0a, 0x10, 0x0f, 0x0b, 0x0c, 0x08, 0x0f,
  0x07, 0x00, 0x13, 0x1c, 0x04, 0x15, 0x0e, 0x02, 0x17, 0x17, 0x00, 0x03, 0x18, 0x00, 0x02, 0x13,
  0x14, 0x0c, 0x01, 0x0a, 0x15, 0x0b, 0x0a, 0x1c, 0x1b, 0x06, 0x17, 0x1d, 0x11, 0x1f, 0x10, 0x04,
  0x1a, 0x01, 0x1b, 0x13, 0x03, 0x09, 0x09, 0x0f, 0x0d, 0x03, 0x15, 0x1c, 0x04, 0x06, 0x06, 0x0b,
  0x04, 0x0a, 0x1f, 0x16, 0x11, 0x0a, 0x05, 0x05, 0x0c, 0x1c, 0x10, 0x0c, 0x11, 0x04, 0x10, 0x1a,
  0x06, 0x10, 0x19, 0x06, 0x15, 0x0f, 0x11, 0x01, 0x10, 0x0c, 0x1d, 0x05, 0x1f, 0x05, 0x12, 0x16,
  0x02, 0x12, 0x14, 0x0d, 0x14, 0x0f, 0x04, 0x07, 0x13, 0x01, 0x11, 0x1c, 0x1c, 0x1d, 0x0e, 0x06,
  0x1d, 0x13, 0x10, 0x06, 0x0f, 0x02, 0x12, 0x10, 0x1e, 0x0c, 0x17, 0x15, 0x0b, 0x1f, 0x01, 0x19,
  0x02, 0x01, 0x07, 0x1d, 0x13, 0x19, 0x0f, 0x0f, 0x10, 0x03, 0x1e, 0x03, 0x0d, 0x0a, 0x0c, 0x0d,

  0x16, 0x1f, 0x16, 0x1a, 0x1c, 0x16, 0x01, 0x03, 0x01, 0x08, 0x14, 0x19, 0x03, 0x1e, 0x08, 0x02,
  0x02, 0x1d, 0x15, 0x00, 0x09, 0x1d, 0x03, 0x11, 0x11, 0x0b, 0x1b, 0x14, 0x01, 0x1e, 0x11, 0x12,
  0x1d, 0x06, 0x0b, 0x13, 0x1e, 0x16, 0x0d, 0x10, 0x11, 0x1f, 0x1c, 0x15, 0x0d, 0x1a, 0x13, 0x1f,
  0x0e, 0x05, 0x10, 0x06, 0x0d, 0x1c, 0x07, 0x19, 0x06, 0x1d, 0x11, 0x00, 0x1c, 0x05, 0x0b, 0x1d,
  0x1c, 0x06, 0x05, 0x1d, 0x00, 0x13, 0x00, 0x12, 0x1b, 0x17, 0x1a, 0x1b, 0x17, 0x1c, 0x16, 0x0a,
  0x11, 0x15, 0x0f, 0x0b, 0x0f, 0x07, 0x0e, 0x04, 0x13, 0x00, 0x1c, 0x05, 0x16, 0x00, 0x1a, 0x04,
  0x17, 0x04, 0x08, 0x1b, 0x05, 0x12, 0x1d, 0x0d, 0x02, 0x16, 0x12, 0x0e, 0x06, 0x08, 0x14, 0x07,
  0x0e, 0x0f, 0x15, 0x13, 0x12, 0x00, 0x1d, 0x16, 0x1b, 0x18, 0x1f, 0x05, 0x12, 0x13, 0x01, 0x0c,
  0x12, 0x04, 0x19, 0x13, 0x12, 0x15, 0x07, 0x06, 0x0a, 0x00, 0x09, 0x14, 0x1e, 0x03, 0x10, 0x1b,
  0x08, 0x1a, 0x07, 0x02, 0x1b, 0x0d, 0x18, 0x13, 0x02, 0x07, 0x1e, 0x05, 0x15, 0x02, 0x06, 0x18,
  0x12, 0x09, 0x1c, 0x07, 0x0b, 0x02, 0x03, 0x00, 0x18, 0x18, 0x03, 0x0f, 0x02, 0x0f, 0x10, 0x09,
  0x05, 0x18, 0x08, 0x1b, 0x0d, 0x10, 0x03, 0x00, 0x0c, 0x14, 0x1d, 0x08, 0x02, 0x10, 0x0b, 0x0c,
  0x00, 0x0d, 0x0d, 0x0a, 0x06, 0x1c, 0x09, 0x19, 0x1b, 0x14, 0x18, 0x0f, 0x02, 0x07, 0x05, 0x04,
  0x1c, 0x15, 0x18, 0x00, 0x0b, 0x10, 0x19, 0x1c, 0x1b, 0x08, 0x1d, 0x12, 0x17, 0x1d, 0x0c, 0x01,
  0x03, 0x0d, 0x03, 0x0d, 0x15, 0x0e, 0x16, 0x08, 0x05, 0x11, 0x1f, 0x03, 0x16, 0x03, 0x0f, 0x10,
  0x08, 0x19, 0x18, 0x15, 0x1f, 0x05, 0x00, 0x09, 0x0e, 0x05, 0x16, 0x1b, 0x01, 0x08, 0x08, 0x1f,
};


static const UINT32 xmap_low_01[8] = { 0x915b174c, 0xd1e3d41d, 0x7afd901e, 0x890aeda6, 0xdaa66bf6, 0xcf3a5859, 0x1fc8ae80, 0xd7c864c2 };
static const UINT32 xmap_low_03[8] = { 0xc9b43501, 0x2d4136ef, 0x5a3e2047, 0xccab4852, 0x67770213, 0xcc1c22ee, 0x7f767fe5, 0xae783fa3 };
static const UINT32 xmap_low_07[8] = { 0x533ce0ff, 0x21561e2b, 0x5e52735b, 0x2f89d3c0, 0x383ee980, 0x807ae78a, 0x6dfab360, 0xccd84e92 };
static const UINT32 xmap_low_23[8] = { 0xa3b39673, 0xb3a21d4a, 0x07440937, 0xa9005a05, 0x12bbf9d7, 0x257164a7, 0x6162a1e4, 0x862c5d73 };

static const UINT32 xmap_low_31[8] = { 0x76fa8a84, 0x2f3f4960, 0x82087362, 0x40aebf9e, 0x02854535, 0xfcbd325a, 0x7b8823f3, 0xcbd62b3a };

static const UINT32 xmap_high_00[8] = { 0x1bf05217, 0xe2b31951, 0x0458ee47, 0x6c06f22c, 0x3f1a7bad, 0xb658f2e4, 0xa2b24b18, 0x3cddd22f };
static const UINT32 xmap_high_02[8] = { 0x3caa374d, 0xfabf45a5, 0x2633d9ba, 0x05573b6a, 0x03234029, 0x185b17b0, 0x53afc974, 0x2067077d };
static const UINT32 xmap_high_03[8] = { 0xdb36b4d7, 0x1e79e916, 0xfcc75654, 0x8b552464, 0x856a3eb4, 0xb60c7c2e, 0xf325d2ee, 0x5cbd9b38 };
static const UINT32 xmap_high_04[8] = { 0x91a1acfe, 0x5adaac01, 0x9dc40024, 0x1c87c08b, 0x34ab1b76, 0x631175d5, 0x017b85e6, 0x13359cd1 };
static const UINT32 xmap_high_06[8] = { 0xd46b6286, 0x2da93768, 0xf95f5b47, 0x657b472e, 0x05ed940f, 0x86364f88, 0x863d5fed, 0xe3f1ef82 };
static const UINT32 xmap_high_21[8] = { 0x1d51f8b6, 0xcc1b30b3, 0x9bf75b9d, 0x2c57e2cd, 0x3b5138de, 0xba5c69c4, 0x422c4b8e, 0xd5465cf6 };
static const UINT32 xmap_high_20[8] = { 0x41d4146c, 0x536d7b04, 0x59d60240, 0x7d01cc23, 0x8a0e5ce4, 0x11e0b0db, 0x513381e1, 0x3264be61 };
static const UINT32 xmap_high_10[8] = { 0xc04f0362, 0x44fa6936, 0xc048b0db, 0x704897b2, 0x7e28568f, 0xfb9e070f, 0xc34a5704, 0xd5888a6f };
static const UINT32 xmap_high_11[8] = { 0xd88e9b92, 0xda49726b, 0xc13f86b7, 0x6ce2a1b0, 0xb3adc6e9, 0xd83c2f64, 0xa14c1efc, 0xe98a3c19 };
static const UINT32 xmap_high_13[8] = { 0x03f8a061, 0x19f39b5a, 0x13a17ae2, 0x85c06682, 0x42118566, 0x78e4ff8a, 0xbee64f97, 0x5eecb443 };
static const UINT32 xmap_high_15[8] = { 0x1c6f2b4f, 0x9eebe281, 0x784b85d8, 0x401d6412, 0x0370ae0a, 0xa791d0b3, 0x89d290ea, 0x4666f009 };

#if 0
static UINT32 xrot(UINT32 v, int r)
{
  return (v >> r) | (v << (32-r));
}
#endif

static UINT32 yrot(UINT32 v, int r)
{
  return (v << r) | (v >> (32-r));
}

static int bt(const UINT32 *tb, int v)
{
  return (tb[v/32] & (1<<(v % 32))) != 0;
}

static UINT32 gr(int i)
{
  int idx = i & 0xff;
  if(i & 0x008000)
    idx ^= 1;
  if(i & 0x100000)
    idx ^= 256;
  return rotate[idx];
}

static UINT32 gm(int i)
{
  unsigned int x;
  int idx = i & 0xff;
  int i1, i2;

  if(i & 0x008000)
    idx ^= 1;
  if(i & 0x100000)
    idx ^= 256;

  i1 = idx & 0xff;
  i2 = (i >> 8) & 0xff;

  x = 0x4c435012;

  if(bt(xmap_low_01, i1))
    x ^= 0x00401000;
  if(bt(xmap_low_03, i1))
    x ^= 0x01000800;
  if(bt(xmap_low_07, i1))
    x ^= 0x00044000;
  if(bt(xmap_low_23, i1))
    x ^= 0x00102000;
  if(bt(xmap_low_31, i1))
    x ^= 0x00008000;
  if(bt(xmap_high_00, i2))
    x ^= 0x0c000400;
  if(bt(xmap_high_02, i2))
    x ^= 0x00200020;
  if(bt(xmap_high_03, i2))
    x ^= 0x02000008;
  if(bt(xmap_high_04, i2))
    x ^= 0x10000200;
  if(bt(xmap_high_06, i2))
    x ^= 0x08000004;
  if(bt(xmap_high_21, i2))
    x ^= 0x80000001;
  if(bt(xmap_high_20, i2))
    x ^= 0x00080040;
  if(bt(xmap_high_10, i2))
    x ^= 0x40000100;
  if(bt(xmap_high_11, i2))
    x ^= 0x00800010;
  if(bt(xmap_high_13, i2))
    x ^= 0x00020080;
  if(bt(xmap_high_15, i2))
    x ^= 0x20000002;

  if(i & 0x010000)
    x ^= 0xaa00000f;
  if(i & 0x020000)
    x ^= 0x00aa00f0;
  if(i & 0x040000)
    x ^= 0x5d000f00;
  if(i & 0x080000)
    x ^= 0x0054f000;

  return x;
}

static UINT32 trans(UINT32 v, UINT32 x)
{
  unsigned y,r2,v2;

  v2 = (BIT(v,30)<<8)|(BIT(v,22)<<12)|(BIT(v,18)<<18)|(BIT(v,19)<<19)|(BIT(v,22)<<22)|(BIT(v,24)<<24)|(BIT(v,3)<<25)|(BIT(v,26)<<26)|(BIT(v,28)<<28);
  v2 ^= 0x01000000;

  r2 = 0;
  y = 0;

  y |= ((                              0 ^ BIT(x, 0) ^ BIT(v, 0))<< 0);
  y |= ((                              0 ^ BIT(x, 1) ^ BIT(v, 1))<< 1);
  y |= ((                              0 ^ BIT(x, 2) ^ BIT(v, 2))<< 2);
  y |= ((                              0 ^ BIT(x, 3) ^ BIT(v, 3))<< 3);
  y |= ((                              0 ^ BIT(x, 4) ^ BIT(v, 4))<< 4);
  y |= ((                              0 ^ BIT(x, 5) ^ BIT(v, 5))<< 5);
  y |= ((                              0 ^ BIT(x, 6) ^ BIT(v, 6))<< 6);
  y |= ((                              0 ^ BIT(x, 7) ^ BIT(v, 7))<< 7);
  y |= ((                              0 ^ BIT(x, 8) ^ BIT(v, 8))<< 8);
  y |= ((                              0 ^ BIT(x, 9) ^ BIT(v, 9))<< 9);
  y |= ((                              0 ^ BIT(x,10) ^ BIT(v,10))<<10);
  y |= ((                              0 ^ BIT(x,11) ^ BIT(v,11))<<11);
  y |= ((                              0 ^ BIT(x,12) ^ BIT(v,12))<<12);
  y |= ((                              0 ^ BIT(x,13) ^ BIT(v,13))<<13);
  y |= ((                              0 ^ BIT(x,14) ^ BIT(v,14))<<14);
  y |= ((                              0 ^ BIT(x,15) ^ BIT(v,15))<<15);
  y |= ((                              0 ^ BIT(x,16) ^ BIT(v,16))<<16);
  y |= ((                              0 ^ BIT(x,17) ^ BIT(v,17))<<17);
  y |= ((                              0 ^ BIT(x,18) ^ BIT(v,18))<<18);
  y |= ((                              1 ^ BIT(x,19) ^ BIT(v,19))<<19);
  y |= ((                      BIT(x,19) ^ BIT(x,20) ^ BIT(v,20))<<20);
  y |= ((                              0 ^ BIT(x,21) ^ BIT(v,21))<<21);
  y |= ((                              0 ^ BIT(x,22) ^ BIT(v,22))<<22);
  y |= ((                              0 ^ BIT(x,23) ^ BIT(v,23))<<23);
  y |= ((                              1 ^ BIT(x,24) ^ BIT(v,24))<<24);
  y |= ((                      BIT(x,24) ^ BIT(x,25) ^ BIT(v,25))<<25);
  y |= ((          (BIT(x,24)&BIT(x,25)) ^ BIT(x,26) ^ BIT(v,26))<<26);
  y |= (((BIT(x,24)&BIT(x,25)&BIT(x,26)) ^ BIT(x,27) ^ BIT(v,27))<<27);
  y |= ((                              0 ^ BIT(x,28) ^ BIT(v,28))<<28);
  y |= ((                              0 ^ BIT(x,29) ^ BIT(v,29))<<29);
  y |= ((                              0 ^ BIT(x,30) ^ BIT(v,30))<<30);
  y |= ((                              0 ^ BIT(x,31) ^ BIT(v,31))<<31);


  r2 |= (                     0<< 0);
  r2 |= (                     0<< 1);
  r2 |= (                     0<< 2);
  r2 |= (                     0<< 3);
  r2 |= (                     0<< 4);
  r2 |= (                     0<< 5);
  r2 |= (                     0<< 6);
  r2 |= (                     0<< 7);
  r2 |= (                     0<< 8);
  r2 |= ((BIT(v2, 8)&BIT(y, 8))<< 9);
  r2 |= (                     0<<10);
  r2 |= (                     0<<11);
  r2 |= (                     0<<12);
  r2 |= ((BIT(v2,12)&BIT(y,12))<<13);
  r2 |= (                     0<<14);
  r2 |= (                     0<<15);
  r2 |= (                     0<<16);
  r2 |= (                     0<<17);
  r2 |= (                     0<<18);
  r2 |= ((BIT(v2,18)&BIT(y,18))<<19);
  r2 |= ((BIT(v2,19)&BIT(y,19))<<20);
  r2 |= (                     0<<21);
  r2 |= (                     0<<22);
  r2 |= ((BIT(v2,22)&BIT(y,22))<<23);
  r2 |= (                     0<<24);
  r2 |= ((BIT(v2,24)&BIT(y,24))<<25);
  r2 |= ((BIT(v2,25)&BIT(y,25))<<26);
  r2 |= ((BIT(v2,26)&BIT(y,26))<<27);
  r2 |= (                     0<<28);
  r2 |= ((BIT(v2,28)&BIT(y,28))<<29);
  r2 |= (                     0<<30);
  r2 |= (                     0<<31);

  r2 ^= y;
  r2 ^= 0x0c500000;

  return r2;
}

void raiden2_decrypt_sprites(running_machine &machine)
{
  int i;
  UINT32 *data = (UINT32 *)machine.region("gfx3")->base();
  for(i=0; i<0x800000/4; i++) {
    UINT32 x1, v1, y1;

    int idx = i & 0xff;
    //int i2;
    //int idx2;

    //idx2 = ((i>>7) & 0x3ff) | ((i>>8) & 0x400);
    if(i & 0x008000)
      idx ^= 1;
    if(i & 0x100000)
      idx ^= 256;

    //i2 = i >> 8;

    v1 = sw(yrot(data[i], gr(i)));

    x1 = gm(i);

    y1 = ~trans(v1, x1);

    data[i] = y1;
  }
}

void zeroteam_decrypt_sprites(running_machine &machine)
{
	// TODO!
}

