/* JPM MPS1/2 Hardware

  TMS9995 CPU

  +

  ???

  AY8910?

*/

#include "emu.h"
#include "cpu/tms9900/tms9900.h"
#include "sound/ay8910.h"

class jpms80_state : public driver_device
{
public:
	jpms80_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }
};

static ADDRESS_MAP_START( jpms80_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x3000, 0x3fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( jpms80_io_map, AS_IO, 8 )

ADDRESS_MAP_END


static INPUT_PORTS_START( jpms80 )
INPUT_PORTS_END


static const ay8910_interface ay8910_interface_jpm =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


// these are wrong
#define MAIN_CLOCK 2000000
#define SOUND_CLOCK 2000000

static MACHINE_CONFIG_START( jpms80, jpms80_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", TMS9995, MAIN_CLOCK)
	MCFG_CPU_PROGRAM_MAP(jpms80_map)
	MCFG_CPU_IO_MAP(jpms80_io_map)

	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("aysnd", AY8910, 2000000)
	MCFG_SOUND_CONFIG(ay8910_interface_jpm)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

MACHINE_CONFIG_END

DRIVER_INIT( jpms80 )
{

}





ROM_START( j80bac )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bankacoinp1.bin", 0x0000, 0x1000, CRC(7b82025b) SHA1(f698688c55f8c5dc891e470de8df2eb12f6b1ec5) )
	ROM_LOAD( "bankacoinp2.bin", 0x1000, 0x1000, CRC(91d71fbe) SHA1(d0c45218b7568d5293f015334d7d1045bcb2fe03) )
	ROM_LOAD( "bankacoinp3.bin", 0x2000, 0x1000, CRC(0c3b2954) SHA1(4342a2a047496caf8569d4519dd8daad47e634e3) )
ROM_END



ROM_START( j80bounc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "bouncer.p1", 0x0000, 0x1000, CRC(81de115b) SHA1(0890de1492859c58411fd130ecf721df7611247a) )
	ROM_LOAD( "bouncer.p2", 0x1000, 0x1000, CRC(8507ea42) SHA1(e4838fe737c8a9964e0067be460e8bfc18b0a406) )
ROM_END



ROM_START( j80frogh )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "froghop1.bin", 0x0000, 0x1000, CRC(606846f8) SHA1(8796fb647a41dad087b9eb3e24fa7071c933d1ec) )
	ROM_LOAD( "froghop2.bin", 0x1000, 0x1000, CRC(b64ed5ad) SHA1(5697b0a16191ee3845f0f4077cf7b597f0b20024) )
	ROM_LOAD( "froghop3.bin", 0x2000, 0x1000, CRC(f5b55c0e) SHA1(9fdef9f634f9b832a1bf6e3e3890a7fa328d20e3) )
ROM_END



ROM_START( j80fruit )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fruit_snappa_1-1.bin", 0x0000, 0x1000, CRC(f6eea72d) SHA1(ae994f9eb68aa6ea127586afb448cc8fbff0c314) )
	ROM_LOAD( "fruit_snappa_1-2.bin", 0x1000, 0x1000, CRC(10eccac5) SHA1(3c9cc57a3b51fdae713c11a33677555be3f669bc) )
	ROM_LOAD( "fruit_snappa_1-3.bin", 0x2000, 0x1000, CRC(6f938a9a) SHA1(edbf44ae7cb060420b6f952652f08271c4af35bd) )
ROM_END



ROM_START( j80golds )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "goldstep2-1.p1", 0x0000, 0x1000, CRC(bc1e0788) SHA1(5e01881bda22fc00b2d2ac2b80acc67caddea682) )
	ROM_LOAD( "goldstep2-2.p2", 0x1000, 0x1000, CRC(6ea82bd9) SHA1(289c9a076b9e5039f09283d64ceb77dfd7ea79ea) )
ROM_END



ROM_START( j80hotln )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "lines2_1.rom", 0x0000, 0x1000, CRC(f0ce5d7f) SHA1(be3f8ff3f83737a004d6a78cc61c3385307df1c3) )
	ROM_LOAD( "lines2_2.rom", 0x1000, 0x1000, CRC(d5e69b49) SHA1(fcaa527875f81e03c5a5866d6d8b017450c50d9c) )
ROM_END



ROM_START( j80myspn )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ms1.bin", 0x0000, 0x1000, CRC(b247374e) SHA1(33399f39bba68eff13e05529174d17f5b1ca0f70) )
	ROM_LOAD( "ms2.bin", 0x1000, 0x1000, CRC(721c35df) SHA1(05ea0cdc83823f268becc7b9dd99db61949ad229) )
ROM_END



ROM_START( j80nudg2 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ndu.p1", 0x0000, 0x1000, CRC(4cfd3c6f) SHA1(06ad825343178a694585ee3b4ff8400caf15dd21) )
ROM_END



ROM_START( j80rr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jpmroadrunnerp1.bin", 0x0000, 0x1000, CRC(86f50997) SHA1(8bb266274d3ebeee942e5f878f7faae012712382) )
	ROM_LOAD( "jpmroadrunnerp2.bin", 0x1000, 0x1000, CRC(aea12b9e) SHA1(6f6eb286c43a9bc04bfcab71713ce59da61cc063) )
	ROM_LOAD( "jpmroadrunnerp3.bin", 0x2000, 0x1000, CRC(9b0b6fb9) SHA1(0282189e2945e4aa3a338930666d1eb34022894c) )
ROM_END


ROM_START( j80rra )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "rr.p1", 0x0000, 0x1000, CRC(38cd5043) SHA1(f4b828ad2e761bba91336714357a18f10d79c22b) )
	ROM_LOAD( "rr.p2", 0x1000, 0x1000, CRC(81dc46ec) SHA1(17c60590cf5628df6bd109213a3f671b1a6df14b) )
	ROM_LOAD( "rr.p3", 0x2000, 0x1000, CRC(5e617600) SHA1(1a2a25f81818fc3abeceb74608b2ffd53fac2c6d) )
ROM_END



ROM_START( j80r66 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "route66p1seta.bin", 0x0000, 0x1000, CRC(86f50997) SHA1(8bb266274d3ebeee942e5f878f7faae012712382) )
	ROM_LOAD( "route66p2seta.bin", 0x1000, 0x1000, CRC(aea12b9e) SHA1(6f6eb286c43a9bc04bfcab71713ce59da61cc063) )
	ROM_LOAD( "route66p3seta.bin", 0x2000, 0x1000, CRC(9b0b6fb9) SHA1(0282189e2945e4aa3a338930666d1eb34022894c) )
ROM_END



ROM_START( j80supst )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "supasteppa2-1.p1", 0x0000, 0x1000, CRC(aac5b165) SHA1(5bf4acb85be227e1f4979fea4552fa5f64e9b7b2) )
	ROM_LOAD( "supasteppa2-2.p2", 0x1000, 0x1000, CRC(3a93ea9e) SHA1(24e711a398d7f071fb904993ff0a974b4ac8b1d6) )
ROM_END



ROM_START( j80supbk )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "sbank-4.1.bin", 0x0000, 0x1000, CRC(effd29fa) SHA1(1e20bc6130f5d49db3856c56c64746f3fa49bd9c) )
	ROM_LOAD( "sbank-4.2.bin", 0x1000, 0x1000, CRC(6ca5cc1d) SHA1(77d9bb44e6837027b61286f30bcb2c1b0e6a53fb) )
	ROM_LOAD( "sbank-4.3.bin", 0x2000, 0x1000, CRC(af08594d) SHA1(ebff60e63e99af102874f4b3f070d9bfd229ab89) )
ROM_END



ROM_START( j80topsp )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "topsprint4-1.p1", 0x0000, 0x1000, CRC(91c4f494) SHA1(e4fd688a1fd23694c4fe8529d07ac248f262ad70) )
	ROM_LOAD( "topsprint4-2.p2", 0x1000, 0x1000, CRC(e9ad3706) SHA1(bb6cb1a8ea740be017055e4fa621fabc8df77086) )
	ROM_LOAD( "topsprint4-3.p3", 0x2000, 0x1000, CRC(d1abfb54) SHA1(33b11563c6e1ddfaa5527ad7a384fecd03c7de0a) )
ROM_END



ROM_START( j80topup )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "topup3-1.bin", 0x0000, 0x1000, CRC(2feb37e8) SHA1(098671f81fa94b851a8fa41ee7bd3d1b762eb824) )
	ROM_LOAD( "topup3-2.bin", 0x1000, 0x1000, CRC(1937e7c9) SHA1(a9ae5163e560642598ec9878276d8785c28eb035) )
	ROM_LOAD( "topup3-3.bin", 0x2000, 0x1000, CRC(283d7dd2) SHA1(8246c80c85956a0a3b59d68700319a59b35a5326) )
ROM_END



ROM_START( j80tumbl )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "tumble3-1.bin", 0x0000, 0x1000, CRC(2feb37e8) SHA1(098671f81fa94b851a8fa41ee7bd3d1b762eb824) )
	ROM_LOAD( "tumble3-2.bin", 0x1000, 0x1000, CRC(1937e7c9) SHA1(a9ae5163e560642598ec9878276d8785c28eb035) )
	ROM_LOAD( "tumble3-3.bin", 0x2000, 0x1000, CRC(23789c80) SHA1(6b6ac4e1dc66d5eb399437e87a9e7ee461bee086) )
ROM_END



ROM_START( j80wsprt )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "winsprint.p1", 0x0000, 0x1000, CRC(e440c7bb) SHA1(5ef85a93a6170115c750257ac6c755b18b3114a9) )
	ROM_LOAD( "winsprint.p2", 0x1000, 0x1000, CRC(225674bf) SHA1(d8a15226ff4f7b16f7f1a8dff969585a6b4536fe) )
	ROM_LOAD( "winsprint.p3", 0x2000, 0x1000, CRC(51d11f59) SHA1(756ba5f02c0733d082767cbdaa93105a7d3f31f3) )
ROM_END



GAME(198?, j80bac	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Bank A Coin (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80bounc	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Bouncer (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80frogh	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Frog Hop (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80fruit	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Fruit Snappa (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80golds	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Golden Steppa (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80hotln	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Hot Lines (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80myspn	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Mystery Spin (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80nudg2	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Nudge Double Up MkII (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80rr	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Road Runner (Jpm) (SYSTEM80, set 1)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80rra	,j80rr		,jpms80,jpms80,jpms80,ROT0,   "Jpm","Road Runner (Jpm) (SYSTEM80, set 2)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80r66	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Route 66 (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80supst	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Supa Steppa (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80supbk	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Superbank (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80topsp	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Top Sprint (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80topup	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Top Up (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80tumbl	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Tumble (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
GAME(198?, j80wsprt	,0			,jpms80,jpms80,jpms80,ROT0,   "Jpm","Winsprint (Jpm) (SYSTEM80)",						GAME_NOT_WORKING|GAME_REQUIRES_ARTWORK|GAME_MECHANICAL|GAME_NO_SOUND )
