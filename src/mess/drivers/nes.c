/***************************************************************************

  nes.c

  Driver file to handle emulation of the Nintendo Entertainment System (Famicom).

  MESS driver by Brad Oliver (bradman@pobox.com), NES sound code by Matt Conte.
  Based in part on the old xNes code, by Nicolas Hamel, Chuck Mason, Brad Oliver,
  Richard Bannister and Jeff Mitchell.

***************************************************************************/

#include "emu.h"
#include "video/ppu2c0x.h"
#include "machine/nes_mmc.h"
#include "includes/nes.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/cartslot.h"
#include "sound/nes_apu.h"
#include "imagedev/flopdrv.h"
#include "formats/nes_dsk.h"


static READ8_DEVICE_HANDLER( psg_4015_r )
{
	return nes_psg_r(device, 0x15);
}

static WRITE8_DEVICE_HANDLER( psg_4015_w )
{
	nes_psg_w(device, 0x15, data);
}

static WRITE8_DEVICE_HANDLER( psg_4017_w )
{
	nes_psg_w(device, 0x17, data);
}

static WRITE8_HANDLER(nes_vh_sprite_dma_w)
{
	nes_state *state = space->machine->driver_data<nes_state>();
	ppu2c0x_spriteram_dma(space, state->ppu, data);
}

static ADDRESS_MAP_START( nes_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_MIRROR(0x1800)					/* RAM */
	AM_RANGE(0x2000, 0x3fff) AM_DEVREADWRITE("ppu", ppu2c0x_r, ppu2c0x_w)		/* PPU registers */
	AM_RANGE(0x4000, 0x4013) AM_DEVREADWRITE("nessound", nes_psg_r, nes_psg_w)		/* PSG primary registers */
	AM_RANGE(0x4014, 0x4014) AM_WRITE(nes_vh_sprite_dma_w)				/* stupid address space hole */
	AM_RANGE(0x4015, 0x4015) AM_DEVREADWRITE("nessound", psg_4015_r, psg_4015_w)		/* PSG status / first control register */
	AM_RANGE(0x4016, 0x4016) AM_READWRITE(nes_IN0_r, nes_IN0_w)			/* IN0 - input port 1 */
	AM_RANGE(0x4017, 0x4017) AM_READ(nes_IN1_r)							/* IN1 - input port 2 */
	AM_RANGE(0x4017, 0x4017) AM_DEVWRITE("nessound", psg_4017_w)		/* PSG second control register */
	AM_RANGE(0x4100, 0x5fff) AM_READWRITE(nes_low_mapper_r, nes_low_mapper_w)	/* Perform unholy acts on the machine */
ADDRESS_MAP_END


static INPUT_PORTS_START( nes_controllers )
	PORT_START("CTRLSEL")  /* Select Controller Type */
	PORT_CATEGORY_CLASS( 0x000f, 0x0001, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0001, "Gamepad",			1 )
	PORT_CATEGORY_ITEM(  0x0002, "Zapper",			5 )
	PORT_CATEGORY_ITEM(  0x0006, "Crazy Climber pad", 10 )
	PORT_CATEGORY_CLASS( 0x00f0, 0x0010, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0010, "Gamepad",			2 )
	PORT_CATEGORY_ITEM(  0x0030, "Zapper",			6 )
	PORT_CATEGORY_ITEM(  0x0040, "Arkanoid paddle",	7 )
//  PORT_CATEGORY_ITEM(  0x0050, "Family Trainer",  11 )
	PORT_CATEGORY_CLASS( 0x0f00, 0x0000, "P3 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0100, "Gamepad",			3 )
	PORT_CATEGORY_CLASS( 0xf000, 0x0000, "P4 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x1000, "Gamepad",			4 )

	PORT_START("PAD1")	/* Joypad 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 A") PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P1 B") PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CATEGORY(1) PORT_PLAYER(1)

	PORT_START("PAD2")	/* Joypad 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P2 A") PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P2 B") PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CATEGORY(2) PORT_PLAYER(2)

	PORT_START("PAD3")	/* Joypad 3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P3 A") PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P3 B") PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CATEGORY(3) PORT_PLAYER(3)

	PORT_START("PAD4")	/* Joypad 4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P4 A") PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P4 B") PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_CATEGORY(4) PORT_PLAYER(4)

	PORT_START("ZAPPER1_X")  /* P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255)														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START("ZAPPER1_Y")  /* P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255)														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START("ZAPPER1_T")  /* P1 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P1 Lightgun Trigger") PORT_CATEGORY(5) PORT_PLAYER(1)

	PORT_START("ZAPPER2_X")  /* P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START("ZAPPER2_Y")  /* P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START("ZAPPER2_T")  /* P2 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 Lightgun Trigger") PORT_CATEGORY(6) PORT_PLAYER(2)

	PORT_START("PADDLE")  /* Arkanoid paddle */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE) PORT_SENSITIVITY(25) PORT_KEYDELTA(3) PORT_MINMAX(0x62,0xf2)																	PORT_CATEGORY(7)

	PORT_START("CC_LEFT")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT ) PORT_CATEGORY(10) PORT_PLAYER(1)

	PORT_START("CC_RIGHT")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT ) PORT_CATEGORY(10) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT ) PORT_CATEGORY(10) PORT_PLAYER(1)

INPUT_PORTS_END

static INPUT_PORTS_START( nes )
	PORT_INCLUDE( nes_controllers )

	PORT_START("CONFIG")  /* configuration */
	PORT_CONFNAME( 0x01, 0x00, "Draw Top/Bottom 8 Lines")
	PORT_CONFSETTING(    0x01, DEF_STR(No) )
	PORT_CONFSETTING(    0x00, DEF_STR(Yes) )
	PORT_CONFNAME( 0x02, 0x00, "Enforce 8 Sprites/line")
	PORT_CONFSETTING(    0x02, DEF_STR(No) )
	PORT_CONFSETTING(    0x00, DEF_STR(Yes) )
INPUT_PORTS_END

static INPUT_PORTS_START( famicom )
	PORT_INCLUDE( nes_controllers )

	PORT_START("FLIPDISK") /* fake keys */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Change Disk Side")
INPUT_PORTS_END

static INPUT_PORTS_START( fc_keyboard )
	PORT_START("FCKEY0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('[')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR(']')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_NAME("Kana")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\\')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_NAME("Stop") PORT_CODE(KEYCODE_BACKSPACE)

	PORT_START("FCKEY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_COLON)	PORT_CHAR(':')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_QUOTE)	PORT_CHAR(';')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CHAR('_')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CHAR('/')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('^')

	PORT_START("FCKEY2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_O)	PORT_CHAR('O')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_L)	PORT_CHAR('L')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_K)	PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_STOP)	PORT_CHAR('.')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_P)	PORT_CHAR('P')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_0)	PORT_CHAR('0')

	PORT_START("FCKEY3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_I)	PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_U)	PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_J)	PORT_CHAR('J')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_M)	PORT_CHAR('M')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_N)	PORT_CHAR('N')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_9)	PORT_CHAR('9')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_8)	PORT_CHAR('8')

	PORT_START("FCKEY4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_Y)	PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_G)	PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_H)	PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_B)	PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_V)	PORT_CHAR('V')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_7)	PORT_CHAR('7')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_6)	PORT_CHAR('6')

	PORT_START("FCKEY5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_T)	PORT_CHAR('T')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_R)	PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_D)	PORT_CHAR('D')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F)	PORT_CHAR('F')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_C)	PORT_CHAR('C')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_5)	PORT_CHAR('5')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_4)	PORT_CHAR('4')

	PORT_START("FCKEY6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_W)	PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_S)	PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_A)	PORT_CHAR('A')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_X)	PORT_CHAR('X')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_Z)	PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_E)	PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_3)	PORT_CHAR('3')

	PORT_START("FCKEY7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_Q)	PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_NAME("Grph") PORT_CODE(KEYCODE_LALT)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_1)	PORT_CHAR('1')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_2)	PORT_CHAR('2')

	PORT_START("FCKEY8")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_NAME("Clr")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_UP)	PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_NAME("Del") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(8) PORT_NAME("Ins") PORT_CODE(KEYCODE_INSERT)

INPUT_PORTS_END

static INPUT_PORTS_START( subor_keyboard )
	PORT_START("SUBKEY0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_4)	PORT_CHAR('4')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_G)	PORT_CHAR('G')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F)	PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_C)	PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_E)	PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_5)	PORT_CHAR('5')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_V)	PORT_CHAR('V')

	PORT_START("SUBKEY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_2)	PORT_CHAR('2')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_D)	PORT_CHAR('D')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_S)	PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_W)	PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_3)	PORT_CHAR('3')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_X)	PORT_CHAR('X')

	PORT_START("SUBKEY2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_NAME("NEXT")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_NAME("PRIOR")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))

	PORT_START("SUBKEY3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_9)	PORT_CHAR('9')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_I)	PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_L)	PORT_CHAR('L')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_O)	PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_0)	PORT_CHAR('0')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_STOP)	PORT_CHAR('.')

	PORT_START("SUBKEY4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_ENTER)	PORT_CHAR(13)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_START("SUBKEY5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_Q)	PORT_CHAR('Q')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_Z)	PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_TAB)	PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_A)	PORT_CHAR('A')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_1)	PORT_CHAR('1')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_SHIFT_2)

	PORT_START("SUBKEY6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_7)	PORT_CHAR('7')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_Y)	PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_K)	PORT_CHAR('K')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_M)	PORT_CHAR('M')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_U)	PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_8)	PORT_CHAR('8')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_J)	PORT_CHAR('J')

	PORT_START("SUBKEY7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_COLON)	PORT_CHAR(':')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_QUOTE)	PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_SLASH)	PORT_CHAR('/')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_P)		PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('=')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_LSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("SUBKEY8")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_T)	PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_H)	PORT_CHAR('H')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_N)	PORT_CHAR('N')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_SPACE)	PORT_CHAR(' ')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_R)	PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_6)	PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_B)	PORT_CHAR('B')

	PORT_START("SUBKEY9")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) PORT_CATEGORY(9)

	PORT_START("SUBKEY10")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_NAME("LMENU")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_4_PAD)	PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_7_PAD)	PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F11)		PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F12)		PORT_CHAR(UCHAR_MAMEKEY(F12))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_1_PAD)	PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_2_PAD)	PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_8_PAD)	PORT_CHAR(UCHAR_MAMEKEY(8_PAD))

	PORT_START("SUBKEY11")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_MINUS_PAD)PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_PLUS_PAD)	PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_ASTERISK)	PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_9_PAD)	PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_5_PAD)	PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_SLASH_PAD)PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_NUMLOCK)	PORT_CHAR(UCHAR_MAMEKEY(NUMLOCK))

	PORT_START("SUBKEY12")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_TILDE)	PORT_CHAR('`')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_6_PAD)	PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_NAME("PAUSE")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_NAME("SPACE2")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_3_PAD)	PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CATEGORY(9) PORT_CODE(KEYCODE_0_PAD)	PORT_CHAR(UCHAR_MAMEKEY(0_PAD))

INPUT_PORTS_END

static INPUT_PORTS_START( famikey )
	PORT_INCLUDE( famicom )
	PORT_INCLUDE( fc_keyboard )
	PORT_INCLUDE( subor_keyboard )

	PORT_MODIFY("CTRLSEL")  /* Select Controller Type */
	PORT_CATEGORY_CLASS( 0x000f, 0x0004, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0001, "Gamepad",			1 )
	PORT_CATEGORY_ITEM(  0x0002, "Zapper",			5 )
	PORT_CATEGORY_ITEM(  0x0006, "Crazy Climber pad", 10 )
	PORT_CATEGORY_ITEM(  0x0007, "FC Keyboard",		8 )
	PORT_CATEGORY_ITEM(  0x0008, "Subor Keyboard",	9 )
	PORT_CATEGORY_CLASS( 0x00f0, 0x0010, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0010, "Gamepad",			2 )
	PORT_CATEGORY_ITEM(  0x0030, "Zapper",			6 )
	PORT_CATEGORY_ITEM(  0x0040, "Arkanoid paddle",	7 )
//  PORT_CATEGORY_ITEM(  0x0050, "Family Trainer",  11 )
	PORT_CATEGORY_CLASS( 0x0f00, 0x0000, "P3 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0100, "Gamepad",			3 )
	PORT_CATEGORY_CLASS( 0xf000, 0x0000, "P4 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x1000, "Gamepad",			4 )
INPUT_PORTS_END

#ifdef UNUSED_FUNCTION
/* This layout is not changed at runtime */
gfx_layout nes_vram_charlayout =
{
    8,8,    /* 8*8 characters */
    512,    /* 512 characters */
    2,  /* 2 bits per pixel */
    { 8*8, 0 }, /* the two bitplanes are separated */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    16*8    /* every char takes 16 consecutive bytes */
};
#endif

static const nes_interface nes_apu_interface =
{
	"maincpu"
};


static void ppu_nmi(device_t *device, int *ppu_regs)
{
	cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
}


static const ppu2c0x_interface nes_ppu_interface =
{
	0,
	0,
	PPU_MIRROR_NONE,
	ppu_nmi
};

static const floppy_config nes_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(nes_only),
	NULL
};


static MACHINE_CONFIG_START( nes, nes_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", N2A03, NTSC_CLOCK)
	MCFG_CPU_PROGRAM_MAP(nes_map)

	MCFG_MACHINE_START( nes )
	MCFG_MACHINE_RESET( nes )

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60.098)
	// This isn't used so much to calulate the vblank duration (the PPU code tracks that manually) but to determine
	// the number of cycles in each scanline for the PPU scanline timer. Since the PPU has 20 vblank scanlines + 2
	// non-rendering scanlines, we compensate. This ends up being 2500 cycles for the non-rendering portion, 2273
	// cycles for the actual vblank period.
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC((113.66/(NTSC_CLOCK/1000000)) * (PPU_VBLANK_LAST_SCANLINE_NTSC-PPU_VBLANK_FIRST_SCANLINE+1+2)))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(32*8, 262)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MCFG_SCREEN_UPDATE(nes)

	MCFG_PALETTE_INIT(nes)
	MCFG_VIDEO_START(nes)

	MCFG_PALETTE_LENGTH(4*16*8)

	MCFG_PPU2C02_ADD( "ppu", nes_ppu_interface )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("nessound", NES, NTSC_CLOCK)
	MCFG_SOUND_CONFIG(nes_apu_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)

	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("nes,unf")
	MCFG_CARTSLOT_MANDATORY
	MCFG_CARTSLOT_INTERFACE("nes_cart")
	MCFG_CARTSLOT_LOAD(nes_cart)
	MCFG_CARTSLOT_PARTIALHASH(nes_partialhash)
	MCFG_SOFTWARE_LIST_ADD("cart_list","nes")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( nespal, nes )

	/* basic machine hardware */
	MCFG_CPU_MODIFY( "maincpu" )
	MCFG_CPU_CLOCK( PAL_CLOCK )

	MCFG_DEVICE_REMOVE( "ppu" )
	MCFG_PPU2C07_ADD( "ppu", nes_ppu_interface )

	/* video hardware */
	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_REFRESH_RATE(53.355)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC((106.53/(PAL_CLOCK/1000000)) * (PPU_VBLANK_LAST_SCANLINE_PAL-PPU_VBLANK_FIRST_SCANLINE+1+2)))
	MCFG_VIDEO_START(nes)

	/* sound hardware */
	MCFG_SOUND_REPLACE("nessound", NES, PAL_CLOCK)
	MCFG_SOUND_CONFIG(nes_apu_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( dendy, nes )

	/* basic machine hardware */
	MCFG_CPU_MODIFY( "maincpu" )
	MCFG_CPU_CLOCK( 26601712/15 ) /* 26.601712MHz / 15 == 1.77344746666... MHz */

	MCFG_DEVICE_REMOVE( "ppu" )
	MCFG_PPU2C07_ADD( "ppu", nes_ppu_interface )

	/* video hardware */
	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_REFRESH_RATE(50.00697796827)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC((106.53/(PAL_CLOCK/1000000)) * (PPU_VBLANK_LAST_SCANLINE_PAL-PPU_VBLANK_FIRST_SCANLINE+1+2)))
	MCFG_VIDEO_START(nes)

	/* sound hardware */
	MCFG_SOUND_REPLACE("nessound", NES, 26601712/15) /* 26.601712MHz / 15 == 1.77344746666... MHz */
	MCFG_SOUND_CONFIG(nes_apu_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( famicom, nes )

	MCFG_CARTSLOT_MODIFY("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("nes,unf")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(nes_cart)
	MCFG_CARTSLOT_PARTIALHASH(nes_partialhash)	

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, nes_floppy_config)
MACHINE_CONFIG_END


/* rom regions are just place-holders: they get removed and re-allocated when a cart is loaded */
ROM_START( nes )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )  /* Main RAM + program banks */
	ROM_REGION( 0x2000,  "gfx1", ROMREGION_ERASE00 )  /* VROM */
	ROM_REGION( 0x4000,  "gfx2", ROMREGION_ERASE00 )  /* VRAM */
	ROM_REGION( 0x800,   "gfx3", ROMREGION_ERASE00 )  /* CI RAM */
ROM_END

ROM_START( nespal )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )  /* Main RAM + program banks */
	ROM_REGION( 0x2000,  "gfx1", ROMREGION_ERASE00 )  /* VROM */
	ROM_REGION( 0x4000,  "gfx2", ROMREGION_ERASE00 )  /* VRAM */
	ROM_REGION( 0x800,   "gfx3", ROMREGION_ERASE00 )  /* CI RAM */
ROM_END

ROM_START( famicom )
	ROM_REGION( 0x10000, "maincpu", 0 )  /* Main RAM + program banks */
	ROM_LOAD_OPTIONAL( "disksys.rom", 0xe000, 0x2000, CRC(5e607dcf) SHA1(57fe1bdee955bb48d357e463ccbf129496930b62) )

	ROM_REGION( 0x2000,  "gfx1", ROMREGION_ERASE00 )  /* VROM */
	ROM_REGION( 0x4000,  "gfx2", ROMREGION_ERASE00 )  /* VRAM */
	ROM_REGION( 0x800,   "gfx3", ROMREGION_ERASE00 )  /* CI RAM */
ROM_END

#define rom_fami_key rom_famicom

ROM_START( famitwin )
	ROM_REGION( 0x10000, "maincpu", 0 )  /* Main RAM + program banks */
	ROM_LOAD_OPTIONAL( "disksyst.rom", 0xe000, 0x2000, CRC(4df24a6c) SHA1(e4e41472c454f928e53eb10e0509bf7d1146ecc1) )

	ROM_REGION( 0x2000,  "gfx1", ROMREGION_ERASE00 )  /* VROM */
	ROM_REGION( 0x4000,  "gfx2", ROMREGION_ERASE00 )  /* VRAM */
	ROM_REGION( 0x800,   "gfx3", ROMREGION_ERASE00 )  /* CI RAM */
ROM_END

ROM_START( m82 )
	ROM_REGION( 0x14000, "maincpu", 0 )  /* Main RAM + program banks */
	/* Banks to be mapped at 0xe000? More investigations needed... */
	ROM_LOAD( "m82_v1_0.bin", 0x10000, 0x4000, CRC(7d56840a) SHA1(cbd2d14fa073273ba58367758f40d67fd8a9106d) )

	ROM_REGION( 0x2000,  "gfx1", ROMREGION_ERASE00 )  /* VROM */
	ROM_REGION( 0x4000,  "gfx2", ROMREGION_ERASE00 )  /* VRAM */
	ROM_REGION( 0x800,   "gfx3", ROMREGION_ERASE00 )  /* CI RAM */
ROM_END

// see http://www.disgruntleddesigner.com/chrisc/drpcjr/index.html
// and http://www.disgruntleddesigner.com/chrisc/drpcjr/DrPCJrMemMap.txt
ROM_START( drpcjr )
	ROM_REGION( 0x18000, "maincpu", 0 )  /* Main RAM + program banks */
	/* 4 banks to be mapped in 0xe000-0xffff (or 8 banks to be mapped in 0xe000-0xefff & 0xf000-0xffff).
    Banks selected by writing at 0x4180 */
	ROM_LOAD("drpcjr_bios.bin", 0x10000, 0x8000, CRC(c8fbef89) SHA1(2cb0a817b31400cdf27817d09bae7e69f41b062b) )	// bios vers. 1.0a
	// Not sure if we should support this: hacked version 1.5a by Chris Covell with bugfixes and GameGenie support
//  ROM_LOAD("drpcjr_v1_5_gg.bin", 0x10000, 0x8000, CRC(98f2033b) SHA1(93c114da787a19279d1a46667c2f69b49e25d4f1) )

	ROM_REGION( 0x2000,  "gfx1", ROMREGION_ERASE00 )  /* VROM */
	ROM_REGION( 0x4000,  "gfx2", ROMREGION_ERASE00 )  /* VRAM */
	ROM_REGION( 0x800,   "gfx3", ROMREGION_ERASE00 )  /* CI RAM */
ROM_END

ROM_START( dendy )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )  /* Main RAM + program banks */
	ROM_REGION( 0x2000,  "gfx1", ROMREGION_ERASE00 )  /* VROM */
	ROM_REGION( 0x4000,  "gfx2", ROMREGION_ERASE00 )  /* VRAM */
	ROM_REGION( 0x800,   "gfx3", ROMREGION_ERASE00 )  /* CI RAM */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT  COMPAT MACHINE   INPUT    INIT    COMPANY       FULLNAME */
CONS( 1985, nes,       0,      0,     nes,      nes,     0,       "Nintendo",  "Nintendo Entertainment System / Famicom (NTSC)", GAME_IMPERFECT_GRAPHICS )
CONS( 1987, nespal,    nes,    0,     nespal,   nes,     0,       "Nintendo",  "Nintendo Entertainment System (PAL)", GAME_IMPERFECT_GRAPHICS )
CONS( 1983, famicom,   nes,    0,     famicom,  famicom, famicom, "Nintendo",  "Famicom (w/ Disk System add-on)", GAME_IMPERFECT_GRAPHICS )
CONS( 1983, fami_key,  nes,    0,     famicom,  famikey, famicom, "Nintendo",  "Famicom (w/ Disk System add-on + Keyboard add-on)", GAME_IMPERFECT_GRAPHICS )
CONS( 1986, famitwin,  nes,    0,     famicom,  famicom, famicom, "Sharp",     "Famicom Twin", GAME_IMPERFECT_GRAPHICS )
CONS( 198?, m82,       nes,    0,     nes,      nes,     0,       "Nintendo",  "M82 Display Unit", GAME_IMPERFECT_GRAPHICS )
CONS( 1996, drpcjr,    nes,    0,     famicom,  famicom, famicom, "Bung",      "Doctor PC Jr", GAME_IMPERFECT_GRAPHICS )
CONS( 1992, dendy,     nes,    0,     dendy,    nes,     0,       "Steepler",  "Dendy Classic", GAME_IMPERFECT_GRAPHICS )
