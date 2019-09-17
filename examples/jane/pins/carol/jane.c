static os_short jane_LIGHT_SWITCH_prm[]= {PIN_SPEED, 3};
const Pin jane_LIGHT_SWITCH = {"LIGHT_SWITCH", PIN_INPUT, 4, 2, jane_LIGHT_SWITCH_prm, sizeof(jane_LIGHT_SWITCH_prm)/sizeof(os_short), OS_NULL, OS_NULL};
const Pin jane_GARAGE_DOOR_SWITCH = {"GARAGE_DOOR_SWITCH", PIN_INPUT, 1, 5, OS_NULL, 0, OS_NULL, jane_LIGHT_SWITCH};

const Pin jane_WORK_LIGHT = {"WORK_LIGHT", PIN_OUTPUT, 1, 3, OS_NULL, 0, OS_NULL, jane_GARAGE_DOOR_SWITCH};
const Pin jane_NIGHT_LIGHT = {"NIGHT_LIGHT", PIN_OUTPUT, 1, 5, OS_NULL, 0, jane_WORK_LIGHT, jane_WORK_LIGHT};

static os_short jane_TEMPERATURE_SENSOR_prm[]= {PIN_SPEED, 3, PIN_DELAY, 11};
const Pin jane_TEMPERATURE_SENSOR = {"TEMPERATURE_SENSOR", PIN_ANALOG_INPUT, 2, 2, jane_TEMPERATURE_SENSOR_prm, sizeof(jane_TEMPERATURE_SENSOR_prm)/sizeof(os_short), OS_NULL, jane_NIGHT_LIGHT};

const Pin *jane_MYLIGHTS_group = jane_NIGHT_LIGHT;
const Pin *jane_pins = jane_TEMPERATURE_SENSOR;
