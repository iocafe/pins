/**

  @file    esp32/pins_esp32_gpio.c
  @brief   GPIO pins.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "pins.h"
// #include "Arduino.h"

#ifdef ESP_PLATFORM
#include "driver/gpio.h"
// #include "driver/ledc.h"
#endif


static void pin_gpio_global_interrupt_control(
    os_boolean enable,
    void *context);

static void pin_set_interrupt_enable_flag(
    const struct Pin *pin,
    os_boolean enable,
    os_int flag);

static void pin_control_pin_interrupt(
    const struct Pin *pin);

/**
****************************************************************************************************

  @brief Setup a pin as input.
  @anchor pin_gpio_setup_input

  The pin_gpio_setup_input() function...
  @return  None.

****************************************************************************************************
*/
void pin_gpio_setup_input(
    const Pin *pin)
{
#ifdef ESP_PLATFORM
    gpio_config_t io_conf;
    os_memclear(&io_conf, sizeof(io_conf));

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << pin->addr;
    io_conf.pull_down_en = pin_get_prm(pin, PIN_PULL_DOWN);
    io_conf.pull_up_en = pin_get_prm(pin, PIN_PULL_UP);
    gpio_config(&io_conf);
#else
    in mode = INPUT;
    if (pin_get_prm(pin, PIN_PULL_UP)) mode = INPUT_PULLUP;
    if (pin_get_prm(pin, PIN_PULL_DOWN)) mode = INPUT_PULLDOWN;
    pinMode(pin->addr, mode);
#endif
}


/**
****************************************************************************************************

  @brief Setup a pin as output.
  @anchor pin_gpio_setup_output

  The pin_gpio_setup_output() function...
  @return  None.

****************************************************************************************************
*/
void pin_gpio_setup_output(
    const Pin *pin)
{
#ifdef ESP_PLATFORM
    gpio_config_t io_conf;
    os_memclear(&io_conf, sizeof(io_conf));

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1ULL << pin->addr;
    gpio_config(&io_conf);
#else
    pinMode(pin->addr, OUTPUT);
#endif
}


/**
****************************************************************************************************

  @brief Attach an interrupt to a GPIO pin.
  @anchor pin_gpio_attach_interrupt

  The pin_gpio_attach_interrupt() function to set an interrupt handler function on a pin by pin basis.

  There may be HW specific parameters and limitations for reserving interrupt channels, etc.
  The HW specific parameters are in JSON file for the hardware.

  @param   pin The GPIO pin structure.
  @param   prm Parameter structure, contains pointer to interrupt handler functio that
           will be called every time the interrupt is triggered.
  @param   flags Flags to specify when interrupt is triggered.
  @return  None.

****************************************************************************************************
*/
void pin_gpio_attach_interrupt(
    const struct Pin *pin,
    pinInterruptParams *prm)
{
#ifdef ESP_PLATFORM
    gpio_int_type_t itype;
    gpio_num_t addr;
    os_boolean enable;
    addr = (gpio_num_t)pin->addr;

    gpio_isr_handler_add(addr,  (gpio_isr_t)(prm->int_handler_func), NULL);

    switch (prm->flags & PINS_INT_CHANGE)
    {
        case PINS_INT_FALLING: itype = GPIO_INTR_NEGEDGE; break;
        case PINS_INT_RISING:  itype = GPIO_INTR_POSEDGE; break;
        default:
        case PINS_INT_CHANGE: itype = GPIO_INTR_ANYEDGE; break;
    }
    gpio_set_intr_type(addr, itype);

    /* Start listening for global interrupt enable functions.
       It is important to clear PIN_INTERRUPT_ENABLED for soft reboot.
     */
    pin_set_prm(pin, PIN_INTERRUPT_ENABLED, 0);
    enable = osal_add_interrupt_to_list(pin_gpio_global_interrupt_control, (void*)pin);
    pin_set_interrupt_enable_flag(pin, enable, PIN_GLOBAL_INTERRUPTS_ENABLED);
    pin_set_interrupt_enable_flag(pin, OS_TRUE, PIN_INTERRUPTS_ENABLED_FOR_PIN);

    pin_control_pin_interrupt(pin);
    // gpio_intr_enable(addr);

#else
    int mode;

    switch (prm->flags & PINS_INT_CHANGE)
    {
        case PINS_INT_FALLING: mode = FALLING; break;
        case PINS_INT_RISING:  mode = RISING; break;
        default:
        case PINS_INT_CHANGE: mode = CHANGE; break;
    }

    attachInterrupt(pin->addr, prm->int_handler_func, mode);
#endif
}


/**
****************************************************************************************************

  @brief Detach interrupt from GPIO pin.
  @anchor pin_gpio_detach_interrupt

  You can call pin_gpio_detach_interrupt() function when you no longer want to receive interrupts
  related to the pin.

  @return  None.

****************************************************************************************************
*/
void pin_gpio_detach_interrupt(
    const struct Pin *pin)
{
#ifdef ESP_PLATFORM
    pin_set_interrupt_enable_flag(pin, OS_FALSE, PIN_INTERRUPTS_ENABLED_FOR_PIN);
    pin_control_pin_interrupt(pin);

    // gpio_intr_disable(addr);
    // gpio_isr_handler_remove(addr);

#else
    detachInterrupt(pin->addr);
#endif
}

/* Global enable/disable interrupts callback for flash writes.
 */
static void pin_gpio_global_interrupt_control(
    os_boolean enable,
    void *context)
{
    const struct Pin *pin;
    pin = (const struct Pin*)context;

    pin_set_interrupt_enable_flag(pin, enable, PIN_GLOBAL_INTERRUPTS_ENABLED);
    pin_control_pin_interrupt(pin);
}

/*
 @param  flag PIN_GLOBAL_INTERRUPTS_ENABLED or PIN_INTERRUPTS_ENABLED_FOR_PIN
*/
static void pin_set_interrupt_enable_flag(
    const struct Pin *pin,
    os_boolean enable,
    os_int flag)
{
    os_int x;

    x = pin_get_prm(pin, PIN_INTERRUPT_ENABLED);
    if (enable) {
        x |= flag;
    }
    else {
        x &= ~flag;
    }
    pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
}


static void pin_control_pin_interrupt(
    const struct Pin *pin)
{
    os_int x;
    x = pin_get_prm(pin, PIN_INTERRUPT_ENABLED);
    if ((x & (PIN_GLOBAL_INTERRUPTS_ENABLED|PIN_INTERRUPTS_ENABLED_FOR_PIN))
        == (PIN_GLOBAL_INTERRUPTS_ENABLED|PIN_INTERRUPTS_ENABLED_FOR_PIN))
    {
        if ((x & PIN_GPIO_PIN_INTERRUPTS_ENABLED) == 0)
        {
            gpio_intr_enable((gpio_num_t)(pin->addr));
            x |= PIN_GPIO_PIN_INTERRUPTS_ENABLED;
            pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
        }
    }
    else
    {
        if (x & PIN_GPIO_PIN_INTERRUPTS_ENABLED)
        {
            gpio_intr_disable((gpio_num_t)(pin->addr));
            x &= ~PIN_GPIO_PIN_INTERRUPTS_ENABLED;
            pin_set_prm(pin, PIN_INTERRUPT_ENABLED, x);
        }
    }
}


/* int IRAM_ATTR local_adc1_read_test(int channel) {
    uint16_t adc_value;
    SENS.sar_meas_start1.sar1_en_pad = (1 << channel); // only one channel is selected
    while (SENS.sar_slave_addr1.meas_status != 0);
    SENS.sar_meas_start1.meas1_start_sar = 0;
    SENS.sar_meas_start1.meas1_start_sar = 1;
    while (SENS.sar_meas_start1.meas1_done_sar == 0);
    adc_value = SENS.sar_meas_start1.meas1_data_sar;
    return adc_value;
} */
