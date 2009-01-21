//ref: http://2xpm.tripod.com/files/2xpmv02_fusion_src.zip/kegafusion_2xpm/2xPM_LQ.cpp
//---------------------------------------------------------------------------------------------------------------------------
// 2xPM_LQ plugin for Kega Fusion - Pablo Medina (aka "pm") (pjmedina3@yahoo.com)
// Check for updated versions at: http://2xpm.freeservers.com
//---------------------------------------------------------------------------------------------------------------------------

#define ENHANCE_COLORS 0
#define LQ_ALPHA_BLEND 0
#define SHOW_CHANGES 0

unsigned short int pg_red_mask;
unsigned short int pg_green_mask;
unsigned short int pg_blue_mask;
unsigned short int pg_lbmask;

#define RED_MASK565   0xF800
#define GREEN_MASK565 0x07E0
#define BLUE_MASK565  0x001F

#define RED_MASK555 0x7C00
#define GREEN_MASK555 0x03E0
#define BLUE_MASK555 0x001F

#define PG_LBMASK565 0xF7DE
#define PG_LBMASK555 0x7BDE
	
#if LQ_ALPHA_BLEND
#define ALPHA_BLEND_128_W(dst, src) dst = (src + dst) >> 1
#else
#define ALPHA_BLEND_128_W(dst, src) dst = ((src & pg_lbmask) >> 1) + ((dst & pg_lbmask) >> 1)
#endif


//---------------------------------------------------------------------------------------------------------------------------

static unsigned short int PBASE[32];

void _2xpmlq_555(void *SrcPtr, void *DstPtr, unsigned long SrcPitch, unsigned long DstPitch, unsigned long SrcW, unsigned long SrcH, int depth)
{
	unsigned long x, y;
	unsigned char *src, *dest;			
	unsigned short int PA, PB, PC, PD, PE, PF, PG, PH, PI;
	register unsigned short int *start_addr1, *start_addr2, *start_addr3;
	unsigned long next_line, next_line_src;	
	unsigned short int *dst_pixel;
	unsigned long src_width, src_height;
	unsigned long complete_line_src, complete_line_dst;	
	unsigned char auto_blend;
	unsigned short int E[4];
	unsigned long src_pitch;
	unsigned char pprev;	
	unsigned char dont_reblit;

#if ENHANCE_COLORS
	unsigned short int PAX, PDX, PGX, PCX, PFX, PIX;
#endif

/*
	pg_red_mask = RED_MASK565;
	pg_green_mask = GREEN_MASK565;
	pg_blue_mask = BLUE_MASK565;
	pg_lbmask = PG_LBMASK565;
*/

	pg_red_mask = RED_MASK555;
	pg_green_mask = GREEN_MASK555;
	pg_blue_mask = BLUE_MASK555;
	pg_lbmask = PG_LBMASK555;

	src = (unsigned char *)SrcPtr;	
	dest = (unsigned char *)DstPtr;
	src_pitch = SrcPitch;

	next_line_src = src_pitch >> 1;
	next_line = DstPitch >> 1;				
	
	src_width = SrcW;
	src_height = SrcH;

	complete_line_src = next_line_src - SrcW;
	complete_line_dst = DstPitch - (SrcW << 1);
	
	start_addr2 = (unsigned short int *)(src - 2);
	start_addr1 = start_addr2;
	start_addr3 = start_addr2 + src_pitch;

	dst_pixel = (unsigned short int *)(dest);	

	y = src_height - 1;
	//for (y = 0; y < src_height; y++)
	do
	{	
		//if (y == src_height - 1)
		if (!y)
			start_addr3 = start_addr2;		
		auto_blend = 0;
		pprev = 1;
		x = src_width - 1;
		
		//for (x = 0; x < src_width; x++)
		do
		{
			PB = start_addr1[1];
			PE = start_addr2[1];			
			PH = start_addr3[1];
			
			PA = start_addr1[pprev];
			PD = start_addr2[pprev];			
			PG = start_addr3[pprev];

			//if (x < src_width - 1)
			if (x)
			{
				PC = start_addr1[2];
				PF = start_addr2[2];
				PI = start_addr3[2];
			} else {
				PC = PB;
				PF = PE;
				PI = PH;
			}

#if SHOW_CHANGES
			unsigned short int PDE = PE;
			PE = 0;
#endif
			E[0] = E[1] = E[2] = E[3] = PE;

			
			dont_reblit = 0;
						
			// Horizontal
			if (!dont_reblit)						
			if (PD != PF)
			{
				if ((PE != PD) && (PD == PH) && (PD == PI) && (PE != PG)
					&& ((PD != PG) || (PE != PF) || (PA != PD))
					&& (!((PD == PA) && (PD == PG) && (PE == PB) && (PE == PF)))
					)
				{				
					E[2] = PH;
					ALPHA_BLEND_128_W(E[3], PH);					
					dont_reblit = 1;
				}
						
				else if ((PE != PF) && (PF == PH) && (PF == PG) && (PE != PI)
					&& ((PF != PI) || (PE != PD) || (PC != PF))
					&& (!((PF == PC) && (PF == PI) && (PE == PB) && (PE == PD)))
					)
				{
					ALPHA_BLEND_128_W(E[2], PH);				
					E[3] = PH;	
					dont_reblit = 1;
				}			
			}		
			
			if (!dont_reblit)
			{				
				if ((PB != PH) && (PD != PF))
				{						
					if ((PB == PD) && (PE != PD)
						&& (!((PE == PF) && (((PE == PA) && (PB == PC))
							             || ((PD == PA) && (PD == PC) && (PG != PD) && (PG != PE)))))
						&& (!((PB == PA) && (PB == PG))))					
						ALPHA_BLEND_128_W(E[0], PB);
					
					else if ((PB == PF) && (PE != PF)		
						&& (!((PE == PD) && (((PE == PC) && (PB == PA))
						                 || ((PF == PA) && (PF == PC) && (PE == PD) && (PI != PF) && (PI != PE)))))
						&& (!((PB == PC) && (PB == PI)))						
						)
						ALPHA_BLEND_128_W(E[1], PB);
									
					if ((PH == PD) && ((PE != PG) || (PE != PD) || ((PE != PF) && (PF == PI)))
						&& (!((PE == PF) && (((PE == PG) && (PH == PI))
						                 || ((PD == PG) && (PD == PI) && (PE == PF) && (PA != PD) && (PA != PE))
										 || ((PE == PG) && (PH == PI)))))						
						&& (!((PH == PG) && (PH == PA)))											
						)									
						ALPHA_BLEND_128_W(E[2], PH);					
				
					else if ((PH == PF) && ((PE != PI) || (PE != PF))
						&& (!((PE == PD) && (((PE == PI) && (PH == PG))
						                 || ((PF == PG) && (PF == PI) && (PC != PF) && (PI != PE)))))
						&& (!((PH == PI) && (PH == PC)))
						)
						ALPHA_BLEND_128_W(E[3], PH);									
				} else				
#if SHOW_CHANGES
				PE = PDE;
#endif
				if ((PD == PB) && (PD == PF) && (PD == PH) && (PD != PE))					
				{					
					if ((PD == PG) || (PD == PC))
					{						
						ALPHA_BLEND_128_W(E[1], PD);
						E[2] = E[1];						
					}
					if ((PD == PA) || (PD == PI))
					{					
						ALPHA_BLEND_128_W(E[0], PD);
						E[3] = E[0];						
					}
				}
			}			
			
			dst_pixel[0] = E[0];
			dst_pixel[1] = E[1];
			dst_pixel[next_line] = E[2];
			dst_pixel[next_line + 1] = E[3];
		
			start_addr1++;
			start_addr2++;
			start_addr3++;				
			
			dst_pixel += 2;
			pprev = 0;		
		} while (x--);

		start_addr2 += complete_line_src;
		start_addr1 = start_addr2 - next_line_src;
		start_addr3 = start_addr2 + next_line_src;
		dst_pixel += complete_line_dst;				
	} while(y--);
}

//---------------------------------------------------------------------------------------------------------------------------
