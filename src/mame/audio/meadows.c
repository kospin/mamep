/***************************************************************************
    meadows.c
    Sound handler
    Dead Eye, Gypsy Juggler

    J. Buchmueller, June '98
****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"
#include "includes/meadows.h"
#include "sound/samples.h"
#include "sound/dac.h"



#define BASE_CLOCK		5000000
#define BASE_CTR1       (BASE_CLOCK / 256)
#define BASE_CTR2		(BASE_CLOCK / 32)

#define DIV2OR4_CTR2	0x01
#define ENABLE_CTR2     0x02
#define ENABLE_DAC      0x04
#define ENABLE_CTR1     0x08

static const INT16 waveform[2] = { -120*256, 120*256 };

/************************************/
/* Sound handler start              */
/************************************/
SAMPLES_START( meadows_sh_start )
{
	meadows_state *state = device->machine->driver_data<meadows_state>();
	state->_0c00 = state->_0c01 = state->_0c02 = state->_0c03 = 0;
	state->dac = 0;
	state->dac_enable = 0;
	state->channel = 0;
	state->freq1 = state->freq2 = 1000;
	state->latched_0c01 = state->latched_0c02 = state->latched_0c03 = 0;

	sample_set_volume(device,0,0);
	sample_start_raw(device,0,waveform,ARRAY_LENGTH(waveform),state->freq1,1);
	sample_set_volume(device,1,0);
	sample_start_raw(device,1,waveform,ARRAY_LENGTH(waveform),state->freq2,1);
}

/************************************/
/* Sound handler update             */
/************************************/
void meadows_sh_update(running_machine *machine)
{
	meadows_state *state = machine->driver_data<meadows_state>();
	device_t *samples = machine->device("samples");
	int preset, amp;

	if (state->latched_0c01 != state->_0c01 || state->latched_0c03 != state->_0c03)
	{
		/* amplitude is a combination of the upper 4 bits of 0c01 */
		/* and bit 4 merged from S2650's flag output */
		amp = ((state->_0c03 & ENABLE_CTR1) == 0) ? 0 : (state->_0c01 & 0xf0) >> 1;
		if( cpu_get_reg(machine->device("maincpu"), S2650_FO) )
			amp += 0x80;
		/* calculate frequency for counter #1 */
		/* bit 0..3 of 0c01 are ctr preset */
		preset = (state->_0c01 & 15) ^ 15;
		if (preset)
			state->freq1 = BASE_CTR1 / (preset + 1);
		else amp = 0;
		logerror("meadows ctr1 channel #%d preset:%3d freq:%5d amp:%d\n", state->channel, preset, state->freq1, amp);
		sample_set_freq(samples, 0, state->freq1 * sizeof(waveform)/2);
		sample_set_volume(samples, 0,amp/255.0);
	}

	if (state->latched_0c02 != state->_0c02 || state->latched_0c03 != state->_0c03)
	{
		/* calculate frequency for counter #2 */
		/* 0c02 is ctr preset, 0c03 bit 0 enables division by 2 */
		amp = ((state->_0c03 & ENABLE_CTR2) != 0) ? 0xa0 : 0;
		preset = state->_0c02 ^ 0xff;
		if (preset)
		{
			state->freq2 = BASE_CTR2 / (preset + 1) / 2;
			if ((state->_0c03 & DIV2OR4_CTR2) == 0)
				state->freq2 >>= 1;
		}
		else amp = 0;
		logerror("meadows ctr2 channel #%d preset:%3d freq:%5d amp:%d\n", state->channel+1, preset, state->freq2, amp);
		sample_set_freq(samples, 1, state->freq2 * sizeof(waveform));
		sample_set_volume(samples, 1,amp/255.0);
	}

	if (state->latched_0c03 != state->_0c03)
	{
		state->dac_enable = state->_0c03 & ENABLE_DAC;

		if (state->dac_enable)
			dac_data_w(machine->device("dac"), state->dac);
		else
			dac_data_w(machine->device("dac"), 0);
	}

	state->latched_0c01 = state->_0c01;
	state->latched_0c02 = state->_0c02;
	state->latched_0c03 = state->_0c03;
}

/************************************/
/* Write DAC value                  */
/************************************/
void meadows_sh_dac_w(running_machine *machine, int data)
{
	meadows_state *state = machine->driver_data<meadows_state>();
	state->dac = data;
	if (state->dac_enable)
		dac_data_w(machine->device("dac"), state->dac);
	else
		dac_data_w(machine->device("dac"), 0);
}


