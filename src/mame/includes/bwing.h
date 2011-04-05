/***************************************************************************

    B-Wings

***************************************************************************/

#define BW_DEBUG 0
#define MAX_SOUNDS 16

class bwing_state : public driver_device
{
public:
	bwing_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* memory pointers */
	UINT8 *    m_videoram;
	UINT8 *    m_spriteram;
	UINT8 *    m_paletteram;
	UINT8 *    m_bwp1_sharedram1;
	UINT8 *    m_bwp2_sharedram1;
	UINT8 *    m_bwp3_rombase;
	size_t     m_bwp3_romsize;

	/* video-related */
	tilemap_t *m_charmap;
	tilemap_t *m_fgmap;
	tilemap_t *m_bgmap;
	UINT8 *m_srbase[4];
	UINT8 *m_fgdata;
	UINT8 *m_bgdata;
	int *m_srxlat;
	unsigned m_sreg[8];
	unsigned m_palatch;
	unsigned m_srbank;
	unsigned m_mapmask;
	unsigned m_mapflip;

	/* sound-related */
	UINT8 m_sound_fifo[MAX_SOUNDS];
	int m_bwp3_nmimask;
	int m_bwp3_u8F_d;
	int m_ffcount;
	int m_ffhead;
	int m_fftail;

	/* misc */
	UINT8 *m_bwp123_membase[3];
	int m_coin;

	/* device */
	device_t *m_maincpu;
	device_t *m_subcpu;
	device_t *m_audiocpu;
};


/*----------- defined in video/bwing.c -----------*/

extern const gfx_layout bwing_tilelayout;

WRITE8_HANDLER( bwing_paletteram_w );
WRITE8_HANDLER( bwing_videoram_w );
WRITE8_HANDLER( bwing_spriteram_w );
WRITE8_HANDLER( bwing_scrollreg_w );
WRITE8_HANDLER( bwing_scrollram_w );
READ8_HANDLER( bwing_scrollram_r );

VIDEO_START( bwing );
SCREEN_UPDATE( bwing );
