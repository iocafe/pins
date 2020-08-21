# pins_to_c.py 17.8.2020/pekka
# Converts hardware IO specification(s) written in JSON to C source and header files.
import json
import os
import sys

pin_types = {
    "inputs" : "PIN_INPUT",
    "outputs" : "PIN_OUTPUT",
    "analog_inputs" : "PIN_ANALOG_INPUT",
    "analog_outputs" : "PIN_ANALOG_OUTPUT",
    "pwm" : "PIN_PWM",
    "spi" : "PIN_SPI",
    "timers" : "PIN_TIMER",
    "cameras" : "PIN_CAMERA",
    "uart" : "PIN_UART"}

prm_type_list = {
    "pull-up": "PIN_PULL_UP",
    "pull-down": "PIN_PULL_DOWN",
    "touch": "PIN_TOUCH",
    "frequency": "PIN_FREQENCY",
    "frequency-kHz": "PIN_FREQENCY_KHZ",
    "resolution": "PIN_RESOLUTION",
    "init": "PIN_INIT",
    "hpoint": "PIN_HPOINT",
    "interrupt": "PIN_INTERRUPT_ENABLED",
    "timer": "PIN_TIMER_SELECT",
    "tgroup": "PIN_TIMER_GROUP_SELECT",
    "miso": "PIN_MISO",
    "mosi": "PIN_MOSI",
    "sclk": "PIN_SCLK",
    "cs": "PIN_CS",
    "dc": "PIN_DC",
    "rx": "PIN_RX",
    "tx": "PIN_TX",
    "tc": "PIN_TRANSMITTER_CTRL",
    "speed": "PIN_SPEED",
    "pin-a": "PIN_A",
    "pin-b": "PIN_B",
    "pin-c": "PIN_C",
    "pin-d": "PIN_D",
    "pin-e": "PIN_E",
    "bank-a": "PIN_A_BANK",
    "bank-b": "PIN_B_BANK",
    "bank-c": "PIN_C_BANK",
    "bank-d": "PIN_D_BANK",
    "bank-e": "PIN_E_BANK",
    "min": "PIN_MIN",
    "max": "PIN_MAX"}

def start_c_files():
    global cfile, hfile, cfilepath, hfilepath
    cfile = open(cfilepath, "w")
    hfile = open(hfilepath, "w")
    cfile.write('/* This file is generated by pins_to_c.py script, do not modify. */\n')
    cfile.write('#include "pins.h"\n')
    hfile.write('/* This file is generated by pins_to_c.py script, do not modify. */\n')
    path, fname = os.path.split(hfilepath)
    fname, ext = os.path.splitext(fname)
    macroname = 'IOC_' + fname.upper() + '_INCLUDED'
    hfile.write('#ifndef ' + macroname + '\n')
    hfile.write('#define ' + macroname + '\n')
    hfile.write('OSAL_C_HEADER_BEGINS\n\n')

def finish_c_files():
    global cfile, hfile
    hfile.write('\nOSAL_C_HEADER_ENDS\n')
    hfile.write('#endif\n')
    cfile.close()
    hfile.close()

def write_pin_to_c_header(pin_name):
    hfile.write("    Pin " + pin_name + ";\n")

def write_pin_to_c_source(pin_type, pin_name, pin_attr):
    global known_groups, prefix, ccontent, c_prm_comment_written
    global nro_pins, pin_nr, define_list, device_list, driver_list, bus_list

    # Generate C parameter list for the pin
    c_prm_list = "PIN_RV, PIN_RV"
    c_prm_list_has_interrupt = False
    for attr, value in pin_attr.items():
        c_attr_name = prm_type_list.get(attr, "")
        if c_attr_name != "":
            if c_prm_list != "":
                c_prm_list = c_prm_list + ", "

            c_prm_list = c_prm_list + c_attr_name + ", "
            if c_attr_name == 'PIN_SPEED':
                c_prm_list = c_prm_list + str(int(value)//100)
            else:
                c_prm_list = c_prm_list + str(value)

            if c_attr_name == 'PIN_INTERRUPT_ENABLED':
                c_prm_list_has_interrupt = True

        elif attr != 'name' and attr != 'addr' and attr != 'bank' and attr != 'group' and attr != 'device' and attr != 'driver':
            print("Pin '" + pin_name + "' has unknown attribute '" + attr + "', ignored.")

    if c_prm_list_has_interrupt == False and pin_type == 'timers':
        c_prm_list_has_interrupt = True
        if c_prm_list != "":
            c_prm_list = c_prm_list + ", "
        c_prm_list = c_prm_list + "PIN_INTERRUPT_ENABLED, 1"

    # If we have C parameters, write to C file
    c_prm_array_name = "OS_NULL"
    if c_prm_list != "":
        if c_prm_comment_written == False:
            cfile.write("\n/* Parameters for " + pin_type + " */\n")
            c_prm_comment_written = True
        c_prm_array_name = prefix + "_" + pin_type + "_" + pin_name + "_prm"
        cfile.write("static os_ushort " + c_prm_array_name + "[]")
        cfile.write("= {" + c_prm_list + "};\n")

    define_text = prefix + '_' + pin_type + '_' + pin_name
    define_list.append(define_text.upper() + ' "' + pin_name + '"')

    full_pin_name = prefix + '.' + pin_type + '.' + pin_name

    # Write pin type
    ccontent += '    {' + pin_types[pin_type] + ", "

    # Write pin address, merge addr and bank
    bank = pin_attr.get("bank", "0")
    ccontent += str(bank) + ", "
    addr = pin_attr.get("addr", "0")
    ccontent += str(addr) + ", "

    # Write pointer to parameter array, if any
    ccontent += c_prm_array_name + ", "
    if c_prm_array_name == "OS_NULL":
        ccontent += "0, "
    else:
        ccontent += "sizeof(" + c_prm_array_name + ")/sizeof(os_ushort), "

    # If IO pin belongs to group, setup linked list
    group = pin_attr.get("group", None)
    if group is None:
        ccontent += "OS_NULL"
    else:
        g = known_groups.get(group, None)
        if g is None:
            g = "OS_NULL"
            known_groups.update( {group : full_pin_name} )
            define_text = prefix + '_' + group + '_GROUP'
            define_list.append(define_text.upper() + ' "' + group + '"')

        else:
            known_groups[group] = full_pin_name
            g = "&" + g

        ccontent += g

    if pin_name in signallist:
        ccontent += ', ' + signallist[pin_name]
    else:
        ccontent += ', OS_NULL'

    # If IO pin is on SPI or I2C device
    bus_device = pin_attr.get("device", None)
    if bus_device != None:
        ccontent += ' PINS_DEVCONF_PTR('
        ccontent += 'pins_device_' + bus_device.replace('.',  '_')
        ccontent += ')'

    else:
        ccontent += ' PINS_DEVCONF_NULL'

    # If IO pin is a SPI or I2C device
    driver = pin_attr.get("driver", None)
    if driver != None:
        if pin_type == 'i2c':
            bus_id = pin_type + '_' + str(pin_attr.get('sclk', 0))
        else:
            bus_id = pin_type + '_' + str(pin_attr.get('sclk', 0))

        next_device = bus_list.get(bus_id, 'OS_NULL')
        bus_list[bus_id] = '&' + 'pins_device_' + pin_type + '_' + pin_name
        device_list[pin_name] = (driver, pin_type, pin_name, next_device, bus_id)
        driver_list[driver] = 'x'

    if c_prm_list_has_interrupt:
        intconf_struct_name = "pin_" + pin_name + "_intconf"
        cfile.write("PINS_INTCONF_STRUCT(")
        cfile.write(intconf_struct_name)
        cfile.write(")\n")
        ccontent += ' PINS_INTCONF_PTR('
        ccontent += intconf_struct_name
        ccontent += ')'
    else:
        ccontent += ' PINS_INTCONF_NULL'

    ccontent += "}"
    if pin_nr <= nro_pins:
        ccontent += ","

    ccontent += ' /* ' + pin_name + ' */\n'

def write_device_list(device_list, driver_list, bus_list):
    global cfile, hfile

    cfile.write('\n#if PINS_SPI || PINS_I2C\n');
    cfile.write('\n/* SPI and I2C bus structures */\n');
    next_bus = 'OS_NULL'
    for bus_name, data in bus_list.items():
        cfile.write('PinsBus pins_bus_' + bus_name + ' = {')
        if bus_name[:3] == 'spi':
            cfile.write('PINS_SPI_BUS, ')
        else:            
            cfile.write('PINS_I2C_BUS, ')
        cfile.write(data + ', ' + next_bus + '};\n')
        next_bus = '&pins_bus_' + bus_name

    cfile.write('\n/* Device bus main structure */\n');
    cfile.write('PinsDeviceBus pins_devicebus = {' + next_bus + '};\n')

    cfile.write('\n/* SPI and I2C device structures */\n');
    for device_name, data in device_list.items():
        cfile.write('PinsBusDevice pins_device_' + data[1] + '_' + data[2] + ' = {')
        cfile.write('&' + prefix + '.' + data[1] + '.' + data[2] + ', ')
        cfile.write('&pins_bus_' +  data[4] + ', ')
        cfile.write(data[3] + ', ')
        cfile.write('&' + data[0] + '_gen_req, ')
        cfile.write('&' + data[0] + '_proc_resp, ')
        cfile.write('&' + data[0] + '_set, ')
        cfile.write('&' + data[0] + '_get};\n')

    cfile.write('\n/* Initialize all SPI and I2C bus devices */\n');
    cfile.write('void pins_initialize_bus_devices(void)\n{\n')
    for bus_name, data in bus_list.items():
        cfile.write('    pins_init_bus(&pins_bus_' + bus_name + ');\n')
    for driver_name, data in driver_list.items():
        cfile.write('    ' + driver_name + '_initialize_driver();\n')
    for device_name, data in device_list.items():
        cfile.write('    ' + data[0] + '_initialize(&pins_device_' + data[1] + '_' + data[2] + ');\n')
    cfile.write('}\n');

    cfile.write('#endif\n');

    hfile.write('\n/* SPI and I2C initialization */\n');
    hfile.write('#if PINS_SPI || PINS_I2C\n');

    # Moved to pins_devicebus.h
    # hfile.write('\n/* Device bus main structure */\n');
    # hfile.write('extern PinsDeviceBus pins_devicebus;\n')

    hfile.write('\n/* SPI and I2C bus structures */\n');
    for bus_name, data in bus_list.items():
        hfile.write('extern PinsBus pins_bus_' + bus_name + ';\n')

    hfile.write('\n/* SPI and I2C device structures */\n');
    for device_name, data in device_list.items():
        hfile.write('extern PinsBusDevice pins_device_' + data[1] + '_' + data[2] + ';\n')

    for device_name, data in device_list.items():
        hfile.write('\n/* ' + data[0] + ' driver functions  */\n');
        hfile.write('void ' + data[0] + '_initialize_driver(void);\n')
        hfile.write('void ' + data[0] + '_initialize(struct PinsBusDevice *device);\n')
        hfile.write('void ' + data[0] + '_gen_req(struct PinsBusDevice *device);\n')
        hfile.write('osalStatus ' + data[0] + '_proc_resp(struct PinsBusDevice *device);\n')
        hfile.write('void ' + data[0] + '_set(struct PinsBusDevice *device, os_short addr, os_int value);\n')
        hfile.write('os_int ' + data[0] + '_get(struct PinsBusDevice *device, os_short addr);\n')

    # for driver_name, func_decl in driver_list.items():
    #     hfile.write(func_decl)
    hfile.write('#endif\n');

def write_linked_list_heads():
    global prefix, known_groups

    isfirst = True
    for g, value in known_groups.items():
        varname = prefix + "_" +  g + "_group"
        if isfirst:
            cfile.write("\n/* Application's pin groups (linked list heads) */\n")
            hfile.write("\n/* Application's pin groups (linked list heads) */\n")
            isfirst = False
        cfile.write("OS_FLASH_MEM Pin *" + varname + " = &" + value + ";\n")
        hfile.write("extern OS_FLASH_MEM_H Pin *" + varname + ";\n")

def process_pin(pin_type, pin_attr):
    global device_name, ccontent
    global pin_nr

    pin_name = pin_attr.get("name", None)
    if pin_name == None:
        print("'name' not found for pin in " + device_name + " " + pin_type)
        exit()

    if pin_nr == 1:
        ccontent += ', &' + prefix + '.' + pin_type + '.' + pin_name + '}, /* ' + pin_type + ' */\n'
    pin_nr = pin_nr + 1

    write_pin_to_c_header(pin_name)
    write_pin_to_c_source(pin_type, pin_name, pin_attr)

def count_pins(pins):
    count = 0
    for pin in pins:
        count = count + 1
    return count

def process_group_block(group):
    global nro_groups, group_nr, ccontent, c_prm_comment_written
    global nro_pins, pin_nr, pin_group_list

    pin_type = group.get("name", None)
    if pin_type == None:
        print("'name' not found for group in " + device_name)
        exit()

    if pin_types.get(pin_type, None) == None:
        print("Pin group '"+ pin_type + "' ignored.")
        return;

    hfile.write('\n  struct\n  {\n')
    hfile.write('    PinGroupHdr hdr;\n')

    pins = group.get("pins", None)
    if pins == None:
        print("'pins' not found for " + device_name + " " + pin_type)
        exit()

    group_nr = group_nr + 1

    pin_group_list.append(prefix + '.' + pin_type)

    nro_pins = count_pins(pins)
    pin_nr = 1
    c_prm_comment_written = False

    ccontent += '\n  {{' + str(nro_pins)

    for pin in pins:
        process_pin(pin_type, pin)

    ccontent += '  }'
    if group_nr <= nro_groups:
        ccontent += ','
    ccontent += '\n'

    hfile.write('  }\n  ' + pin_type + ';\n')

def count_groups(groups):
    count = 0
    for group in groups:
        count = count + 1
    return count

def list_signals_in_mblk(mblk, device_name):
    global signallist
    groups = mblk.get("groups", None)
    mblk_name = mblk.get("name", None);

    for group in groups:
        signals  = group.get("signals", None);
        if signals != None:
            for signal in signals:
                signal_name = signal.get('name', None)
                if signal_name is not None:
                    signallist.update({signal_name : '&' + device_name + '.' + mblk_name + '.' + signal_name})

def list_signals_in_file(path):
    signals_file = open(path, "r")
    if signals_file:
        data = json.load(signals_file)
        device_name = data.get("name", None)
        if device_name == None:
            print("device 'name' not found in " + path)
            return
        mblks = data.get("mblk", None)
        if mblks == None:
            print("'mblk' not found in "  + path)
            return

        for mblk in mblks:
            list_signals_in_mblk(mblk, device_name)

    else:
        printf ("Opening file " + path + " failed")

def process_io_device(io):
    global device_name, known_groups, prefix, signallist, device_list, driver_list, bus_list
    global nro_groups, group_nr, ccontent, pin_group_list, define_list

    device_name = io.get("name", "ioblock")
    groups = io.get("groups", None)
    prefix = io.get("prefix", "pins")
    pin_group_list = []
    define_list = []

    signallist = {}
    if signalspath != None:
        list_signals_in_file(signalspath)

    device_list = {}
    driver_list = {}
    bus_list = {}

    hfile.write("/* " + device_name.upper() + " IO configuration structure */\n")
    hfile.write('typedef struct\n{')

    if groups == None:
        print("'groups' not found for " + device_name)
        exit()

    nro_groups = count_groups(groups)
    group_nr = 1;

    ccontent = "\n/* " + device_name.upper() + " IO configuration structure */\n"
    ccontent += 'OS_FLASH_MEM ' + prefix + '_t ' + prefix + ' =\n{'

    known_groups = {}

    for group in groups:
        process_group_block(group)

    ccontent += '};\n\n'
    cfile.write(ccontent)

    list_name = prefix + "_group_list"
    cfile.write('/* List of pin type groups */\n')
    cfile.write('static OS_FLASH_MEM PinGroupHdr * OS_FLASH_MEM ' + list_name + '[] =\n{\n  ')
    isfirst = True
    for p in pin_group_list:
        if not isfirst:
            cfile.write(',\n  ')
        isfirst = False
        cfile.write('&' + p + '.hdr')
    cfile.write('\n};\n\n')

    cfile.write('/* ' + device_name.upper() + ' IO configuration top header structure */\n')
    cfile.write('OS_FLASH_MEM IoPinsHdr pins_hdr = {' + list_name + ', sizeof(' + list_name + ')/' + 'sizeof(PinGroupHdr*)};\n')

    hfile.write('}\n' + prefix + '_t;\n\n')

    hfile.write("/* " + device_name.upper() + " IO configuration top header structure */\n")
    hfile.write('extern OS_FLASH_MEM_H IoPinsHdr ' + prefix + '_' + 'hdr;\n\n')

    hfile.write("/* Global " + device_name.upper() + " IO configuration structure */\n")
    hfile.write('extern OS_FLASH_MEM_H ' + prefix + '_t ' + prefix + ';\n')

    write_linked_list_heads()

    hfile.write("\n/* Name defines for pins and application pin groups (use ifdef to check if HW has pin) */\n")
    for d in define_list:
        hfile.write('#define ' +d + '\n')

    write_device_list(device_list, driver_list, bus_list)        

def process_source_file(path):
    read_file = open(path, "r")
    if read_file:
        data = json.load(read_file)
        ioroot = data.get("io", None)
        if ioroot == None:
            print("'io' not found")
            exit()

        for io in ioroot:
            process_io_device(io)

    else:
        printf ("Opening file " + path + " failed")

def mymain():
    global cfilepath, hfilepath, signalspath

    # Get options
    n = len(sys.argv)
    sourcefiles = []
    outpath = None
    signalspath = None
    expectpath = True
    for i in range(1, n):
        if sys.argv[i][0] == "-":
            if sys.argv[i][1] == "o":
                outpath = sys.argv[i+1]
                expectpath = False

            if sys.argv[i][1] == "s":
                signalspath = sys.argv[i+1]
                expectpath = False

        else:
            if expectpath:
                sourcefiles.append(sys.argv[i])

            expectpath = True

    if len(sourcefiles) < 1:
        print("No source files")
        exit()

#    sourcefiles.append('/coderoot/iocom/examples/buster/config/pins/grumpy/pins_io.json')
#    outpath = '/coderoot/iocom/examples/buster/config/include/grumpy/pins_io.c'

#    sourcefiles.append('/coderoot/dehec-ref/dref/config/pins/yogurt/pins_io.json')
#    outpath = '/coderoot/dehec-ref/dref/config/include/yogurt/pins_io.c'

#     sourcefiles.append('/coderoot/pins/examples/jane/config/pins/carol/pins_io.json')
#     outpath = '/coderoot/pins/examples/jane/config/include/carol/pins_io.c'

#    sourcefiles.append('/coderoot/iocom/examples/gina/config/pins/carol/gina-io.json')
#    outpath = '/coderoot/iocom/examples/gina/config/include/carol/gina-io.c'
#    signalspath = '/coderoot/iocom/examples/gina/config/signals/gina-signals.json'

    if outpath is None:
        outpath = sourcefiles[0]

    filename, file_extension = os.path.splitext(outpath)
    cfilepath = filename + '.c'
    hfilepath = filename + '.h'

    print("Writing files " + cfilepath + " and " + hfilepath)

    start_c_files()

    for path in sourcefiles:
        process_source_file(path)

    finish_c_files()

mymain()
