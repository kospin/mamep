/**********************************************************************

    Intel 8214 Priority Interrupt Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "i8214.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 0



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type I8214 = i8214_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(i8214, "I8214")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void i8214_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const i8214_interface *intf = reinterpret_cast<const i8214_interface *>(static_config());
	if (intf != NULL)
		*static_cast<i8214_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_int_func, 0, sizeof(m_out_int_func));
		memset(&m_out_enlg_func, 0, sizeof(m_out_enlg_func));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  trigger_interrupt -
//-------------------------------------------------

inline void i8214_device::trigger_interrupt(int level)
{
	if (LOG) logerror("I8214 '%s' Interrupt Level %u\n", tag(), level);

	m_a = level;

	// disable interrupts
	m_int_dis = 1;

	// disable next level group
	devcb_call_write_line(&m_out_enlg_func, 0);

	// toggle interrupt line
	devcb_call_write_line(&m_out_int_func, ASSERT_LINE);
	devcb_call_write_line(&m_out_int_func, CLEAR_LINE);
}


//-------------------------------------------------
//  check_interrupt -
//-------------------------------------------------

inline void i8214_device::check_interrupt()
{
	int level;

	if (m_int_dis || !m_etlg) return;

	for (level = 7; level >= 0; level--)
	{
		if (!BIT(m_r, 7 - level))
		{
			if (m_sgs)
			{
				if (level > m_b)
				{
					trigger_interrupt(level);
				}
			}
			else
			{
				trigger_interrupt(level);
			}
		}
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  i8214_device - constructor
//-------------------------------------------------

i8214_device::i8214_device(running_machine &_machine, const i8214_device_config &config)
    : device_t(_machine, config),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void i8214_device::device_start()
{
	// resolve callbacks
	devcb_resolve_write_line(&m_out_int_func, &m_config.m_out_int_func, this);
	devcb_resolve_write_line(&m_out_enlg_func, &m_config.m_out_enlg_func, this);

	// register for state saving
	save_item(NAME(m_inte));
	save_item(NAME(m_int_dis));
	save_item(NAME(m_a));
	save_item(NAME(m_b));
	save_item(NAME(m_r));
	save_item(NAME(m_sgs));
	save_item(NAME(m_etlg));
}


//-------------------------------------------------
//  a_r -
//-------------------------------------------------

UINT8 i8214_device::a_r()
{
	UINT8 a = m_a & 0x07;

	if (LOG) logerror("I8214 '%s' A: %01x\n", tag(), a);

	return a;
}


//-------------------------------------------------
//  b_w -
//-------------------------------------------------

void i8214_device::b_w(UINT8 data)
{
	m_b = data & 0x07;

	if (LOG) logerror("I8214 '%s' B: %01x\n", tag(), m_b);

	// enable interrupts
	m_int_dis = 0;

	// enable next level group
	devcb_call_write_line(&m_out_enlg_func, 1);

	check_interrupt();
}


//-------------------------------------------------
//  r_w -
//-------------------------------------------------

void i8214_device::r_w(UINT8 data)
{
	if (LOG) logerror("I8214 '%s' R: %02x\n", tag(), data);

	m_r = data;

	check_interrupt();
}


//-------------------------------------------------
//  sgs_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( i8214_device::sgs_w )
{
	if (LOG) logerror("I8214 '%s' SGS: %u\n", tag(), state);

	m_sgs = state;

	check_interrupt();
}


//-------------------------------------------------
//  etlg_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( i8214_device::etlg_w )
{
	if (LOG) logerror("I8214 '%s' ETLG: %u\n", tag(), state);

	m_etlg = state;
}


//-------------------------------------------------
//  inte_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( i8214_device::inte_w )
{
	if (LOG) logerror("I8214 '%s' INTE: %u\n", tag(), state);

	m_inte = state;
}
