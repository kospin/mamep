/******************************************************************************

    mamedriv.c

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

    The list of all available drivers. Drivers have to be included here to be
    recognized by the executable.

    To save some typing, we use a hack here. This file is recursively #included
    twice, with different definitions of the DRIVER() macro. The first one
    declares external references to the drivers; the second one builds an array
    storing all the drivers.

******************************************************************************/

#include "driver.h"

#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern game_driver driver_##NAME;
#include "mamemddriv.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#define DRIVER(NAME) &driver_##NAME,
#ifdef HAZEMD
const game_driver * const drivers[] =
#else /*HAZEMD */
const game_driver * const hazemddrivers[] =
#endif /*HAZEMD */
{
#include "mamemddriv.c"
	0	/* end of array */
};

#else	/* DRIVER_RECURSIVE */
	/* Sort this when the main driver is sorted */

	DRIVER( g_12i1 )
	DRIVER( g_4in1 )
	DRIVER( g_10in1 )
	//DRIVER( g_indy )
	DRIVER( g_sbea )
	DRIVER( g_fcr )
	DRIVER( g_iss )
	DRIVER( g_asol )
	DRIVER( g_gcm )
	DRIVER( g_cont )
	DRIVER( g_dhed )
	DRIVER( g_mmax )
	DRIVER( g_mic )
	DRIVER( g_maui )
	DRIVER( g_mica)
	DRIVER( g_mic2 )
	DRIVER( g_mic9 )
	DRIVER( g_micm )
	DRIVER( g_gf2 )
	DRIVER( g_str2 )
	DRIVER( g_lio3 )
	DRIVER( g_rx3 )
	DRIVER( g_dte )
	DRIVER( g_fwbb )
	DRIVER( g_whac )
	DRIVER( g_bugl )
	DRIVER( g_raid )
	DRIVER( g_wbug )
	DRIVER( ssf2ghw )
	DRIVER( g_ssf2 )
	DRIVER( g_quac )
	DRIVER( g_smb2 )
	DRIVER( g_sbub )
	DRIVER( g_toy )
	DRIVER( g_jpa2 )
	DRIVER( g_vec2 )
	DRIVER( g_kawa )
	DRIVER( g_squi )
	DRIVER( g_hard )
	DRIVER( g_ddpl )
	DRIVER( g_gsh )
	DRIVER( g_jp3 )
	DRIVER( g_col )
	DRIVER( g_bloc )
	DRIVER( g_rrsh )
	DRIVER( g_lem )
	DRIVER( g_pirg )
	DRIVER( g_dcap )
	DRIVER( g_fant )
	DRIVER( g_gau4 )
	DRIVER( g_jbok )
	DRIVER( g_dino )
	DRIVER( g_jim2 )
	DRIVER( g_soni )
	DRIVER( g_tf4 )
	DRIVER( g_abat )
	DRIVER( g_wani )
	DRIVER( g_wild )
	DRIVER( g_son2 )
	DRIVER( g_son2a )
	DRIVER( g_son3 )
	DRIVER( g_will )
	DRIVER( g_tje )
	DRIVER( g_anim )
	DRIVER( g_cill )
	DRIVER( g_gley )
	DRIVER( g_rist )
	DRIVER( g_rkni )
	DRIVER( g_srr )
	DRIVER( g_sons )
	DRIVER( g_spl3 )
	DRIVER( g_tazm )
	DRIVER( g_mk )
	DRIVER( g_comx )
	DRIVER( g_coss )
	DRIVER( g_paca )
	DRIVER( g_jimp )
	DRIVER( g_casv )
	DRIVER( g_f98 )
	DRIVER( g_dash )
	DRIVER( g_boog )
	DRIVER( g_sor )
	DRIVER( g_sor2 )
	DRIVER( g_sor3 )
	DRIVER( g_sgp2 )
	DRIVER( g_batf )
	DRIVER( g_boas )
	DRIVER( g_chk2 )
	DRIVER( g_alad )
	DRIVER( g_bean )
	DRIVER( g_blee )
	DRIVER( g_eco2 )
	DRIVER( g_exos )
	DRIVER( g_fatr )
	DRIVER( g_kgs )
	DRIVER( g_kgsr )
	DRIVER( g_gtwi )
	DRIVER( g_garg )
	DRIVER( g_gng )
	DRIVER( g_daim )
	DRIVER( g_hsh )
	DRIVER( g_jp2 )
	DRIVER( g_lgal )
	DRIVER( g_lion )
	DRIVER( g_mazi )
	DRIVER( g_mick )
	DRIVER( g_mman )
	DRIVER( g_pano )
	DRIVER( g_poca )
	DRIVER( g_pmon )
	DRIVER( g_puls )
	DRIVER( g_redz )
	DRIVER( g_sock )
	DRIVER( g_skit )
	DRIVER( g_s3d )
	DRIVER( g_spo2 )
	DRIVER( g_subt )
	DRIVER( g_sfz )
	DRIVER( g_sho )
	DRIVER( g_smgp )
	DRIVER( g_stb )
	DRIVER( g_tta )
	DRIVER( g_tyra )
	DRIVER( g_vf2 )
	DRIVER( g_wizl )
	DRIVER( g_ztks )

	DRIVER( g_popu )
	DRIVER( g_pino )
	DRIVER( g_puyo )
	DRIVER( g_revs )
	DRIVER( g_seaq )
	DRIVER( g_tale )
	DRIVER( g_zool )
	DRIVER( g_aate )
	DRIVER( g_balz )
	DRIVER( g_bsht )
	DRIVER( g_bonk )
	DRIVER( g_mars )

	DRIVER( g_fdma )

	DRIVER( g_3nin )
	DRIVER( g_acro )
	DRIVER( g_acr2 )
	DRIVER( g_haun )
	DRIVER( g_pugg )
	DRIVER( g_sams )
	DRIVER( g_mmf  )
	DRIVER( g_mfli )

	DRIVER( g_skid )
	DRIVER( g_or20 )
	DRIVER( g_rwoo )
	DRIVER( g_rolo )
	DRIVER( g_lot  )
	DRIVER( g_lot2 )
	DRIVER( g_stra )
	DRIVER( g_tinh )
	DRIVER( g_tg2 )
	DRIVER( g_gdog )
	DRIVER( g_gaia )
	DRIVER( g_gods )
	DRIVER( g_gyno )
	DRIVER( g_elem )
	DRIVER( g_mano )
	DRIVER( g_daze )
	DRIVER( g_jwws )
	DRIVER( g_exmu )
	DRIVER( g_busq )
	DRIVER( g_olan )
	DRIVER( g_shar )
	DRIVER( g_pidw )
	DRIVER( g_bbrb )
	DRIVER( g_bbbq )
	DRIVER( g_toki )
	DRIVER( g_mtur )
	DRIVER( g_tanr )
	DRIVER( g_rop )
	DRIVER( g_seni )
	DRIVER( g_ps96 )
	DRIVER( g_toug )
	DRIVER( g_spir )
	DRIVER( g_strk )
	DRIVER( g_wcs2 )
	DRIVER( g_rrs2 )
	DRIVER( g_rrs3 )
	DRIVER( g_zomb )
	DRIVER( g_yuyu )
	DRIVER( g_devi )
	DRIVER( g_alis )
	DRIVER( g_arun )
	DRIVER( g_orur )
	DRIVER( g_orun )
	DRIVER( g_chee )
	DRIVER( g_nbah )
	DRIVER( g_bhb )
	DRIVER( g_vect )
	DRIVER( g_stol )
	DRIVER( g_td2 )
	DRIVER( g_tout )
	DRIVER( g_sprk )
	DRIVER( g_garf )
	DRIVER( g_dfry )
	DRIVER( g_drev )

	DRIVER( g_afam )
	DRIVER( g_afav )
	DRIVER( g_abus )
	DRIVER( g_ali3 )
	DRIVER( g_arcu )
	DRIVER( g_arie )
	DRIVER( g_arro )
	DRIVER( g_asgr )
	DRIVER( g_aspg )
	DRIVER( g_awep )

	DRIVER( g_bob )
	DRIVER( g_blma )
	DRIVER( g_body )
	DRIVER( g_drac )
	DRIVER( g_brpw )
	DRIVER( g_buba )
	DRIVER( g_bubs )
	DRIVER( g_bub2 )
	DRIVER( g_bbny )
	DRIVER( g_cano )
	DRIVER( g_cpoo )
	DRIVER( g_chao )
	DRIVER( g_chq2 )
	DRIVER( g_cool )
	DRIVER( g_cybo )
	DRIVER( g_dicv )
	DRIVER( g_ejim )
	DRIVER( g_taz2 )
	DRIVER( g_fand )
	DRIVER( g_fanda )

	DRIVER( g_fbak )
	DRIVER( g_huni )
	DRIVER( g_hell )
	DRIVER( g_hook )
	DRIVER( g_huma )
	DRIVER( g_jpra )
	DRIVER( g_ksfh )
	DRIVER( g_land )
	DRIVER( g_mhat )
	DRIVER( g_mtla )
	DRIVER( g_nutz )
	DRIVER( g_ooze )
	DRIVER( g_ootw )
	DRIVER( g_page )
	DRIVER( g_2040 )
	DRIVER( g_ppin )
	DRIVER( g_ranx )
	DRIVER( g_shi3 )
	DRIVER( g_krew )
	DRIVER( g_sks3 )
	DRIVER( g_skik )
	DRIVER( g_sylv )
	DRIVER( g_term )
	DRIVER( g_tje2 )
	DRIVER( g_uqix )
	DRIVER( g_wolv )

	DRIVER( g_real )
	DRIVER( g_abz2 )
	DRIVER( g_abu2 )
	DRIVER( g_aqua )
	DRIVER( g_suj2 )
	DRIVER( g_batr )
	DRIVER( g_btoa )
	DRIVER( g_che2 )
	DRIVER( g_che )
	DRIVER( g_chuk )
	DRIVER( g_clay )
	DRIVER( g_crue )
	DRIVER( g_daff )
	DRIVER( g_davi )
	DRIVER( g_desd )
	DRIVER( g_dstr )
	DRIVER( g_djby )
	DRIVER( g_dtro )
	DRIVER( g_ecco )
	DRIVER( g_e_sw )
	DRIVER( g_etch )
	DRIVER( g_f1ce )
	DRIVER( g_flic )
	DRIVER( g_gax3 )
	DRIVER( g_gran )
	DRIVER( g_hurr )
	DRIVER( g_izzy )
	DRIVER( g_bond )
	DRIVER( g_jp )
	DRIVER( g_jstr )
	DRIVER( g_kidc )
	DRIVER( g_lawn )
	DRIVER( g_lem2 )
	DRIVER( g_lost )
	DRIVER( g_marv )
	DRIVER( g_megp )
	DRIVER( g_mmpm )
	DRIVER( g_mk2 )
	DRIVER( g_mk3 )
	DRIVER( g_nh98 )
	DRIVER( g_otti )
	DRIVER( g_pst3 )
	DRIVER( g_pink )
	DRIVER( g_pitf )
	DRIVER( g_prin )
	DRIVER( g_radr )
	DRIVER( g_sks2 )
	DRIVER( g_s_sa )
	DRIVER( g_spl2 )
	DRIVER( g_stri )
	DRIVER( g_turt )
	DRIVER( g_tick )
	DRIVER( g_tomj )
	DRIVER( g_umk3 )
	DRIVER( g_uded )
	DRIVER( g_ustr )
	DRIVER( g_muth )
	DRIVER( g_alex )
	DRIVER( g_btdd )
	DRIVER( g_chak )
	DRIVER( g_blav )
	DRIVER( g_budo )
	DRIVER( g_clif )
	DRIVER( g_col3 )
	DRIVER( g_cutt )
	DRIVER( g_dang )
	DRIVER( g_dar2 )
	DRIVER( g_duel )
	DRIVER( g_demo )
	DRIVER( g_dick )
	DRIVER( g_dora )
	DRIVER( g_dn3d )
	DRIVER( g_elvi )
	DRIVER( g_faf2 )
	DRIVER( g_faf )
	DRIVER( g_fzon )
	DRIVER( g_flin )
	DRIVER( g_genc )
	DRIVER( g_glos )
	DRIVER( g_gbus )
	DRIVER( g_hoso )
	DRIVER( g_home )
	DRIVER( g_hom2 )
	DRIVER( g_immo )
	DRIVER( g_icd )
	DRIVER( g_hulk )
	DRIVER( g_junc )
	DRIVER( g_jpar )
	DRIVER( g_ligh )
	DRIVER( g_fran )
	DRIVER( g_mfpl )
	DRIVER( g_onsl )
	DRIVER( g_peng )
	DRIVER( g_rain )
	DRIVER( g_rens )
	DRIVER( g_robt )
	DRIVER( g_sitd )
	DRIVER( g_btnm )
	DRIVER( g_smf )
	DRIVER( g_smf2 )
	DRIVER( g_snkn )
	DRIVER( g_s_mc )
	DRIVER( g_sgat )
	DRIVER( g_tfh )
	DRIVER( g_tutf )
	DRIVER( g_tf3 )
	DRIVER( g_ttaa )
	DRIVER( g_tter )
	DRIVER( g_unis )
	DRIVER( g_vrtr )
	DRIVER( g_wayn )
	DRIVER( g_weap )
	DRIVER( g_wmar )
	DRIVER( g_xme2 )
	DRIVER( g_yogi )
	DRIVER( g_soff )
	DRIVER( g_ddr )
	DRIVER( g_ddrv )
	DRIVER( g_ddr3 )
	DRIVER( g_ddr2 )
	DRIVER( g_mb8p )
	DRIVER( g_fido )
	DRIVER( g_xper )
	DRIVER( g_sscc )
	DRIVER( g_tnnb )
	DRIVER( g_tnno )

	DRIVER( g_batj )
	DRIVER( g_bat )
	DRIVER( g_bsqu )
	DRIVER( g_crkd )
	DRIVER( g_dune )
	DRIVER( g_earn )
	DRIVER( g_f117 )
	DRIVER( g_frog )
	DRIVER( g_gloc )
	DRIVER( g_gax2 )
	DRIVER( g_gshi )
	DRIVER( g_herz )
	DRIVER( g_last )
	DRIVER( g_mush )
	DRIVER( g_mjmw )
	DRIVER( g_mmpr )
	DRIVER( g_mpac )
	DRIVER( g_ncir )
	DRIVER( g_ogol )
	DRIVER( g_pacm )
	DRIVER( g_pdri )
	DRIVER( g_race )
	DRIVER( g_sbe2 )
	DRIVER( g_shaq )
	DRIVER( g_bart )
	DRIVER( g_sold )
	DRIVER( g_si91 )
	DRIVER( g_s_ar )
	DRIVER( g_ssri )
	DRIVER( g_sstv )
	DRIVER( g_targ )
	DRIVER( g_ter2 )
	DRIVER( g_tf2 )
	DRIVER( g_toys )
	DRIVER( g_true )
	DRIVER( g_uwnh )
	DRIVER( g_view )
	DRIVER( g_wagh )
	DRIVER( g_worm )
	DRIVER( g_xen2 )
	DRIVER( g_zany )
	DRIVER( g_zoop )
	DRIVER( g_arta )
	DRIVER( g_asl )
	DRIVER( g_arca )
	DRIVER( g_aahh )
	DRIVER( g_aero )
	DRIVER( g_awsp )
	DRIVER( g_bttf )
	DRIVER( g_bar3 )
	DRIVER( g_suj )
	DRIVER( g_beav )
	DRIVER( g_botb )
	DRIVER( g_sail )
	DRIVER( g_bl96 )
	DRIVER( g_buck )
	DRIVER( g_curs )
	DRIVER( g_hb95 )
	DRIVER( g_ws98 )
	DRIVER( g_dark )
	DRIVER( g_4081 )
	DRIVER( g_dmov )
	DRIVER( g_drs )
	DRIVER( g_hire )
	DRIVER( g_dblc )
	DRIVER( g_elim )
	DRIVER( g_eswa )
	DRIVER( g_ecs )
	DRIVER( g_must )
	DRIVER( g_gax )
	DRIVER( g_goof )
	DRIVER( g_grin )
	DRIVER( g_dodg )
	DRIVER( g_indl )
	DRIVER( g_jcte )
	DRIVER( g_jdre )
	DRIVER( g_jltf )
	DRIVER( g_kout )
	DRIVER( g_taru )
	DRIVER( g_osom )
	DRIVER( g_puy2 )
	DRIVER( g_scob )
	DRIVER( g_srun )
	DRIVER( g_snow )
	DRIVER( g_spb2 )
	DRIVER( g_s_as )
	DRIVER( g_s_kp )
	DRIVER( g_semp )
	DRIVER( g_sair )
	DRIVER( g_sbat )
	DRIVER( g_tprk )
	DRIVER( g_tint )
	DRIVER( g_tsht )
	DRIVER( g_gomo )
	DRIVER( g_usoc )
	DRIVER( g_wfrr )
	DRIVER( g_wfra )
	DRIVER( g_wolf )
	DRIVER( g_zoom )
	DRIVER( g_resq )
	DRIVER( g_jely )
	DRIVER( g_itch )
	DRIVER( g_2sam )

	DRIVER( g_pre2 )
	DRIVER( g_pst2 )
	DRIVER( g_pst4 )
	DRIVER( g_pga3 )
	DRIVER( g_pga2 )
	DRIVER( g_pga )
	DRIVER( g_pg96 )
	DRIVER( g_pm97 )
	DRIVER( g_pman )
	DRIVER( g_prim )
	DRIVER( g_pwbl )
	DRIVER( g_puni )
	DRIVER( g_ram3 )
	DRIVER( g_revx )
	DRIVER( g_rbls )
	DRIVER( g_rrr )
	DRIVER( g_rth2 )
	DRIVER( g_rth3 )
	DRIVER( g_snsm )
	DRIVER( g_sdan )
	DRIVER( g_s2de )
	DRIVER( g_shf1 )
	DRIVER( g_shf2 )
	DRIVER( g_sole )
	DRIVER( g_sspa )
	DRIVER( g_sf2c )
	DRIVER( g_sbt )
	DRIVER( g_2020 )
	DRIVER( g_f22i )
	DRIVER( g_faer )
	DRIVER( g_gain )
	DRIVER( g_0tol )
	DRIVER( g_zwin )
	DRIVER( g_yidy )
	DRIVER( g_wher )
	DRIVER( g_whip )
	DRIVER( g_ward )
	DRIVER( g_vbar )
	DRIVER( g_2cd )
	DRIVER( g_twih )
	DRIVER( g_twic )
	DRIVER( g_turr )
	DRIVER( g_trux )
	DRIVER( g_tp96 )
	DRIVER( g_tpgo )
	DRIVER( g_todd )
	DRIVER( g_tkil )
	DRIVER( g_tfox )
	DRIVER( g_tetr )
	DRIVER( g_synd )
	DRIVER( g_pins )
	DRIVER( g_pifg )
	DRIVER( g_pac2 )
	DRIVER( g_nbd )
	DRIVER( g_ko3 )
	DRIVER( g_kgk )
	DRIVER( g_ksal )
	DRIVER( g_lvsc )
	DRIVER( g_wbdt )
	DRIVER( g_comc )
	DRIVER( g_688a )
	DRIVER( g_adiv )
	DRIVER( g_asto )
	DRIVER( g_abea )
	DRIVER( g_agla )
	DRIVER( g_arch )
	DRIVER( g_aptg )
	DRIVER( g_aof )
	DRIVER( g_akid )
	DRIVER( g_atpt )
	DRIVER( g_arug )
	DRIVER( g_bjak )
	DRIVER( g_barb )
	DRIVER( g_bass )
	DRIVER( g_basp )
	DRIVER( g_barn )
	DRIVER( g_btl2 )
	DRIVER( g_btlm )
	DRIVER( g_btms )
	DRIVER( g_btec )
	DRIVER( g_bwre )
	DRIVER( g_bear )
	DRIVER( g_bw95 )
	DRIVER( g_bwcf )
	DRIVER( g_bimi )
	DRIVER( g_bnza )
	DRIVER( g_bwbw )
	DRIVER( g_boxl )
	DRIVER( g_bh95 )
	DRIVER( g_blcr )
	DRIVER( g_bvsb )
	DRIVER( g_bvsl )
	DRIVER( g_burf )
	DRIVER( g_cada )
	DRIVER( g_caes )
	DRIVER( g_crjb )
	DRIVER( g_c50 )
	DRIVER( g_cgam )
	DRIVER( g_capa )
	DRIVER( g_capp )
	DRIVER( g_chik )
	DRIVER( g_clue )
	DRIVER( g_corp )
	DRIVER( g_cuty )
	DRIVER( g_ddwe )
	DRIVER( g_dlnd )
	DRIVER( g_dduk )
	DRIVER( g_ecjr )
	DRIVER( g_e_bt )
	DRIVER( g_e_hn )
	DRIVER( g_e_sn )
	DRIVER( g_ehrd )
	DRIVER( g_f15s )
	DRIVER( g_feud )
	DRIVER( g_fatl )
	DRIVER( g_frgp )
	DRIVER( g_fifa )
	DRIVER( g_fi95 )
	DRIVER( g_fi96 )
	DRIVER( g_fi97 )
	DRIVER( g_gfko )
	DRIVER( g_gste )
	DRIVER( g_grwl )
	DRIVER( g_hnov )
	DRIVER( g_hice )
	DRIVER( g_jnpg )
	DRIVER( g_ktm2 )
	DRIVER( g_ktm )
	DRIVER( g_klax )
	DRIVER( g_len )
	DRIVER( g_len2 )
	DRIVER( g_mamr )
	DRIVER( g_mamo )
	DRIVER( g_mbmb )
	DRIVER( g_njte )
	DRIVER( g_nl98 )
	DRIVER( g_tnzs )
	DRIVER( g_noes )
	DRIVER( g_norm )
	DRIVER( g_papb )
	DRIVER( g_pap2 )
	DRIVER( g_pop2 )
	DRIVER( g_quad )
	DRIVER( g_risk )
	DRIVER( g_rise )
	DRIVER( g_shov )
	DRIVER( g_side )
	DRIVER( g_std9 )
	DRIVER( g_stng )
	DRIVER( g_t2ar )
	DRIVER( g_vali )
	DRIVER( g_val3 )
	DRIVER( g_vpin )
	DRIVER( g_wimb )
	DRIVER( g_nl97 )
	DRIVER( g_nl96 )
	DRIVER( g_nl95 )
	DRIVER( g_njam )
	DRIVER( g_nasc )
	DRIVER( g_nact )
	DRIVER( g_na95 )
	DRIVER( g_npbb )
	DRIVER( g_nba9 )
	DRIVER( g_npbl )
	DRIVER( g_nbs9 )
	DRIVER( g_nccf )
	DRIVER( g_ncff )
	DRIVER( g_nfl8 )
	DRIVER( g_nfl5 )
	DRIVER( g_nfl4 )
	DRIVER( g_nfpt )
	DRIVER( g_nqc6 )
	DRIVER( g_nqc )
	DRIVER( g_nh94 )
	DRIVER( g_nh95 )
	DRIVER( g_nh9e )
	DRIVER( g_nh96 )
	DRIVER( g_nh97 )
	DRIVER( g_nash )
	DRIVER( g_nhlh )
	DRIVER( g_nhlp )
	DRIVER( g_olsg )
	DRIVER( g_olwg )
	DRIVER( g_patb )
	DRIVER( g_pele )
	DRIVER( g_pelw )
	DRIVER( g_pst )
	DRIVER( g_pgae )
	DRIVER( g_rb3 )
	DRIVER( g_rb4 )
	DRIVER( g_rb93 )
	DRIVER( g_rb94 )
	DRIVER( g_rcvm )
	DRIVER( g_rw93 )
	DRIVER( g_swcr )
	DRIVER( g_shi )
	DRIVER( g_srba )
	DRIVER( g_svol )
	DRIVER( g_virr )
	DRIVER( g_ws96 )
	DRIVER( g_ws95 )
	DRIVER( g_wsb )
	DRIVER( g_wts )
	DRIVER( g_wfsw )
	DRIVER( g_ys3 )
	DRIVER( g_ma95 )
	DRIVER( g_ma96 )
	DRIVER( g_ma97 )
	DRIVER( g_ma98 )
	DRIVER( g_ma94 )
	DRIVER( g_ma93 )
	DRIVER( g_ma92 )
	DRIVER( g_ma3c )
	DRIVER( g_ma )
	DRIVER( g_jmof )
	DRIVER( g_jms2 )
	DRIVER( g_jms )
	DRIVER( g_hb94 )
	DRIVER( g_hb3 )
	DRIVER( g_hb )

	DRIVER( g_coak )
	DRIVER( g_cf96 )
	DRIVER( g_cf97 )
	DRIVER( g_cfn )
	DRIVER( g_cfn2 )
	DRIVER( g_csla )
	DRIVER( g_cfir )
	DRIVER( g_cc )
	DRIVER( g_crys )
	DRIVER( g_cybb )
	DRIVER( g_ccop )
	DRIVER( g_dcat )
	DRIVER( g_drbb )
	DRIVER( g_drsc )
	DRIVER( g_dcmd )
	DRIVER( g_dc3d )
	DRIVER( g_drib )
	DRIVER( g_dbzj )
	DRIVER( g_dbzf )
	DRIVER( g_deye )
	DRIVER( g_eaho )
	DRIVER( g_exil )
	DRIVER( g_f1c )
	DRIVER( g_f1gp )
	DRIVER( g_f1h )
	DRIVER( g_f1sl )
	DRIVER( g_f1wc )
	DRIVER( g_fas1 )
	DRIVER( g_fatm )
	DRIVER( g_fmas )
	DRIVER( g_fblo )
	DRIVER( g_fpro )
	DRIVER( g_fshk )
	DRIVER( g_fore )
	DRIVER( g_fw )
	DRIVER( g_fung )
	DRIVER( g_geng )
	DRIVER( g_gws )
	DRIVER( g_hydu )
	DRIVER( g_imgi )
	DRIVER( g_insx )
	DRIVER( g_intr )
	DRIVER( g_ishi )
	DRIVER( g_jlcs )
	DRIVER( g_jlp2 )
	DRIVER( g_jlpp )
	DRIVER( g_jlp )
	DRIVER( g_jlpf )
	DRIVER( g_jbdb )
	DRIVER( g_jamm )
	DRIVER( g_jano )
	DRIVER( g_jant )
	DRIVER( g_jeop )
	DRIVER( g_jeod )
	DRIVER( g_jeos )
	DRIVER( g_jgpf )
	DRIVER( g_jewl )
	DRIVER( g_jmac )
	DRIVER( g_jb11 )
	DRIVER( g_kbox )
	DRIVER( g_kbou )
	DRIVER( g_lbat )
	DRIVER( g_lhx )
	DRIVER( g_msbu )
	DRIVER( g_mhy )
	DRIVER( g_mmad )
	DRIVER( g_mlem )
	DRIVER( g_mowe )
	DRIVER( g_math )
	DRIVER( g_mlo )
	DRIVER( g_mloj )
	DRIVER( g_mswi )
	DRIVER( g_mtrx )
	DRIVER( g_merc )
	DRIVER( g_mult )
	DRIVER( g_midr )
	DRIVER( g_m29 )
	DRIVER( g_mmg )
	DRIVER( g_mike )
	DRIVER( g_mlbb )
	DRIVER( g_mlbs )
	DRIVER( g_mono )
	DRIVER( g_mahb )
	DRIVER( g_mysd )
	DRIVER( g_mysf )
	DRIVER( g_nhir )
	DRIVER( g_nobu )
	DRIVER( g_opeu )
	DRIVER( g_pto )
	DRIVER( g_pbgl )
	DRIVER( g_phel )
	DRIVER( g_path )
	DRIVER( g_prho )
	DRIVER( g_prqu )
	DRIVER( g_ramp )
	DRIVER( g_rast )
	DRIVER( g_rsbt )
	DRIVER( g_rmmw )
	DRIVER( g_robw )
	DRIVER( g_r3k2 )
	DRIVER( g_r3k3 )
	DRIVER( g_sswo )
	DRIVER( g_sen )
	DRIVER( g_sbls )
	DRIVER( g_sdnk )
	DRIVER( g_sspo )
	DRIVER( g_sks1 )
	DRIVER( g_scon )
	DRIVER( g_sfli )
	DRIVER( g_ssma )
	DRIVER( g_sumc )
	DRIVER( g_shyd )
	DRIVER( g_sl )
	DRIVER( g_sl91 )
	DRIVER( g_smas )
	DRIVER( g_supm )
	DRIVER( g_swso )
	DRIVER( g_swve )
	DRIVER( g_tusa )
	DRIVER( g_tcls )
	DRIVER( g_tcop )
	DRIVER( g_tc )
	DRIVER( g_tsb )
	DRIVER( g_tsbw )
	DRIVER( g_tbw2 )
	DRIVER( g_tbw3 )
	DRIVER( g_tsh )
	DRIVER( g_tsnb )
	DRIVER( g_tw92 )
	DRIVER( g_tw93 )
	DRIVER( g_ttnk )
	DRIVER( g_tlbb )
	DRIVER( g_tr95 )
	DRIVER( g_trbb )
	DRIVER( g_totf )
	DRIVER( g_toxi )
	DRIVER( g_tanf )
	DRIVER( g_uman )
	DRIVER( g_ur95 )
	DRIVER( g_vapt )
	DRIVER( g_v5 )
	DRIVER( g_volf )
	DRIVER( g_wack )
	DRIVER( g_wloc )
	DRIVER( g_wrom )
	DRIVER( g_wars )
	DRIVER( g_wgas )
	DRIVER( g_wfor )
	DRIVER( g_winc )
	DRIVER( g_wb3 )
	DRIVER( g_wbmw )
	DRIVER( g_wcl )

	DRIVER( g_wcs )
	DRIVER( g_wc90 )
	DRIVER( g_wc94 )
	DRIVER( g_wwar )
	DRIVER( g_cwcs )

	DRIVER( g_cbwl )
	DRIVER( g_cpam )
	DRIVER( g_cdor )
	DRIVER( g_ckid )
	DRIVER( g_chi )
	DRIVER( g_bglf )
	DRIVER( g_baha )
	DRIVER( g_awld )
	DRIVER( g_airm )
	DRIVER( g_airm2 )
	DRIVER( g_aoki )
	DRIVER( g_advdai )
	DRIVER( g_blueal )
	DRIVER( g_chav2 )
	DRIVER( g_chibi )
	DRIVER( g_crayon )
	DRIVER( g_konsen )
	DRIVER( g_dahna )
	DRIVER( g_daikou )
	DRIVER( g_discol )
	DRIVER( g_dslay )
	DRIVER( g_dslay2 )
	DRIVER( g_draxos )

	DRIVER( g_eadoub )
	DRIVER( g_europa )
	DRIVER( g_exranz )
	DRIVER( g_fushig )
	DRIVER( g_ftbhb )
	DRIVER( g_gameno )
	DRIVER( g_gemfi )
	DRIVER( g_gaunt )
	DRIVER( g_ghwor )
	DRIVER( g_hssoc )
	DRIVER( g_hokuto )
	DRIVER( g_hybrid )
	DRIVER( g_ichir )
	DRIVER( g_hyokk )
	DRIVER( g_janout )
	DRIVER( g_juju )
	DRIVER( g_juuo00 )
	DRIVER( g_juuo01 )
	DRIVER( g_kishi )
	DRIVER( g_kyuuk )
	DRIVER( g_langr )
	DRIVER( g_lngr2a )
	DRIVER( g_lngr2c )
	DRIVER( g_librty )
	DRIVER( g_m1tank )
	DRIVER( g_madou )
	DRIVER( g_mahcop )
	DRIVER( g_maoure )
	DRIVER( g_maten )
	DRIVER( g_manser )
	DRIVER( g_megaq )
	DRIVER( g_mfang )
	DRIVER( g_minnie )
	DRIVER( g_minato )
	DRIVER( g_mpiano )
	DRIVER( g_monwr4 )


	DRIVER( g_mlfoot )
	DRIVER( g_new3dg )
	DRIVER( g_nikkan )
	DRIVER( g_nobbus )
	DRIVER( g_nobzen )
	DRIVER( g_noblor )
	DRIVER( g_patlab )
	DRIVER( g_psyobl )
	DRIVER( g_ragna )
	DRIVER( g_ransei )
	DRIVER( g_renthe )
	DRIVER( g_robo3 )
	DRIVER( g_roybld )
	DRIVER( g_sagia )
	DRIVER( g_sango2 )
	DRIVER( g_sango3 )
	DRIVER( g_sangor )
	DRIVER( g_shikin )
	DRIVER( g_shiten )
	DRIVER( g_shogi )
	DRIVER( g_shura )
	DRIVER( g_slapf )
	DRIVER( g_soldf )
	DRIVER( g_sorckd )
	DRIVER( g_stalon )
	DRIVER( g_sthor )
	DRIVER( g_sthorj )
	DRIVER( g_supdai )
	DRIVER( g_surgin )
	DRIVER( g_sydval )
	DRIVER( g_taiga )
	DRIVER( g_taikou )
	DRIVER( g_teitok )
	DRIVER( g_telmj )
	DRIVER( g_telstd )
	DRIVER( g_tpglf2 )
	DRIVER( g_tpglf )
	DRIVER( g_traysi )
	DRIVER( g_twintl )
	DRIVER( g_uw )
	DRIVER( g_verytx )
	DRIVER( g_vix357 )
	DRIVER( g_waiala )
	DRIVER( g_warps )
	DRIVER( g_wrom2 )
	DRIVER( g_wboy5 )
	DRIVER( g_wonlib )
	DRIVER( g_wresbl )
	DRIVER( g_xdaze )
	DRIVER( g_xmen )
	DRIVER( g_yuyuga )
	DRIVER( g_zanya )
	DRIVER( g_aresha )
	DRIVER( g_asolde )
	DRIVER( g_alisie )
	DRIVER( g_alisij )
	DRIVER( g_adivej )
	DRIVER( g_bdomen )
	DRIVER( g_aofe )
	DRIVER( g_bnza00 )
	DRIVER( g_bnza01 )
	DRIVER( g_astgru )
	DRIVER( g_booge )
	DRIVER( g_cvane )
	DRIVER( g_captha )
	DRIVER( g_chouya )
	DRIVER( g_chikij )
	DRIVER( g_comixe )
	DRIVER( g_comixj )
	DRIVER( g_contrj )
	DRIVER( g_coole )
	DRIVER( g_crying )
	DRIVER( g_alade )
	DRIVER( g_aladj )
	DRIVER( g_ejim2e )
	DRIVER( g_ejime )
	DRIVER( g_dtusa )
	DRIVER( g_beane )
	DRIVER( g_dynab2 )
	DRIVER( g_dheadj )
	DRIVER( g_taz2e )
	DRIVER( g_f15e )
	DRIVER( g_f22j )
	DRIVER( g_f22ua )
	DRIVER( g_f22u )
	DRIVER( g_fever )
	DRIVER( g_fbckua )
	DRIVER( g_fbckj )
	DRIVER( g_fbcke )
	DRIVER( g_gamblr )
	DRIVER( g_garou )
	DRIVER( g_garou2 )
	DRIVER( g_godse )
	DRIVER( g_godsj )
	DRIVER( g_gstenj )
	DRIVER( g_gshe )
	DRIVER( g_gshj )
	DRIVER( g_mickey )
	DRIVER( g_immorj )
	DRIVER( g_jmons2 )
	DRIVER( g_madj )
	DRIVER( g_jbooke )
	DRIVER( g_jstrj )
	DRIVER( g_kingcj )
	DRIVER( g_klaxj )
	DRIVER( g_kujaku )
	DRIVER( g_ksfh00 )
	DRIVER( g_kuuga )
	DRIVER( g_kyuuky )
	DRIVER( g_landsj )
	DRIVER( g_landse )
	DRIVER( g_landsg )
	DRIVER( g_lem2e )
	DRIVER( g_leme )
	DRIVER( g_lem00 )
	DRIVER( g_lenfe )
	DRIVER( g_lenfj )
	DRIVER( g_lighfr )
	DRIVER( g_lordmo )
	DRIVER( g_mmftbe )
	DRIVER( g_marsue )
	DRIVER( g_mazij )
	DRIVER( g_mbmba )
	DRIVER( g_mturre )
	DRIVER( g_mwlk00 )
	DRIVER( g_mmprme )
	DRIVER( g_mmpre )
	DRIVER( g_mk3e )
	DRIVER( g_mk00 )
	DRIVER( g_mushaj )
	DRIVER( g_nbahte )
	DRIVER( g_nbajj )
	DRIVER( g_nbaj00 )
	DRIVER( g_nbajt0 )
	DRIVER( g_nigel )
	DRIVER( g_oozee )
	DRIVER( g_2019e )
	DRIVER( g_2019j )
	DRIVER( g_orunj )
	DRIVER( g_orunrj )
	DRIVER( g_pstr4j )
	DRIVER( g_pstr2j )
	DRIVER( g_pstr3j )
	DRIVER( g_2040e )
	DRIVER( g_pitfe )
	DRIVER( g_pifia )
	DRIVER( g_pocae )
	DRIVER( g_popue )
	DRIVER( g_popuj )
	DRIVER( g_pmonj )
	DRIVER( g_probot )
	DRIVER( g_ppina )
	DRIVER( g_puy200 )
	DRIVER( g_renste )
	DRIVER( g_rrsh2j )
	DRIVER( g_rrsh22 )
	DRIVER( g_runark )
	DRIVER( g_ryuuko )
	DRIVER( g_samshe )
	DRIVER( g_samspi )
	DRIVER( g_shin3e )
	DRIVER( g_sidepe )
	DRIVER( g_bart00 )
	DRIVER( g_soleif )
	DRIVER( g_soleig )
	DRIVER( g_sonse )
	DRIVER( g_sonsj )
	DRIVER( g_son200 )
	DRIVER( g_son3e )
	DRIVER( g_son3j )
	DRIVER( g_soni00 )
	DRIVER( g_sorcer )
	DRIVER( g_sfbob )
	DRIVER( g_si90 )
	DRIVER( g_sf2e )
	DRIVER( g_sf2j )
	DRIVER( g_sor2je )
	DRIVER( g_sor3e )
	DRIVER( g_sor3ea )
	DRIVER( g_sor00 )
	DRIVER( g_strid2 )
	DRIVER( g_suphq )
	DRIVER( g_ssf2e )
	DRIVER( g_ssf2j )
	DRIVER( g_talmit )
	DRIVER( g_terme )
	DRIVER( g_tf2md )
	DRIVER( g_tf4e )
	DRIVER( g_tje00 )
	DRIVER( g_tje2e )
	DRIVER( g_tje2g )
	DRIVER( g_tje2j )
	DRIVER( g_toyste )
	DRIVER( g_umk3e )
	DRIVER( g_vamkil )
	DRIVER( g_wardj )
	DRIVER( g_wwics )
	DRIVER( g_wtics )
	DRIVER( g_wticsa )
	DRIVER( g_whipj )
	DRIVER( g_winwor )
	DRIVER( g_wizlie )
	DRIVER( g_zombe )
	DRIVER( g_zoole )
	DRIVER( g_zoope )
	DRIVER( g_zouzou )
	DRIVER( g_ztkse )
	DRIVER( g_007 )
	DRIVER( g_abate )
	DRIVER( g_aerobl )
	DRIVER( g_acro2e )
	DRIVER( g_abrn2j )
	DRIVER( g_alexkk )
	DRIVER( g_alxkeb )
	DRIVER( g_alexke )
	DRIVER( g_alexkj )
	DRIVER( g_ali300 )
	DRIVER( g_aatee )
	DRIVER( g_anime )
	DRIVER( g_arcusj )
	DRIVER( g_arrowj )
	DRIVER( g_robokj )
	DRIVER( g_arunre )
	DRIVER( g_smgp2a )
	DRIVER( g_bttfe )
	DRIVER( g_batmnj )
	DRIVER( g_beaswr )
	DRIVER( g_beave )
	DRIVER( g_drace )
	DRIVER( g_bubbae )
	DRIVER( g_budoe )
	DRIVER( g_burnfj )
	DRIVER( g_chelno )
	DRIVER( g_chk2e )
	DRIVER( g_chk2j )
	DRIVER( g_chkrke )
	DRIVER( g_col00 )
	DRIVER( g_col3j )
	DRIVER( g_crkde )
	DRIVER( g_crkdj )
	DRIVER( g_crudeb )
	DRIVER( g_cruej )
	DRIVER( g_dinolj )
	DRIVER( g_aladb )
	DRIVER( g_djboye )
	DRIVER( g_djboyj )
	DRIVER( g_ddukea )
	DRIVER( g_ecco2e )
	DRIVER( g_ecco2j )
	DRIVER( g_ecjr01 )
	DRIVER( g_eccodj )
	DRIVER( g_elvinj )
	DRIVER( g_elemj )
	DRIVER( g_eswatj )
	DRIVER( g_echmpe )
	DRIVER( g_echmpj )
	DRIVER( g_yuyub )
	DRIVER( g_ys3j )
	DRIVER( g_xzr )
	DRIVER( g_xmene )
	DRIVER( g_wfwaal )
	DRIVER( g_wormp )
	DRIVER( g_wille )
	DRIVER( g_wheroj )
	DRIVER( g_wcs00 )
	DRIVER( g_wimbp )
	DRIVER( g_wimbe )
	DRIVER( g_virraj )
	DRIVER( g_virrae )
	DRIVER( g_vectp )
	DRIVER( g_vect2p )
	DRIVER( g_valsdj )
	DRIVER( g_val3j )
	DRIVER( g_valj )
	DRIVER( g_uzuke )

	DRIVER( g_f1wlce )
	DRIVER( g_f22b )
	DRIVER( g_f117j )
	DRIVER( g_fante )
	DRIVER( g_fant00 )
	DRIVER( g_ferias )
	DRIVER( g_fi99r )
	DRIVER( g_fghmsj )
	DRIVER( g_fshrke )
	DRIVER( g_fshrku )
	DRIVER( g_flinte )
	DRIVER( g_flintj )
	DRIVER( g_fw00 )
	DRIVER( g_ggrouj )
	DRIVER( g_gfkobe )
	DRIVER( g_gbus00 )
	DRIVER( g_gng01 )
	DRIVER( g_gax00 )
	DRIVER( g_gran00 )
	DRIVER( g_ghwj )
	DRIVER( g_gshsam )
	DRIVER( g_gynoge )
	DRIVER( g_helfij )
	DRIVER( g_herzoj )
	DRIVER( g_hybrip )
	DRIVER( g_hdunkj )
	DRIVER( g_indlce )
	DRIVER( g_insxj )
	DRIVER( g_jlps00 )
	DRIVER( g_jp2j )
	DRIVER( g_jewlj )
	DRIVER( g_jb11j )
	DRIVER( g_jb1100 )
	DRIVER( g_jparke )
	DRIVER( g_jparkj )
	DRIVER( g_kageki )
	DRIVER( g_kotme )
	DRIVER( g_ksalmj )
	DRIVER( g_lth2j )
	DRIVER( g_lhxj )
	DRIVER( g_licrue )
	DRIVER( g_licruj )
	DRIVER( g_licruk )
	DRIVER( g_marvj )
	DRIVER( g_momonj )
	DRIVER( g_mazie )
	DRIVER( g_mcdtj )
	DRIVER( g_micmaj )
	DRIVER( g_mmanie )
	DRIVER( g_mmanij )
	DRIVER( g_micm2e )
	DRIVER( g_midrej )
	DRIVER( g_mig29e )
	DRIVER( g_mdpfua )
	DRIVER( g_mutlfj )
	DRIVER( g_mysd00 )
	DRIVER( g_nfl94j )
	DRIVER( g_olgole )
	DRIVER( g_olgolj )
	DRIVER( g_olgolu )
	DRIVER( g_olwge )
	DRIVER( g_olwgj )
	DRIVER( g_sswoj )
	DRIVER( g_samex3 )
	DRIVER( g_shbeaj )
	DRIVER( g_shrunj )
	DRIVER( g_shinch )
	DRIVER( g_shdrkj )
	DRIVER( g_shfrcj )
	DRIVER( g_shfr2j )
	DRIVER( g_shfr2e )
	DRIVER( g_shdrkb )
	DRIVER( g_showd )
	DRIVER( g_showd2 )
	DRIVER( g_shwd2a )
	DRIVER( g_sokoba )
	DRIVER( g_soni2p )
	DRIVER( g_sorkin )
	DRIVER( g_shar2j )
	DRIVER( g_sparke )
	DRIVER( g_sparkj )
	DRIVER( g_sbal2e )
	DRIVER( g_shou2e )
	DRIVER( g_shou3j )
	DRIVER( g_spot2e )
	DRIVER( g_starcj )
	DRIVER( g_stng00 )
	DRIVER( g_strf00 )
	DRIVER( g_stalj )
	DRIVER( g_slordj )
	DRIVER( g_stridj )
	DRIVER( g_subte )
	DRIVER( g_subtj )
	DRIVER( g_ssride )
	DRIVER( g_2020j )
	DRIVER( g_sfze )
	DRIVER( g_sho00 )
	DRIVER( g_shyde )
	DRIVER( g_shydj )
	DRIVER( g_smgp03 )
	DRIVER( g_smgp00 )
	DRIVER( g_smgp01 )
	DRIVER( g_srealj )
	DRIVER( g_sshin2 )
	DRIVER( g_stb00 )
	DRIVER( g_svolua )
	DRIVER( g_supmne )
	DRIVER( g_swsoj )
	DRIVER( g_swvej )
	DRIVER( g_pachin )
	DRIVER( g_pageme )
	DRIVER( g_pboyj )
	DRIVER( g_pblbej )
	DRIVER( g_pste03 )
	DRIVER( g_pgat01 )
	DRIVER( g_pga2j )
	DRIVER( g_pga200 )
	DRIVER( g_phst2b )
	DRIVER( g_phst3b )
	DRIVER( g_phelie )
	DRIVER( g_phelij )
	DRIVER( g_puggse )
	DRIVER( g_quac00 )
	DRIVER( g_quac01 )
	DRIVER( g_ram300 )
	DRIVER( g_ranxe )
	DRIVER( g_rastj )
	DRIVER( g_rshi00 )
	DRIVER( g_rshi01 )
	DRIVER( g_rshi02 )
	DRIVER( g_ristj )
	DRIVER( g_roadbj )
	DRIVER( g_rnrre )
	DRIVER( g_rkadve )
	DRIVER( g_rkadvj )
	DRIVER( g_rkmnja )
	DRIVER( g_rthn2e )
	DRIVER( g_rthn2j )
	DRIVER( g_ron98b )
	DRIVER( g_tfhxj )
	DRIVER( g_tsbwlj )
	DRIVER( g_tsbwlu )
	DRIVER( g_tsbw2j )
	DRIVER( g_tsnbaj )
	DRIVER( g_tmntj )
	DRIVER( g_tmnttj )
	DRIVER( g_telerb )
	DRIVER( g_tfoxj )
	DRIVER( g_tdom1 )
	DRIVER( g_tkille )
	DRIVER( g_ttabe )
	DRIVER( g_toddj )
	DRIVER( g_tajua )
	DRIVER( g_turma )
	DRIVER( g_clascl )
	DRIVER( g_gen6pk )
	DRIVER( g_mg2e )
	DRIVER( g_mg3e )
	DRIVER( g_mg6v1e )
	DRIVER( g_mg6v2e )
	DRIVER( g_mg6v3e )
	DRIVER( g_mg10i1 )
	DRIVER( g_mg1e )
	DRIVER( g_menace )
	DRIVER( g_segsp1 )
	DRIVER( g_stop5b )
	DRIVER( g_soncle )
	DRIVER( g_sonclu )
	DRIVER( g_sptgb )
	DRIVER( g_16tile )
	DRIVER( g_777cas )
	DRIVER( g_adamb )
	DRIVER( g_mmaxe )
	DRIVER( g_aateb )
	DRIVER( g_atpte )
	DRIVER( g_awsep )
	DRIVER( g_bobb )
	DRIVER( g_babyb1 )
	DRIVER( g_babyb2 )
	DRIVER( g_barbvb )
	DRIVER( g_bk2b )
	DRIVER( g_bk3b )
	DRIVER( g_batme )
	DRIVER( g_beavib )
	DRIVER( g_botbb )
	DRIVER( g_bzerot )
	DRIVER( g_biohzb )
	DRIVER( g_blam2b )
	DRIVER( g_blocb2 )
	DRIVER( g_bcounb )
	DRIVER( g_blcrb )
	DRIVER( g_brutle )
	DRIVER( g_bubbab )
	DRIVER( g_burnfe )
	DRIVER( g_caeno )
	DRIVER( g_caeno2 )
	DRIVER( g_capamb )
	DRIVER( g_capame )
	DRIVER( g_cappb )
	DRIVER( g_cvtngb )
	DRIVER( g_cengb )
	DRIVER( g_chk2b )
	DRIVER( g_chuck )
	DRIVER( g_clifhb )
	DRIVER( g_clifhe )
	DRIVER( g_comacb )
	DRIVER( g_comixb )
	DRIVER( g_congob )
	DRIVER( g_spotb )
	DRIVER( g_cyjusb )
	DRIVER( g_daffyb )
	DRIVER( g_daik2 )
	DRIVER( g_dari2a )
	DRIVER( g_dashb )
	DRIVER( g_dwctb1 )
	DRIVER( g_dwctb2 )
	DRIVER( g_dwctb )
	DRIVER( g_dazeb )
	DRIVER( g_demomb )
	DRIVER( g_dmahtw )
	DRIVER( g_dialq )
	DRIVER( g_domino )
	DRIVER( g_dominu )
	DRIVER( g_beanb )
	DRIVER( g_drgble )
	DRIVER( g_duneg )
	DRIVER( g_dunee )
	DRIVER( g_dynabr )
	DRIVER( g_dheadb )
	DRIVER( g_ecco2b )
	DRIVER( g_elfwor )
	DRIVER( g_e_nhb )
	DRIVER( g_etchmb )
	DRIVER( g_exosb )
	DRIVER( g_exrnzb )
	DRIVER( g_f1wceb )
	DRIVER( g_f15b )
	DRIVER( g_fatfue )
	DRIVER( g_fengsh )
	DRIVER( g_fergpb )
	DRIVER( g_funnge )
	DRIVER( g_fut98 )
	DRIVER( g_glf200 )
	DRIVER( g_gaun4a )
	DRIVER( g_ghunt )
	DRIVER( g_glocb )
	DRIVER( g_godsb )
	DRIVER( g_gax2b )
	DRIVER( g_helfie )
	DRIVER( g_herc2 )
	DRIVER( g_homeab )
	DRIVER( g_hdunkb )
	DRIVER( g_micdo )
	DRIVER( g_icdb )
	DRIVER( g_iraq03 )
	DRIVER( g_icftd )
	DRIVER( g_jbondw )
	DRIVER( g_jdrdb )
	DRIVER( g_jdrdba )
	DRIVER( g_jstrkb )
	DRIVER( g_junkb )
	DRIVER( g_jparkb )
	DRIVER( g_kawab )
	DRIVER( g_landsb )
	DRIVER( g_landsf )
	DRIVER( g_lng201 )
	DRIVER( g_linkdr )
	DRIVER( g_lostvb )
	DRIVER( g_lostve )
	DRIVER( g_lotu2b )
	DRIVER( g_mmfb )
	DRIVER( g_mazib )
	DRIVER( g_mcdtlb )
	DRIVER( g_mcdtle )
	DRIVER( g_mlom01 )
	DRIVER( g_mswive )
	DRIVER( g_megme )
	DRIVER( g_micmab )
	DRIVER( g_mmanib )
	DRIVER( g_micm96 )
	DRIVER( g_micmc )
	DRIVER( g_mima3b )
	DRIVER( g_monob )
	DRIVER( g_mahbb )
	DRIVER( g_mahbe )
	DRIVER( g_nba94b )
	DRIVER( g_nhl96e )
	DRIVER( g_nhlp00 )
	DRIVER( g_nigwce )
	DRIVER( g_ncircb )
	DRIVER( g_ngaidb )
	DRIVER( g_ottifb )
	DRIVER( g_ootwb )
	DRIVER( g_outlnb )
	DRIVER( g_2019b )
	DRIVER( g_pagemb )
	DRIVER( g_pblbee )
	DRIVER( g_pst201 )
	DRIVER( g_pinkb )
	DRIVER( g_pinnoe )
	DRIVER( g_pirdwu )
	DRIVER( g_pgoldb )
	DRIVER( g_pgmah )
	DRIVER( g_persb1 )
	DRIVER( g_persb2 )
	DRIVER( g_perse )
	DRIVER( g_puggsb )
	DRIVER( g_punise )
	DRIVER( g_rrexe )
	DRIVER( g_rbi4b )
	DRIVER( g_renstb )
	DRIVER( g_robtb1 )
	DRIVER( g_robtb2 )
	DRIVER( g_robte )
	DRIVER( g_robwrb )
	DRIVER( g_snsme )
	DRIVER( g_scrabb )
	DRIVER( g_seaqe )
	DRIVER( g_sensib )
	DRIVER( g_shan2b )
	DRIVER( g_sfrcbt )
	DRIVER( g_skrewe )
	DRIVER( g_smous )
	DRIVER( g_soleib )
	DRIVER( g_soleis )
	DRIVER( g_soncrk )
	DRIVER( g_sonsb )
	DRIVER( g_sork00 )
	DRIVER( g_s_asb1 )
	DRIVER( g_s_asb2 )
	DRIVER( g_stds9e )
	DRIVER( g_sgatb )
	DRIVER( g_sempb )
	DRIVER( g_sthorb )
	DRIVER( g_sthorf )
	DRIVER( g_sthorg )
	DRIVER( g_sthork )
	DRIVER( g_sthors )
	DRIVER( g_sf2b )
	DRIVER( g_strikb )
	DRIVER( g_subtb1 )
	DRIVER( g_subtb2 )
	DRIVER( g_sleage )
	DRIVER( g_sshi2b )
	DRIVER( g_skida )
	DRIVER( g_supmnb )
	DRIVER( g_sylvb )
	DRIVER( g_t2arcb )
	DRIVER( g_taiwan )
	DRIVER( g_tmhte )
	DRIVER( g_tmhtte )
	DRIVER( g_ttadae )
	DRIVER( g_tpgola )
	DRIVER( g_twisf )
	DRIVER( g_crudee )
	DRIVER( g_unkch )
	DRIVER( g_vectb )
	DRIVER( g_viewpb )
	DRIVER( g_vf2t2 )
	DRIVER( g_virrea )
	DRIVER( g_wacrac )
	DRIVER( g_warlob )
	DRIVER( g_watrb )
	DRIVER( g_wwcse )
	DRIVER( g_waghe )
	DRIVER( g_wchalb )
	DRIVER( g_wcs02 )
	DRIVER( g_wcs2b )
	DRIVER( g_willb )
	DRIVER( g_wwarb )
	DRIVER( g_yindyb )
	DRIVER( g_yindcb )
	DRIVER( g_zany01 )
	DRIVER( g_zwingj )
	DRIVER( g_zombhb )
	DRIVER( g_16ton )
	DRIVER( g_hymar )
	DRIVER( g_labyd )
	DRIVER( g_padfi )
	DRIVER( g_p2anne )
	DRIVER( g_p2huey )
	DRIVER( g_p2kind )
	DRIVER( g_p2shil )
	DRIVER( g_putter )
	DRIVER( g_pymag )
	DRIVER( g_pymag2 )
	DRIVER( g_pymag3 )
	DRIVER( g_pymags )
	DRIVER( g_serase )
	DRIVER( g_act52 )
	DRIVER( g_act52a )
	DRIVER( g_chaoji )
	DRIVER( g_chess )
	DRIVER( g_maggrl )
	DRIVER( g_mjlovr )
	DRIVER( g_sj6 )
	DRIVER( g_sj6p )
	DRIVER( g_smbro )
	DRIVER( g_alad2 )
	DRIVER( g_barver )
	DRIVER( g_rtk5c )
	DRIVER( g_sanret )
	DRIVER( g_tighun )
	DRIVER( g_princ2 )

	DRIVER( g_megamd )
	DRIVER( g_aworg )
	DRIVER( g_teddy )
	DRIVER( g_robobt )
	DRIVER( g_medalc )
	DRIVER( g_riddle )
	DRIVER( g_kisssh )
	DRIVER( g_rist00 )

	DRIVER( radicav1 )
	DRIVER( radicasf )

	DRIVER( g_bible )
	DRIVER( g_joshua )
	DRIVER( g_exodus )
	DRIVER( g_spirit )
	DRIVER( g_divine )
	DRIVER( g_topfig )
	DRIVER( g_mk5sz )
	DRIVER( g_kof98 )
	DRIVER( g_hercu )
	DRIVER( g_lionk2 )
	DRIVER( g_f2000g )
	DRIVER( g_soulb )
	DRIVER( g_sho3ja )
	DRIVER( g_xinqi )
	DRIVER( g_xinqia )
	DRIVER( g_yang )
	DRIVER( g_yasec )

	DRIVER( g_pockm )
	DRIVER( g_pockma )
	DRIVER( g_pockm2 )
	DRIVER( g_mulan )

	/* Master System Games */
	DRIVER( s_cosmic )
	DRIVER( s_micro )
	DRIVER( s_fantdz )
	DRIVER( s_dinob )
	DRIVER( s_landil )
	DRIVER( s_tazman )
	DRIVER( s_bubbob )
	DRIVER( s_chuck )
	DRIVER( s_chuck2 )
	DRIVER( s_adams )
	DRIVER( s_aburn )
	DRIVER( s_aladin )
	DRIVER( s_alexmi )
	DRIVER( s_alsynd )
	DRIVER( s_alstor )
	DRIVER( s_actfgh )
	DRIVER( s_column )
	DRIVER( s_bean )
	DRIVER( s_fzone )
	DRIVER( s_fzone2 )
	DRIVER( s_fzone3 )
	DRIVER( s_flint )
	DRIVER( s_gng )
	DRIVER( s_wb3dt )
	DRIVER( s_woody )
	DRIVER( s_zool )
	DRIVER( s_smgpa )
	DRIVER( s_sor )
	DRIVER( s_lucky )
	DRIVER( s_lionk )
	DRIVER( s_lemm )
	DRIVER( s_jp2 )
	DRIVER( s_gpride )
	DRIVER( s_jungbk )
	DRIVER( s_gaunt )
	DRIVER( s_castil )
	DRIVER( s_sonic )
	DRIVER( s_sonic2 )
	DRIVER( s_spyspy )
	DRIVER( s_suptet )
	DRIVER( s_supko )
	DRIVER( s_strid )
	DRIVER( s_ssi )
	DRIVER( s_rrsh )
	DRIVER( s_psycho )
	DRIVER( s_tnzs )
	DRIVER( s_20em1 )
	DRIVER( s_aceace )
	DRIVER( s_actfgj )
	DRIVER( s_aerial )
	DRIVER( s_airesc )
	DRIVER( s_aleste )
	DRIVER( s_alexls )
	DRIVER( s_alexbm )
	DRIVER( s_alexht )
	DRIVER( s_alf )

	DRIVER( s_alien3 )
	DRIVER( s_altbea )
	DRIVER( s_ash )
	DRIVER( s_astrx )
	DRIVER( s_astrxa )
	DRIVER( s_astgr )
	DRIVER( s_astsm )
	DRIVER( s_bttf2 )
	DRIVER( s_bttf3 )
	DRIVER( s_baku )

	DRIVER( s_bartsm )
	DRIVER( s_boutr )
	DRIVER( s_calig )
	DRIVER( s_calig2 )
	DRIVER( s_coolsp )
	DRIVER( s_ddux )
	DRIVER( s_legnil )
	DRIVER( s_mspac )
	DRIVER( s_pmania )
	DRIVER( s_rtype )
	DRIVER( s_sensi )
	DRIVER( s_smgp2 )
	DRIVER( s_supoff )
	DRIVER( s_zill )
	DRIVER( s_zill2 )

	/* Game Gear Games */
	DRIVER( gg_exldz )
	DRIVER( gg_bust )
	DRIVER( gg_puzlo )
	DRIVER( gg_puyo2 )
	DRIVER( gg_tempj )
	DRIVER( gg_tess )
	DRIVER( gg_popil )
	DRIVER( gg_nazo )
	DRIVER( gg_nazo2 )
	DRIVER( gg_gear )
	DRIVER( gg_bean )
	DRIVER( gg_cols )
	DRIVER( gg_baku )

	/* System E */
//	DRIVER( hangonjr )
//	DRIVER( transfrm )
//	DRIVER( ridleofp )
//	DRIVER( tetrisse )

	/* for testing only */
//	DRIVER( megatech )
//	DRIVER( mt_astro )
//	DRIVER( topshoot )
//	DRIVER( 32x_knuk )
//	DRIVER( 32x_bios )


#endif	/* DRIVER_RECURSIVE */
