/*
**	2xMC68000 + Z80
**	YM2151 + Custom PCM
**
**	Out Run
**	Super Hang-On
**	Super Hang-On Limited Edition
**	Turbo Out Run
**
**	Additional infos :
**
**	It has been said that Super Hang On runs on Out Run hardware, this is not exactly the case !
**	Super Hang On use sega video board 171-5480 with the following specifications :
**
**	Sega custom IC on 171-5480
**
**	315-5196, 315-5197 and 315-5242.
**
**	Pals 	: 315-5213
**		: 315-5251
**
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "cpu/m68000/m68000.h"
#include "system16.h"

extern void fd1094_driver_init(void);
extern void fd1094_machine_init(void);


static void set_fg_page( int data ){
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[2] = (data>>4)&0xf;
	sys16_fg_page[3] = data&0xf;
}

static void set_bg_page( int data ){
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[2] = (data>>4)&0xf;
	sys16_bg_page[3] = data&0xf;
}

static void set_fg_page1( int data ){
	sys16_fg_page[1] = data>>12;
	sys16_fg_page[0] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;
}

static void set_bg_page1( int data ){
	sys16_bg_page[1] = data>>12;
	sys16_bg_page[0] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;
}

#if 0
static void set_fg2_page( int data ){
	sys16_fg2_page[0] = data>>12;
	sys16_fg2_page[1] = (data>>8)&0xf;
	sys16_fg2_page[2] = (data>>4)&0xf;
	sys16_fg2_page[3] = data&0xf;
}

static void set_bg2_page( int data ){
	sys16_bg2_page[0] = data>>12;
	sys16_bg2_page[1] = (data>>8)&0xf;
	sys16_bg2_page[2] = (data>>4)&0xf;
	sys16_bg2_page[3] = data&0xf;
}
#endif

/* hang-on's accel/brake are really both analog controls, but I've added them
as digital as well to see what works better */
#define HANGON_DIGITAL_CONTROLS

static READ16_HANDLER( ho_io_x_r ){ return input_port_0_r( offset ); }
#ifdef HANGON_DIGITAL_CONTROLS
static READ16_HANDLER( ho_io_y_r ){
	int data = input_port_1_r( offset );

	switch(data & 3)
	{
		case 3:	return 0xffff;	// both
		case 2:	return 0x00ff;  // brake
		case 1:	return 0xff00;  // accel
		case 0:	return 0x0000;  // neither
	}
	return 0x0000;
}
#else
static READ16_HANDLER( ho_io_y_r ){ return (input_port_1_r( offset ) << 8) + input_port_5_r( offset ); }
#endif

//	outrun: generate_gr_screen(0x200,0x800,0,0,3,0x8000);
static void generate_gr_screen(
	int w,int bitmap_width,int skip,
	int start_color,int end_color, int source_size )
{
	UINT8 *buf = malloc( source_size );
	UINT8 *buf_base = buf;
	if( buf ){
		UINT8 *gr = memory_region(REGION_GFX3);
		UINT8 *grr = NULL;
	    int i,j,k;
	    int center_offset=0;
		sys16_gr_bitmap_width = bitmap_width;

		memcpy(buf,gr,source_size);
		memset(gr,0,256*bitmap_width);

		if (w!=sys16_gr_bitmap_width){
			if (skip>0) // needs mirrored RHS
				grr=gr;
			else {
				center_offset= bitmap_width-w;
				gr+=center_offset/2;
			}
		}

		for (i=0; i<256; i++){ // build gr_bitmap
			UINT8 last_bit;
			UINT8 color_data[4];
			color_data[0]=start_color;
			color_data[1]=start_color+1;
			color_data[2]=start_color+2;
			color_data[3]=start_color+3;

			last_bit = ((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
			for (j=0; j<w/8; j++){
				for (k=0; k<8; k++){
					UINT8 bit=((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
					if (bit!=last_bit && bit==0 && i>1){ // color flipped to 0,advance color[0]
						if (color_data[0]+end_color <= end_color){
							color_data[0]+=end_color;
						}
						else{
							color_data[0]-=end_color;
						}
					}
					*gr++ = color_data[bit];
					last_bit=bit;
					buf[0] <<= 1; buf[0x4000] <<= 1;
				}
				buf++;
			}

			if (grr!=NULL){ // need mirrored RHS
				const UINT8 *temp = gr-1-skip;
				for (j=0; j<w-skip; j++){
					*gr++ = *temp--;
				}
				for (j=0; j<skip; j++) *gr++ = 0;
			}
			else {
				gr+=center_offset;
			}
		}

		i=1;
		while ( (1<<i) < sys16_gr_bitmap_width ) i++;
		sys16_gr_bitmap_width=i; // power of 2
		free(buf_base);
	}
}

static data16_t coinctrl;

static WRITE16_HANDLER( sys16_3d_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x10;
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (0 in fantzone) */

		/* Hang-On, Super Hang-On, Space Harrier, Enduro Racer */
		set_led_status(0,coinctrl & 0x04);

		/* Space Harrier */
		set_led_status(1,coinctrl & 0x08);
	}
}

#if 0
static WRITE16_HANDLER( sound_command_nmi_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}
#endif

static READ16_HANDLER( sys16_coinctrl_r ){
	return coinctrl;
}

#if 0
static WRITE16_HANDLER( sys16_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x20;
		coin_counter_w(0,coinctrl & 0x01);
		set_led_status(0,coinctrl & 0x04);
		set_led_status(1,coinctrl & 0x08);
		/* bit 6 is also used (1 most of the time; 0 in dduxbl, sdi, wb3;
		   tturf has it normally 1 but 0 after coin insertion) */
		/* eswat sets bit 4 */
	}
}
#endif

static INTERRUPT_GEN( sys16_interrupt ){
	if(sys16_custom_irq) sys16_custom_irq();
	cpunum_set_input_line(cpu_getactivecpu(), 4, HOLD_LINE); /* Interrupt vector 4, used by VBlank */
}

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xc0, 0xc0) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
ADDRESS_MAP_END

static data16_t *shared_ram;
static READ16_HANDLER( shared_ram_r ){
	return shared_ram[offset];
}
static WRITE16_HANDLER( shared_ram_w ){
	COMBINE_DATA( &shared_ram[offset] );
}

static unsigned char *sound_shared_ram;
static READ16_HANDLER( sound_shared_ram_r )
{
	return (sound_shared_ram[offset*2] << 8) +
			sound_shared_ram[offset*2+1];
}

static WRITE16_HANDLER( sound_shared_ram_w )
{
	if( ACCESSING_LSB ){
		sound_shared_ram[offset*2+1] = data&0xff;
	}
	if( ACCESSING_MSB ){
		sound_shared_ram[offset*2] = data>>8;
	}
}

static READ8_HANDLER( sound2_shared_ram_r ){
	return sound_shared_ram[offset];
}
static WRITE8_HANDLER( sound2_shared_ram_w ){
	sound_shared_ram[offset] = data;
}


ROM_START( shangonb )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "s-hangon.30", 0x000000, 0x10000, CRC(d95e82fc) SHA1(bc6cd0b0ac98a9c53f2e22ac086521704ab59e4d) )
	ROM_LOAD16_BYTE( "s-hangon.32", 0x000001, 0x10000, CRC(2ee4b4fb) SHA1(ba4042ab6e533c16c3cde848248d75e484be113f) )
	ROM_LOAD16_BYTE( "s-hangon.29", 0x020000, 0x8000, CRC(12ee8716) SHA1(8e798d23d22f85cd046641184d104c17b27995b2) )
	ROM_LOAD16_BYTE( "s-hangon.31", 0x020001, 0x8000, CRC(155e0cfd) SHA1(e51734351c887fe3920c881f57abdfbb7d075f57) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep10652.54",        0x00000, 0x08000, CRC(260286f9) SHA1(dc7c8d2c6ef924a937328685eed19bda1c8b1819) )
	ROM_LOAD( "ep10651.55",        0x08000, 0x08000, CRC(c609ee7b) SHA1(c6dacf81cbfe7e5df1f9a967cf571be1dcf1c429) )
	ROM_LOAD( "ep10650.56",        0x10000, 0x08000, CRC(b236a403) SHA1(af02b8122794c083a66f2ab35d2c73b84b2df0be) )

	ROM_REGION( 0x0120000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ep10675.8",	0x000001, 0x010000, CRC(d6ac012b) SHA1(305023b1a0a9d84cfc081ffc2ad7578b53d562f2) )
	ROM_RELOAD(     		0x100001, 0x010000 )
	ROM_LOAD16_BYTE( "ep10682.16",  0x000000, 0x010000, CRC(d9d83250) SHA1(f8ca3197edcdf53643a5b335c3c044ddc1310cd4) )
	ROM_RELOAD(     		0x100000, 0x010000 )
	ROM_LOAD16_BYTE( "s-hangon.20", 0x020001, 0x010000, CRC(eef23b3d) SHA1(2416fa9991afbdddf25d469082e53858289550db) )
	ROM_RELOAD(           		      0x0e0001, 0x010000 )
	ROM_LOAD16_BYTE( "s-hangon.14", 0x020000, 0x010000, CRC(0f26d131) SHA1(0d8b6eb8b8aae0aa8f0fa0c31dc91ad0e610be3e) )
	ROM_RELOAD(              		  0x0e0000, 0x010000 )
	ROM_LOAD16_BYTE( "ep10677.6",   0x040001, 0x010000, CRC(8a57b8d6) SHA1(df1a31559dd2d1e7c2c9d800bf97526bdf3e84e6) )
	ROM_LOAD16_BYTE( "ep10684.14",  0x040000, 0x010000, CRC(3aff8910) SHA1(4b41a49a7f02363424e814b37edce9a7a44a112e) )
	ROM_LOAD16_BYTE( "ep10678.5",   0x060001, 0x010000, CRC(af473098) SHA1(a2afaba1cbf672949dc50e407b46d7e9ae183774) )
	ROM_LOAD16_BYTE( "ep10685.13",  0x060000, 0x010000, CRC(80bafeef) SHA1(f01bcf65485e60f34e533295a896fca0b92e5b14) )
	ROM_LOAD16_BYTE( "ep10679.4",   0x080001, 0x010000, CRC(03bc4878) SHA1(548fc58bcc620204e30fa12fa4c4f0a3f6a1e4c0) )
	ROM_LOAD16_BYTE( "ep10686.12",  0x080000, 0x010000, CRC(274b734e) SHA1(906fa528659bc17c9b4744cec52f7096711adce8) )
	ROM_LOAD16_BYTE( "ep10680.3",   0x0a0001, 0x010000, CRC(9f0677ed) SHA1(5964642b70bfad418da44f2d91476f887b021f74) )
	ROM_LOAD16_BYTE( "ep10687.11",  0x0a0000, 0x010000, CRC(508a4701) SHA1(d17aea2aadc2e2cd65d81bf91feb3ef6923d5c0b) )
	ROM_LOAD16_BYTE( "ep10681.2",   0x0c0001, 0x010000, CRC(b176ea72) SHA1(7ec0eb0f13398d014c2e235773ded00351edb3e2) )
	ROM_LOAD16_BYTE( "ep10688.10",  0x0c0000, 0x010000, CRC(42fcd51d) SHA1(0eacb3527dc21746e5b901fcac83f2764a0f9e2c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-hangon.03", 0x0000, 0x08000, CRC(83347dc0) SHA1(079bb750edd6372750a207764e8c84bb6abf2f79) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "s-hangon.02", 0x00000, 0x10000, CRC(da08ca2b) SHA1(2c94c127efd66f6cf86b25e2653637818a99aed1) )
	ROM_LOAD( "s-hangon.01", 0x10000, 0x10000, CRC(8b10e601) SHA1(75e9bcdd3f096be9bed672d61064b9240690deec) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "s-hangon.09", 0x00000, 0x10000, CRC(070c8059) SHA1(a18c5e9473b6634f6e7165300e39029335b41ba3) )
	ROM_LOAD16_BYTE( "s-hangon.05", 0x00001, 0x10000, CRC(9916c54b) SHA1(41a7c5a9bdb1e3feae8fadf1ac5f51fab6376157) )
	ROM_LOAD16_BYTE( "s-hangon.08", 0x20000, 0x10000, CRC(000ad595) SHA1(eb80e798159c09bc5142a7ea8b9b0f895976b0d4) )
	ROM_LOAD16_BYTE( "s-hangon.04", 0x20001, 0x10000, CRC(8f8f4af0) SHA1(1dac21b7df6ec6874d36a07e30de7129b7f7f33a) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "s-hangon.26", 0x0000, 0x8000, CRC(1bbe4fc8) SHA1(30f7f301e4d10d3b254d12bf3d32e5371661a566) )
ROM_END

ROM_START( outrunb )
	ROM_REGION( 0x060000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "orun_mn.rom", 0x000000, 0x10000, CRC(cddceea2) SHA1(34cb4ca61c941e96e585f3cd2aed79bdde67f8eb) )
	ROM_LOAD16_BYTE( "orun_ml.rom", 0x000001, 0x10000, CRC(9cfc07d5) SHA1(b1b5992ff99e4158bb008684e694e088a4b282c6) )
	ROM_LOAD16_BYTE( "orun_mm.rom", 0x020000, 0x10000, CRC(3092d857) SHA1(8ebfeab9217b80a7983a4f8eb7bb7d3387d791b3) )
	ROM_LOAD16_BYTE( "orun_mk.rom", 0x020001, 0x10000, CRC(30a1c496) SHA1(734c82930197e6e8cd2bea145aedda6b3c1145d0) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, CRC(95344b04) SHA1(b3480714b11fc49b449660431f85d4ba92f799ba) )
	ROM_LOAD( "10232", 0x08000, 0x08000, CRC(776ba1eb) SHA1(e3477961d19e694c97643066534a1f720e0c4327) )
	ROM_LOAD( "10267", 0x10000, 0x08000, CRC(a85bb823) SHA1(a7e0143dee5a47e679fd5155e58e717813912692) )
	ROM_LOAD( "10231", 0x18000, 0x08000, CRC(8908bcbf) SHA1(8e1237b640a6f26bdcbfd5e201dadb2687c4febb) )
	ROM_LOAD( "10266", 0x20000, 0x08000, CRC(9f6f1a74) SHA1(09164e858ebeedcff4d389524ddf89e7c216dcae) )
	ROM_LOAD( "10230", 0x28000, 0x08000, CRC(686f5e50) SHA1(03697b892f911177968aa40de6c5f464eb0258e7) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "orun_1.rom", 	0x00000, 0x10000, CRC(77377e00) SHA1(4f376b05692f33d529f4c058dac989136c808ca1) )
	ROM_LOAD( "orun_17.rom", 	0x10000, 0x10000, CRC(4f784236) SHA1(1fb610fd29d3ddd8c5d4892ae215386b18552e6f) )
	ROM_LOAD( "orun_2.rom", 	0x20000, 0x10000, CRC(2c0e7277) SHA1(cf14d1ca1fba2e2687998c04ad2ab8c629917412) )
	ROM_LOAD( "orun_18.rom", 	0x30000, 0x10000, CRC(8d459356) SHA1(143914b1ac074708fed1d89072f915424aeb841e) )
	ROM_LOAD( "orun_3.rom", 	0x40000, 0x10000, CRC(69ecc975) SHA1(3560e9a31fc71e263a6ff61224b8db2b17836075) )
	ROM_LOAD( "orun_19.rom", 	0x50000, 0x10000, CRC(ee4f7154) SHA1(3a84c1b19d9dfcd5310e9cee90c0d4562a4a7786) )
	ROM_LOAD( "orun_4.rom", 	0x60000, 0x10000, CRC(54761e57) SHA1(dc0fc645eb998675ab9fe683d63d4ee57ae23693) )
	ROM_LOAD( "orun_20.rom",	0x70000, 0x10000, CRC(c2825654) SHA1(566ecb6e3dc76300351e54e4c0f24b9c2a6c710c) )
	ROM_LOAD( "orun_5.rom", 	0x80000, 0x10000, CRC(b6a8d0e2) SHA1(6184700dbe2c8c9c91f220e246501b7a865e4a05) )
	ROM_LOAD( "orun_21.rom", 	0x90000, 0x10000, CRC(e9880aa3) SHA1(cc47f631e758bd856bbc6d010fe230f9b1ed29de) )
	ROM_LOAD( "orun_6.rom", 	0xa0000, 0x10000, CRC(a00d0676) SHA1(c2ab29a7489c6f774ce26ef023758215ea3f7050) )
	ROM_LOAD( "orun_22.rom",	0xb0000, 0x10000, CRC(ef7d06fe) SHA1(541b5ba45f4140e2cc29a9da2592b476d414af5d) )
	ROM_LOAD( "orun_7.rom", 	0xc0000, 0x10000, CRC(d632d8a2) SHA1(27ca6faaa073bd01b2be959dba0359f93e8c1ec1) )
	ROM_LOAD( "orun_23.rom", 	0xd0000, 0x10000, CRC(dc286dc2) SHA1(eaa245b81f8a324988f617467fc3134a39b59c65) )
	ROM_LOAD( "orun_8.rom", 	0xe0000, 0x10000, CRC(da398368) SHA1(115b2881d2d5ddeda2ce82bb209a2c0b4acfcae4) )
	ROM_LOAD( "orun_24.rom",	0xf0000, 0x10000, CRC(1222af9f) SHA1(2364bd54cbe21dd688efff32e93bf154546c93d6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "orun_ma.rom", 0x00000, 0x8000, CRC(a3ff797a) SHA1(d97318a0602965cb5059c69a68609691d55a8e41) )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, CRC(bcd10dde) SHA1(417ce1d7242884640c5b14f4db8ee57cde7d085d) )
	ROM_RELOAD(              0x30000, 0x8000 ) // twice??
	ROM_LOAD( "10192",       0x08000, 0x8000, CRC(770f1270) SHA1(686bdf44d45c1d6002622f6658f037735382f3e0) )
	ROM_LOAD( "10191",       0x10000, 0x8000, CRC(20a284ab) SHA1(7c9027416d4122791ba53782fe2230cf02b7d506) )
	ROM_LOAD( "10190",       0x18000, 0x8000, CRC(7cab70e2) SHA1(a3c581d2b438630d0d4c39481dcfd85681c9f889) )
	ROM_LOAD( "10189",       0x20000, 0x8000, CRC(01366b54) SHA1(f467a6b807694d5832a985f5381c170d24aaee4e) )
	ROM_LOAD( "10188",       0x28000, 0x8000, CRC(bad30ad9) SHA1(f70dd3a6362c314adef313b064102f7a250401c8) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "orun_mj.rom", 0x00000, 0x10000, CRC(d7f5aae0) SHA1(0f9b693f078cdbbfeade5a373a94a20110d586ca) )
	ROM_LOAD16_BYTE( "orun_mh.rom", 0x00001, 0x10000, CRC(88c2e78f) SHA1(198cab9133345e4529f7fb52c29974c9a1a84933) )
	ROM_LOAD16_BYTE( "10328a",      0x20000, 0x10000, CRC(d5ec5e5d) SHA1(a4e3cfca4d803e72bc4fcf91ab00e21bf3f8959f) )
	ROM_LOAD16_BYTE( "orun_mg.rom", 0x20001, 0x10000, CRC(74c5fbec) SHA1(a44f1477d830fdb4d6c29351da94776843e5d3e1) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "orun_me.rom", 0x0000, 0x8000, CRC(666fe754) SHA1(606090db53d658d7b04dca4748014a411e12f259) )

//	ROM_LOAD( "orun_mf.rom", 0x0000, 0x8000, CRC(ed5bda9c) )	//??
ROM_END


/***************************************************************************/

#if 0
static READ16_HANDLER( or_io_joy_r ){
	return (input_port_5_r( offset ) << 8) + input_port_6_r( offset );
}
#endif

#ifdef HANGON_DIGITAL_CONTROLS
static READ16_HANDLER( or_io_brake_r ){
	int data = input_port_1_r( offset );

	switch(data & 3)
	{
		case 3:	return 0xff00;	// both
		case 1:	return 0xff00;  // brake
		case 2:	return 0x0000;  // accel
		case 0:	return 0x0000;  // neither
	}
	return 0x0000;
}

static READ16_HANDLER( or_io_acc_steer_r ){
	int data = input_port_1_r( offset );
	int ret = input_port_0_r( offset ) << 8;

	switch(data & 3)
	{
		case 3:	return 0x00 | ret;	// both
		case 1:	return 0x00 | ret;  // brake
		case 2:	return 0xff | ret;  // accel
		case 0:	return 0x00 | ret ;  // neither
	}
	return 0x00 | ret;
}
#else
static READ16_HANDLER( or_io_acc_steer_r ){ return (input_port_0_r( offset ) << 8) + input_port_1_r( offset ); }
static READ16_HANDLER( or_io_brake_r ){ return input_port_5_r( offset ) << 8; }
#endif

static int selected_analog;

static READ16_HANDLER( outrun_analog_r )
{
	switch (selected_analog)
	{
		default:
		case 0: return or_io_acc_steer_r(0,0) >> 8;
		case 1: return or_io_acc_steer_r(0,0) & 0xff;
		case 2: return or_io_brake_r(0,0) >> 8;
		case 3: return or_io_brake_r(0,0) & 0xff;
	}
}

static WRITE16_HANDLER( outrun_analog_select_w )
{
	if ( ACCESSING_LSB )
	{
		selected_analog = (data & 0x0c) >> 2;
	}
}

static int or_gear=0;

static READ16_HANDLER( or_io_service_r )
{
	int ret=input_port_2_r( offset );
	int data=input_port_1_r( offset );
	if(data & 4) or_gear=0;
	else if(data & 8) or_gear=1;

	if(or_gear) ret|=0x10;
	else ret&=0xef;

	return ret;
}

static WRITE16_HANDLER( outrun_sound_write_w )
{
	sound_shared_ram[0]=data&0xff;
}


static WRITE16_HANDLER( outrun_ctrl2_w )
{
	if( ACCESSING_LSB ){
		/* bit 0 always 1? */
		set_led_status(0,data & 0x04);
		set_led_status(1,data & 0x02);	/* brakes */
		coin_counter_w(0,data & 0x10);
	}
}
static unsigned char ctrl1;

static WRITE16_HANDLER( outrun_ctrl1_w )
{
/* sound be sound cpu reset, not 2nd 68k cpu? */
#if 0
	if(ACCESSING_LSB) {
		int changed = data ^ ctrl1;
		ctrl1 = data;
		if(changed & 1) {
			if(data & 1) {
				cpunum_set_input_line(2, INPUT_LINE_HALT, CLEAR_LINE);
				cpunum_set_input_line(2, INPUT_LINE_RESET, PULSE_LINE);
			} else
				cpunum_set_input_line(2, INPUT_LINE_HALT, ASSERT_LINE);
		}

//		sys16_kill_set(data & 0x20);

		/* bit 0 always 1? */
		/* bits 2-3 continuously change: 00-01-10-11; this is the same that
		   gets written to 140030 so is probably input related */
	}
#endif
}

static void outrun_reset(void)
{
	cpunum_set_input_line(2, INPUT_LINE_RESET, PULSE_LINE);
	cpunum_set_input_line(2, INPUT_LINE_HALT, CLEAR_LINE);
}



static ADDRESS_MAP_START( outrun_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x060900, 0x060907) AM_READ(sound_shared_ram_r)		//???
	AM_RANGE(0x060000, 0x067fff) AM_READ(SYS16_MRA16_EXTRAM2)

	AM_RANGE(0x100000, 0x10ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x110000, 0x110fff) AM_READ(SYS16_MRA16_TEXTRAM)

	AM_RANGE(0x130000, 0x130fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x120000, 0x121fff) AM_READ(SYS16_MRA16_PALETTERAM)

	AM_RANGE(0x140010, 0x140011) AM_READ(or_io_service_r)
	AM_RANGE(0x140014, 0x140015) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0x140016, 0x140017) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0x140030, 0x140031) AM_READ(outrun_analog_r)

	AM_RANGE(0x200000, 0x23ffff) AM_READ(SYS16_CPU3ROM16_r)
	AM_RANGE(0x260000, 0x267fff) AM_READ(shared_ram_r)
	AM_RANGE(0xe00000, 0xe00001) AM_READ(SYS16_CPU2_RESET_HACK)
ADDRESS_MAP_END

static ADDRESS_MAP_START( outrun_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x060900, 0x060907) AM_WRITE(sound_shared_ram_w)		//???
	AM_RANGE(0x060000, 0x067fff) AM_WRITE(SYS16_MWA16_EXTRAM2) AM_BASE(&sys16_extraram2)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x110000, 0x110fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x130000, 0x130fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x120000, 0x121fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0x140004, 0x140005) AM_WRITE(outrun_ctrl1_w)
	AM_RANGE(0x140020, 0x140021) AM_WRITE(outrun_ctrl2_w)
	AM_RANGE(0x140030, 0x140031) AM_WRITE(outrun_analog_select_w)
	AM_RANGE(0x200000, 0x23ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x260000, 0x267fff) AM_WRITE(shared_ram_w) AM_BASE(&shared_ram)
	AM_RANGE(0xffff06, 0xffff07) AM_WRITE(outrun_sound_write_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( outrun_readmem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x060000, 0x067fff) AM_READ(shared_ram_r)
	AM_RANGE(0x080000, 0x09ffff) AM_READ(SYS16_MRA16_EXTRAM)		// gr
ADDRESS_MAP_END

static ADDRESS_MAP_START( outrun_writemem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x060000, 0x067fff) AM_WRITE(shared_ram_w)
	AM_RANGE(0x080000, 0x09ffff) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)		// gr
ADDRESS_MAP_END

// Outrun

static ADDRESS_MAP_START( outrun_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xf0ff) AM_READ(SegaPCM_r)
	AM_RANGE(0xf100, 0xf7ff) AM_READ(MRA8_NOP)
	AM_RANGE(0xf800, 0xf807) AM_READ(sound2_shared_ram_r)
	AM_RANGE(0xf808, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( outrun_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xf0ff) AM_WRITE(SegaPCM_w)
	AM_RANGE(0xf100, 0xf7ff) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf800, 0xf807) AM_WRITE(sound2_shared_ram_w) AM_BASE(&sound_shared_ram)
	AM_RANGE(0xf808, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

/***************************************************************************/

static void outrun_update_proc( void ){
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( outrun ){
	static int bank[8] = {
		7,0,1,2,
		3,4,5,6
	};
	sys16_obj_bank = bank;
	sys16_spritesystem = sys16_sprite_outrun;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;
	sys16_sprxoffset = -0xc0;
	ctrl1 = 0x20;

// *forced sound cmd (eww)
	if (!strcmp(Machine->gamedrv->name,"outrun")) sys16_patch_code( 0x55ed, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outruna")) sys16_patch_code( 0x5661, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outrunb")) sys16_patch_code( 0x5661, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outrundx")) sys16_patch_code( 0x559f, 0x00);

	cpunum_set_info_fct(0, CPUINFO_PTR_M68K_RESET_CALLBACK, (genf *)outrun_reset);

	sys16_update_proc = outrun_update_proc;

	sys16_gr_ver = &sys16_extraram[0];
	sys16_gr_hor = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0xc00/2;

	sys16_gr_palette= 0xf00 / 2;
	sys16_gr_palette_default = 0x800 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x06 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_gr_second_road = &sys16_extraram[0x8000];

	cpunum_set_input_line(2, INPUT_LINE_HALT, ASSERT_LINE);

}

static DRIVER_INIT( outrun )
{
	machine_init_sys16_onetime();
	sys16_interleave_sprite_data( 0x100000 );
	generate_gr_screen(512,2048,0,0,3,0x8000);
}



static DRIVER_INIT( outrunb )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	int i;

	machine_init_sys16_onetime();
/*
  Main Processor
	Comparing the bootleg with the custom bootleg, it seems that:-

  if even bytes &0x28 == 0x20 or 0x08 then they are xored with 0x28
  if odd bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0

  ie. data lines are switched.
*/

	for( i=0;i<0x40000;i+=2 ){
		data16_t word = RAM[i/2];
		UINT8 even = word>>8;
		UINT8 odd = word&0xff;

		// even byte
		if((even&0x28) == 0x20 || (even&0x28) == 0x08) even^=0x28;

		// odd byte
		if((odd&0xc0) == 0x80 || (odd&0xc0) == 0x40) odd^=0xc0;

		RAM[i/2] = (even<<8)+odd;
	}

/*
  Second Processor

  if even bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0
  if odd bytes &0x0c == 0x04 or 0x08 then they are xored with 0x0c
*/
	RAM = (data16_t *)memory_region(REGION_CPU3);
	for(i=0;i<0x40000;i+=2)
	{
		data16_t word = RAM[i/2];
		UINT8 even = word>>8;
		UINT8 odd = word&0xff;

		// even byte
		if((even&0xc0) == 0x80 || (even&0xc0) == 0x40) even^=0xc0;

		// odd byte
		if((odd&0x0c) == 0x08 || (odd&0x0c) == 0x04) odd^=0x0c;

		RAM[i/2] = (even<<8)+odd;
	}
/*
  Road GFX

	rom orun_me.rom
	if bytes &0x60 == 0x40 or 0x20 then they are xored with 0x60

	rom orun_mf.rom
	if bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0

  I don't know why there's 2 road roms, but I'm using orun_me.rom
*/
	{
		UINT8 *mem = memory_region(REGION_GFX3);
		for(i=0;i<0x8000;i++){
			if( (mem[i]&0x60) == 0x20 || (mem[i]&0x60) == 0x40 ) mem[i]^=0x60;
		}
	}

	generate_gr_screen(512,2048,0,0,3,0x8000);
	sys16_interleave_sprite_data( 0x100000 );

/*
  Z80 Code
	rom orun_ma.rom
	if bytes &0x60 == 0x40 or 0x20 then they are xored with 0x60

*/
	{
		UINT8 *mem = memory_region(REGION_CPU2);
		for(i=0;i<0x8000;i++){
			if( (mem[i]&0x60) == 0x20 || (mem[i]&0x60) == 0x40 ) mem[i]^=0x60;
		}
	}
}

/***************************************************************************/

INPUT_PORTS_START( outrun )
PORT_START	/* Steering */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x48,0xb8) PORT_SENSITIVITY(100) PORT_KEYDELTA(3)
//	PORT_BIT( 0xff, 0x7f, IPT_PADDLE  ) PORT_MINMAX(0x48,0xb8) PORT_SENSITIVITY(70) PORT_KEYDELTA(3)

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )

#else

PORT_START	/* Accel / Decel */
	PORT_BIT( 0xff, 0x30, IPT_AD_STICK_Y ) PORT_MINMAX(0x30,0x90) PORT_SENSITIVITY(100) PORT_KEYDELTA(16) PORT_REVERSE

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, "Up Cockpit" )
	PORT_DIPSETTING(    0x01, "Mini Up" )
	PORT_DIPSETTING(    0x03, "Moving" )
//	PORT_DIPSETTING(    0x00, "No Use" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Time" )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Enemies" )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_BIT( 0xff, 0x30, IPT_AD_STICK_Y ) PORT_MINMAX(0x30,0x90) PORT_SENSITIVITY(100) PORT_KEYDELTA(16) PORT_REVERSE PORT_PLAYER(2)

#endif

INPUT_PORTS_END

/***************************************************************************/
static INTERRUPT_GEN( or_interrupt ){
	int intleft=cpu_getiloops();
	if(intleft!=0) cpunum_set_input_line(0, 2, HOLD_LINE);
	else cpunum_set_input_line(0, 4, HOLD_LINE);
}


static MACHINE_DRIVER_START( outrun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(outrun_readmem,outrun_writemem)
	MDRV_CPU_VBLANK_INT(or_interrupt,2)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(outrun_sound_readmem,outrun_sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(outrun_readmem2,outrun_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(outrun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*ShadowColorsMultiplier)

	MDRV_VIDEO_START(old_outrun)
	MDRV_VIDEO_UPDATE(old_outrun)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k)
MACHINE_DRIVER_END

#if 0
static MACHINE_DRIVER_START( outruna )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(outrun)

	MDRV_MACHINE_INIT(outruna)
MACHINE_DRIVER_END
#endif

static data16_t *shared_ram2;
static READ16_HANDLER( shared_ram2_r ){
	return shared_ram2[offset];
}
static WRITE16_HANDLER( shared_ram2_w ){
	COMBINE_DATA(&shared_ram2[offset]);
}

static ADDRESS_MAP_START( shangon_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x20c640, 0x20c647) AM_READ(sound_shared_ram_r)
	AM_RANGE(0x20c000, 0x20ffff) AM_READ(SYS16_MRA16_EXTRAM2)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x600000, 0x600fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0xa00000, 0xa00fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc68000, 0xc68fff) AM_READ(shared_ram_r)
	AM_RANGE(0xc7c000, 0xc7ffff) AM_READ(shared_ram2_r)
	AM_RANGE(0xe00002, 0xe00003) AM_READ(sys16_coinctrl_r)
	AM_RANGE(0xe01000, 0xe01001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xe0100c, 0xe0100d) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xe0100a, 0xe0100b) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xe030f8, 0xe030f9) AM_READ(ho_io_x_r)
	AM_RANGE(0xe030fa, 0xe030fb) AM_READ(ho_io_y_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x20c640, 0x20c647) AM_WRITE(sound_shared_ram_w)
	AM_RANGE(0x20c000, 0x20ffff) AM_WRITE(SYS16_MWA16_EXTRAM2) AM_BASE(&sys16_extraram2)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x600000, 0x600fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0xa00000, 0xa00fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc68000, 0xc68fff) AM_WRITE(shared_ram_w) AM_BASE(&shared_ram)
	AM_RANGE(0xc7c000, 0xc7ffff) AM_WRITE(shared_ram2_w) AM_BASE(&shared_ram2)
	AM_RANGE(0xe00002, 0xe00003) AM_WRITE(sys16_3d_coinctrl_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_readmem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x454000, 0x45401f) AM_READ(SYS16_MRA16_EXTRAM3)
	AM_RANGE(0x7e8000, 0x7e8fff) AM_READ(shared_ram_r)
	AM_RANGE(0x7fc000, 0x7ffbff) AM_READ(shared_ram2_r)
	AM_RANGE(0x7ffc00, 0x7fffff) AM_READ(SYS16_MRA16_EXTRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_writemem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x454000, 0x45401f) AM_WRITE(SYS16_MWA16_EXTRAM3) AM_BASE(&sys16_extraram3)
	AM_RANGE(0x7e8000, 0x7e8fff) AM_WRITE(shared_ram_w)
	AM_RANGE(0x7fc000, 0x7ffbff) AM_WRITE(shared_ram2_w)
	AM_RANGE(0x7ffc00, 0x7fffff) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xf7ff) AM_READ(SegaPCM_r)
	AM_RANGE(0xf800, 0xf807) AM_READ(sound2_shared_ram_r)
	AM_RANGE(0xf808, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(SegaPCM_w)
	AM_RANGE(0xf800, 0xf807) AM_WRITE(sound2_shared_ram_w) AM_BASE(&sound_shared_ram)
	AM_RANGE(0xf808, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

/***************************************************************************/

static void shangon_update_proc( void ){
	set_bg_page1( sys16_textram[0x74e] );
	set_fg_page1( sys16_textram[0x74f] );
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static MACHINE_INIT( shangon ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_hangon;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_patch_code( 0x65bd, 0xf9);
	sys16_patch_code( 0x6677, 0xfa);
	sys16_patch_code( 0x66d5, 0xfb);
	sys16_patch_code( 0x9621, 0xfb);

	sys16_update_proc = shangon_update_proc;

	sys16_gr_ver = shared_ram;
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x06 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x04 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x02 / 2;
}

static DRIVER_INIT( shangon ){
	machine_init_sys16_onetime();
	generate_gr_screen(512,1024,0,0,4,0x8000);

	sys16_patch_z80code( 0x1087, 0x20);
	sys16_patch_z80code( 0x1088, 0x01);
}

static DRIVER_INIT( shangonb ){
	machine_init_sys16_onetime();
	generate_gr_screen(512,1024,8,0,4,0x8000);
}
/***************************************************************************/

INPUT_PORTS_START( shangon )
PORT_START	/* Steering */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE  ) PORT_MINMAX(0x42,0xbd) PORT_SENSITIVITY(100) PORT_KEYDELTA(3) PORT_REVERSE

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )

#else

PORT_START	/* Accel / Decel */
	PORT_BIT( 0xff, 0x1, IPT_AD_STICK_Y ) PORT_MINMAX(1,0xa2) PORT_SENSITIVITY(100) PORT_KEYDELTA(16) PORT_REVERSE

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x18, "Time Adj." )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, "Play Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_BIT( 0xff, 0x1, IPT_AD_STICK_Y ) PORT_MINMAX(1,0xa2) PORT_SENSITIVITY(100) PORT_KEYDELTA(16) PORT_REVERSE PORT_PLAYER(2)

#endif
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( shangon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(shangon_readmem,shangon_writemem)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(shangon_sound_readmem,shangon_sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(shangon_readmem2,shangon_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100)

	MDRV_MACHINE_INIT(shangon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	MDRV_VIDEO_START(hangon)
	MDRV_VIDEO_UPDATE(hangon)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k_512)
MACHINE_DRIVER_END

GAME( 1992, shangonb, shangon,  shangon,  shangon,  shangonb, ROT0,         "bootleg", "Super Hang-On (bootleg)" )

