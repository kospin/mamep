ADDRESS_MAP_EXTERN(timeplt_sound_readmem);
ADDRESS_MAP_EXTERN(timeplt_sound_writemem);
ADDRESS_MAP_EXTERN(locomotn_sound_readmem);
ADDRESS_MAP_EXTERN(locomotn_sound_writemem);
extern struct AY8910interface timeplt_ay8910_interface;
WRITE8_HANDLER( timeplt_sh_irqtrigger_w );
