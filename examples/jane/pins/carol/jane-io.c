#include "pins.h"
const Pin jane_LED_BUILTIN = {"LED_BUILTIN", PIN_OUTPUT, 1, 2, OS_NULL, 0, OS_NULL, OS_NULL};

const Pin *jane_pins = &jane_LED_BUILTIN;
