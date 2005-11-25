/*************************************************************************

    Generic 3dfx Voodoo rasterization

**************************************************************************/

static void render_0c000035_00045119_000b4779_0824101f(void);
static void render_0c000035_00045119_000b4779_0824109f(void);
static void render_0c000035_00045119_000b4779_082410df(void);
static void render_0c000035_00045119_000b4779_082418df(void);

static void render_0c600c09_00045119_000b4779_0824100f(void);
static void render_0c600c09_00045119_000b4779_0824180f(void);
static void render_0c600c09_00045119_000b4779_082418cf(void);
static void render_0c480035_00045119_000b4779_082418df(void);
static void render_0c480035_00045119_000b4379_082418df(void);

static void render_0c000035_00040400_000b4739_0c26180f(void);
static void render_0c582c35_00515110_000b4739_0c26180f(void);
static void render_0c000035_64040409_000b4739_0c26180f(void);
static void render_0c002c35_64515119_000b4799_0c26180f(void);
static void render_0c582c35_00515110_000b4739_0c2618cf(void);
static void render_0c002c35_40515119_000b4739_0c26180f(void);

static void render_0c002435_04045119_00034279_0c26180f(void);
static void render_0c002425_00045119_00034679_0c26180f(void);


static int init_generator(void)
{
	/* no init needed */
	return 1;
}


static struct rasterizer_info *generate_rasterizer(void)
{
	struct rasterizer_info *info;
	UINT32 temp;

	temp = voodoo_regs[fbzColorPath] & FBZCOLORPATH_MASK;
	if (temp == 0x0c000035)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0824101f)
					return add_rasterizer(render_0c000035_00045119_000b4779_0824101f);	/* wg3dh */
				else if (temp == 0x0824109f)
					return add_rasterizer(render_0c000035_00045119_000b4779_0824109f);	/* wg3dh */
				else if (temp == 0x082410df)
					return add_rasterizer(render_0c000035_00045119_000b4779_082410df);	/* wg3dh */
				else if (temp == 0x082418df)
					return add_rasterizer(render_0c000035_00045119_000b4779_082418df);	/* wg3dh */
			}
		}
		else if (temp == 0x00040400)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					return add_rasterizer(render_0c000035_00040400_000b4739_0c26180f);	/* blitz99 */
			}
		}
		else if (temp == 0x64040409)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					return add_rasterizer(render_0c000035_64040409_000b4739_0c26180f);	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c002c35)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x64515119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4799)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					return add_rasterizer(render_0c002c35_64515119_000b4799_0c26180f);	/* blitz99 */
			}
		}
		else if (temp == 0x40515119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					return add_rasterizer(render_0c002c35_40515119_000b4739_0c26180f);	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c582c35)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00515110)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					return add_rasterizer(render_0c582c35_00515110_000b4739_0c26180f);	/* blitz99 */
				else if (temp == 0x0c2618cf)
					return add_rasterizer(render_0c582c35_00515110_000b4739_0c2618cf);	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c600c09)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0824100f)
					return add_rasterizer(render_0c600c09_00045119_000b4779_0824100f);	/* mace */
				else if (temp == 0x0824180f)
					return add_rasterizer(render_0c600c09_00045119_000b4779_0824180f);	/* mace */
				else if (temp == 0x082418cf)
					return add_rasterizer(render_0c600c09_00045119_000b4779_082418cf);	/* mace */
			}
		}
	}
	else if (temp == 0x0c480035)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x082418df)
					return add_rasterizer(render_0c480035_00045119_000b4779_082418df);	/* mace */
			}
			else if (temp == 0x000b4379)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x082418df)
					return add_rasterizer(render_0c480035_00045119_000b4379_082418df);	/* mace */
			}
		}
	}
	else if (temp == 0x0c002425)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x00034679)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					return add_rasterizer(render_0c002425_00045119_00034679_0c26180f);	/* carnevil */
			}
		}
	}
	else if (temp == 0x0c002435)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x04045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x00034279)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					return add_rasterizer(render_0c002435_04045119_00034279_0c26180f);	/* carnevil */
			}
		}
	}


	info = add_rasterizer((tmus == 1) ? generic_render_1tmu : generic_render_2tmu);
	info->is_generic = TRUE;
	return info;
}

/*
    WG3dh:

    816782: 0C000035 00000000 00045119 000B4779 082410DF
    629976: 0C000035 00000000 00045119 000B4779 0824109F
    497958: 0C000035 00000000 00045119 000B4779 0824101F
    141069: 0C000035 00000000 00045119 000B4779 082418DF
*/

#define NUM_TMUS			1

#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x00045119
#define FBZMODE				0x000b4779
#define TEXTUREMODE1		0x00000000

#define RENDERFUNC			render_0c000035_00045119_000b4779_0824101f
#define TEXTUREMODE0		0x0824101f
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_0824109f
#define TEXTUREMODE0		0x0824109f
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_082410df
#define TEXTUREMODE0		0x082410df
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_082418df
#define TEXTUREMODE0		0x082418df
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef TEXTUREMODE1

#undef NUM_TMUS


/*

    mace:

000000001173E00C: 0C000035 00000000 00045119 000B4779 082418DF (done)
000000000D3EB6D7: 0C600C09 00000000 00045119 000B4779 0824100F
0000000003E4A1D5: 0C600C09 00000000 00045119 000B4779 0824180F
0000000003AAEA07: 0C600C09 00000000 00045119 000B4779 082418CF
000000000389D5A8: 0C480035 00000000 00045119 000B4779 082418DF
000000000168ED9C: 0C480035 00000000 00045119 000B4379 082418DF
000000000142E146: 08602401 00000000 00045119 000B4779 082418DF

*/

#define NUM_TMUS			1

#define FBZCOLORPATH		0x0c600c09
#define ALPHAMODE			0x00045119
#define FBZMODE				0x000b4779
#define TEXTUREMODE1		0x00000000

#define RENDERFUNC			render_0c600c09_00045119_000b4779_0824100f
#define TEXTUREMODE0		0x0824100f
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c600c09_00045119_000b4779_0824180f
#define TEXTUREMODE0		0x0824180f
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c600c09_00045119_000b4779_082418cf
#define TEXTUREMODE0		0x082418cf
#include "voodrast.h"
#undef TEXTUREMODE0
#undef RENDERFUNC

#undef FBZCOLORPATH
#undef FBZMODE
#define FBZCOLORPATH		0x0c480035
#define TEXTUREMODE0		0x082418df

#define FBZMODE				0x000b4779
#define RENDERFUNC			render_0c480035_00045119_000b4779_082418df
#include "voodrast.h"
#undef FBZMODE
#undef RENDERFUNC

#define FBZMODE				0x000b4379
#define RENDERFUNC			render_0c480035_00045119_000b4379_082418df
#include "voodrast.h"
#undef FBZMODE
#undef RENDERFUNC

#undef TEXTUREMODE0
#undef RENDERFUNC
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef TEXTUREMODE1

#undef NUM_TMUS


/*
    blitz99:

389BA0CA: 0C000035 00000000 00040400 000B4739 0C26180F 00000000 00000000
0667BB5A: 0C582C35 00000000 00515110 000B4739 0C26180F 00000000 00000000
0661E0A1: 0C000035 00000000 64040409 000B4739 0C26180F 00000000 00000000
048488C4: 0C002C35 00000000 64515119 000B4799 0C26180F 00000000 00000000
044A750D: 0C582C35 00000000 00515110 000B4739 0C2618CF 00000000 00000000
04351781: 0C002C35 00000000 40515119 000B4739 0C26180F 00000000 00000000
02A984D0: 0C002C35 00000000 40515119 000B47F9 0C26180F 00000000 00000000
0121D0A8: 0D422439 00000000 00040400 000B473B 0C2610C9 00000000 00000000

0000000039CEEE06: 0C000035 00000001 00040400 000B4739 0C26180F
0000000011F145DD: 0C582C35 00000000 00515110 000B4739 0C2618CF
000000000DEAA542: 0C582C35 00000000 00515110 000B4739 0C26180F
000000000C8D034E: 0C002C35 00000000 00515110 000B47F9 0C26180F
0000000005599D25: 0C000035 00000001 64040409 000B4739 0C26180F
00000000043FA611: 0C000035 00000000 00040400 000B4739 0C26180F
000000000422A43F: 0C002C35 00000001 40515119 000B4739 0C26180F
0000000003959E1E: 0C002C35 00000001 64515119 000B4799 0C26180F
000000000347D228: 0C000035 00000001 00040400 000B47F9 0C26180F
00000000033A5BC8: 0C002C35 00000001 40515119 000B47F9 0C26180F
0000000002F8A88F: 0D422439 00000000 00040400 000B473B 0C2610C9
*/

#define NUM_TMUS			1

#define RENDERFUNC			render_0c000035_00040400_000b4739_0c26180f
#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x00040400
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c582c35_00515110_000b4739_0c26180f
#define FBZCOLORPATH		0x0c582c35
#define ALPHAMODE			0x00515110
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_64040409_000b4739_0c26180f
#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x64040409
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c002c35_64515119_000b4799_0c26180f
#define FBZCOLORPATH		0x0c002c35
#define ALPHAMODE			0x64515119
#define FBZMODE				0x000b4799
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c582c35_00515110_000b4739_0c2618cf
#define FBZCOLORPATH		0x0c582c35
#define ALPHAMODE			0x00515110
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c2618cf
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c002c35_40515119_000b4739_0c26180f
#define FBZCOLORPATH		0x0c002c35
#define ALPHAMODE			0x40515119
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#undef NUM_TMUS




/*
    carnevil:

*  13356036   200: 0C002435 04045119 00000000 00034279 0C261A0F 00000000 (97)
*   3054901 140990: 0C002425 00045119 00000000 00034679 0C261A0F 00000000 (97)

*/

#define NUM_TMUS			1

#define RENDERFUNC			render_0c002435_04045119_00034279_0c26180f
#define FBZCOLORPATH		0x0c002435
#define ALPHAMODE			0x04045119
#define FBZMODE				0x00034279
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c002425_00045119_00034679_0c26180f
#define FBZCOLORPATH		0x0c002425
#define ALPHAMODE			0x00045119
#define FBZMODE				0x00034679
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodrast.h"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#undef NUM_TMUS


/*
    Sfrush:

One of these is bad:
       175: 0C000035 00000000 00045119 000B4779 082418DF 00000000 00000000
       175: 0C480035 00000000 00045119 000B4779 082418DF 00000000 00000000
       703: 0C000035 00000000 00045119 000B477B 082410DB 00000000 00000000

       108: 0C600C09 00000001 00045119 000B4779 0824101F 0824101F 00000000
       688: 0C600C09 00000001 00045119 000B4779 00000000 00000000 00000000
         1: 0C600C09 00000001 00045119 000B4779 0824101F 082410DF 00000000
        47: 0C600C09 00000001 00045119 000B4779 082410DF 0824101F 00000000
        53: 0C600C09 00000001 00045119 000B4779 082418DF 0824101F 00000000
OK      41: 0C482435 00000001 00045119 000B4379 0824101F 0824101F 00000000
       225: 0C600C09 00000001 00045119 000B4779 0824101F 0824181F 00000000
        18: 0C600C09 00000001 00045119 000B4779 0824181F 0824181F 00000000
        13: 0C600C09 00000001 00045119 000B4779 082418DF 0824181F 00000000
        23: 0C000035 00000001 00045119 000B4779 082418DF 0824181F 00000000
         9: 0C000035 00000001 00045119 000B477B 082410DB 0824181F 00000000
       463: 0C000035 00000001 00045119 000B4779 082410DF 0824181F 00000000
         1: 0C480035 00000001 00045119 000B4779 082418DF 0824181F 00000000

       127: 0C000035 00000000 00045119 000B4779 082410DF 0824181F 00000000
        31: 0C480035 00000000 00045119 000B4779 082418DF 0824181F 00000000
       127: 0C000035 00000000 00045119 000B477B 082410DB 0824181F 00000000

-----------------------------------------------------------

       511: 00000002 00000000 00000000 00000300 00000000
         1: 08000001 00000000 00000000 00000300 00000800
         1: 08000001 00000000 00000000 00000200 08241800
     32353: 0C000035 00000000 00045119 000B4779 082418DF
      5437: 0C480035 00000000 00045119 000B4779 082418DF
     23867: 0C000035 00000000 00045119 000B477B 082410DB
     10655: 0C600C09 00000001 00045119 000B4779 0824101F
     13057: 0C600C09 00000001 00045119 000B4779 00000000
       949: 0C600C09 00000001 00045119 000B4779 082410DF
      2723: 0C600C09 00000001 00045119 000B4779 082418DF
       240: 0C482435 00000001 00045119 000B4379 0824101F
      4166: 0C600C09 00000001 00045119 000B4779 0824181F
       747: 0C000035 00000001 00045119 000B4779 082418DF
       427: 0C000035 00000001 00045119 000B477B 082410DB
     12063: 0C000035 00000001 00045119 000B4779 082410DF
        93: 0C480035 00000001 00045119 000B4779 082418DF
      3949: 0C000035 00000000 00045119 000B4779 082410DF
    470768: 0C600C09 00000000 00045119 000B4779 00000000
    418032: 0C600C09 00000000 00045119 000B4779 0824101F
    130673: 0C600C09 00000000 00045119 000B4779 0824181F
      1857: 0C480035 00000000 00045119 000B477B 00000000
      1891: 0C480035 00000000 00045119 000B4779 082410DF
     92962: 0C482435 00000000 00045119 000B4379 0824101F
    119123: 0C600C09 00000000 00045119 000B4779 082708DF
     33176: 0C600C09 00000000 00045119 000B4779 082418DF
     44448: 0C600C09 00000000 00045119 000B4779 082700DF
      1937: 0C600C09 00000000 00045119 000B4779 0827001F
     36352: 0C600C09 00000000 00045119 000B4779 082410DF
       328: 0C600C09 00000001 00045119 000B4779 082700DF
       659: 0C600C09 00000001 00045119 000B4779 082708DF
        67: 0C480035 00000000 00045119 000B4779 00000000

*/


/*

Carnevil:
0000000002B4E96A: 0C002425 00000000 00045119 00034679 0C26180F 00000000 00000000
0000000002885479: 0C002435 00000000 04045119 00034279 0C26180F 00000000 00000000
0000000001EE2400: 0C480015 00000000 0F045119 000346F9 0C2618C9 00000000 00000000
0000000001B92D31: 0D422439 00000000 00040400 000B4739 243210C9 00000000 00000000
000000000186F400: 0C000035 00000000 0A045119 000346F9 0C2618C9 00000000 00000000
00000000013C93EE: 0C482415 00000000 0A045119 000346F9 0C26180F 00000000 00000000
000000000139CF3C: 0C482415 00000000 40045119 00034679 0C2618C9 00000000 00000000
00000000013697FC: 0C486116 00000000 01045119 00034279 0C26180F 00000000 00000000
0000000000E4DE5A: 0C482415 00000000 0F045119 000346F9 0C2618C9 00000000 00000000
0000000000DF385B: 0C482415 00000000 04045119 00034279 0C26180F 00000000 00000000
0000000000D02EB3: 0D422409 00000000 00045119 00034679 0C26180F 00000000 00000000

00000000004F8328: 0C002435 00000000 40045119 000B4779 0C26180F 00000000 00000000
000000000000EDA8: 0C002435 00000000 40045119 000B43F9 0C26180F 00000000 00000000
000000000005B736: 0C002435 00000000 40045119 000342F9 0C26180F 00000000 00000000
0000000000A7E85E: 0D422439 00000000 00040400 000B473B 0C2610C9 00000000 00000000
000000000010DCC4: 0D420039 00000000 00040400 000B473B 0C2610C9 00000000 00000000
00000000006525EC: 0C002435 00000000 08045119 000346F9 0C26180F 00000000 00000000
00000000003EFE00: 0C000035 00000000 08045119 000346F9 0C2618C9 00000000 00000000
0000000000086AE4: 0C482415 00000000 05045119 00034679 0C26180F 00000000 00000000
000000000000B1EE: 0C482405 00000000 00045119 00034679 0C26180F 00000000 00000000
00000000003D9446: 0C482415 00000000 04045119 00034279 0C2618C9 00000000 00000000
0000000000BD3AB0: 0C480015 00000000 04045119 000346F9 0C2618C9 00000000 00000000
0000000000001027: 0542611A 00000000 00515119 00034679 0C26180F 00000000 00000000
00000000002C1360: 0C000035 00000000 04045119 00034679 0C2618C9 00000000 00000000
000000000007BAB0: 0C002435 00000000 04045119 00034679 0C2618C9 00000000 00000000
0000000000005360: 0C002425 00000000 04045119 00034679 0C26180F 00000000 00000000
00000000000190D8: 0C482415 00000000 10045119 00034679 0C2618C9 00000000 00000000
0000000000470C50: 0C482415 00000000 05045119 00034279 0C2618C9 00000000 00000000
00000000000A2800: 0C000035 00000000 04040409 00034679 0C2618C9 00000000 00000000
000000000001E814: 0C002435 00000000 04045119 00034279 0C2618C9 00000000 00000000
0000000000146C20: 0C000035 00000000 00045119 00034679 0C2618C9 00000000 00000000
00000000000B6B00: 0C002435 00000000 00045119 00034679 0C2618C9 00000000 00000000
000000000002A05E: 0C482415 00000000 05045119 000346F9 0C26180F 00000000 00000000
00000000001E1C98: 0C002435 00000000 40045119 00034679 0C26180F 00000000 00000000
00000000000198CA: 0C002435 00000000 40045119 000346F9 0C26180F 00000000 00000000

*/

#undef TEXTUREMODE0_MASK
#undef TEXTUREMODE1_MASK
#undef FBZMODE_MASK
#undef ALPHAMODE_MASK
#undef FBZCOLORPATH_MASK


