/* This file is gerated by signals-to-c.py script, do not modify. */
OSAL_C_HEADER_BEGINS

typedef struct gina_t
{
  struct 
  {
    iocMblkSignalHdr hdr;
    iocSignal dip_switch_3;
    iocSignal dip_switch_4;
    iocSignal touch_sensor;
    iocSignal potentiometer;
  }
  up;

  struct 
  {
    iocMblkSignalHdr hdr;
    iocSignal servo;
    iocSignal dimmer_led;
    iocSignal led_builtin;
    iocSignal seven_segment;
  }
  down;
}
gina_t;

#define GINA_UP_MBLK_SZ 36
#define GINA_DOWN_MBLK_SZ 32

extern gina_t gina;
extern const iocDeviceHdr gina_hdr;

#define GINA_DOWN_SEVEN_SEGMENT_ARRAY_SZ 8

OSAL_C_HEADER_ENDS
