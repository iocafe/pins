# pins-to-c.py 16.9.2019/pekka
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
    "timers" : "PIN_TIMER"}

prm_type_list = {
    "pull-up": "PIN_PULL_UP", 
    "touch": "PIN_TOUCH", 
    "frequency": "PIN_FREQENCY",
    "resolution": "PIN_RESOLUTION",
    "init": "PIN_INIT",
    "speed": "PIN_SPEED", 
    "delay": "PIN_DELAY"}

def start_c_files():
    global cfile, hfile, cfilepath, hfilepath
    cfile = open(cfilepath, "w")
    hfile = open(hfilepath, "w")
    cfile.write('/* This file is gerated by pins-to-c.py script, do not modify. */\n')
    cfile.write('#include "pins.h"\n\n')
    hfile.write('/* This file is gerated by pins-to-c.py script, do not modify. */\n')
    hfile.write('OSAL_C_HEADER_BEGINS\n\n')

def finish_c_files():
    global cfile, hfile
    hfile.write('\nOSAL_C_HEADER_ENDS\n')
    cfile.close()
    hfile.close()

def write_pin_to_c_header(pin_name):
    hfile.write("    Pin " + pin_name + ";\n")

def write_pin_to_c_source(pin_type, pin_name, pin_attr):
    global known_groups, prefix, ccontent
    global nro_pins, pin_nr

    # Generate C parameter list for the pin
    c_prm_list = ""
    for attr, value in pin_attr.items():
        c_attr_name = prm_type_list.get(attr, "")
        if c_attr_name is not "":
            if c_prm_list is not "":
                c_prm_list = c_prm_list + ", "    
                            
            c_prm_list = c_prm_list + c_attr_name + ", " + str(value)

    # If we have C attributes, write to C file
    c_prm_array_name = "OS_NULL"
    if c_prm_list is not "":
        c_prm_array_name = prefix + "_" + pin_type + "_" + pin_name + "_prm"
        cfile.write("static os_short " + c_prm_array_name + "[]")
        cfile.write("= {" + c_prm_list + "};\n")

    full_pin_name = prefix + '.' + pin_type + '.' + pin_name

    # Write pin name and type
    ccontent += '    {"' + pin_name + '", ' + pin_types[pin_type] + ", "

    # Write pin address, merge addr and bank
    bank = pin_attr.get("bank", "0")
    ccontent += str(bank) + ", "
    addr = pin_attr.get("addr", "0")
    ccontent += str(addr) + ", "

    # Write pointer to parameter array, if any
    ccontent += c_prm_array_name + ", "
    if c_prm_array_name is "OS_NULL":
        ccontent += "0, "
    else:
        ccontent += "sizeof(" + c_prm_array_name + ")/sizeof(os_short), "

    # If IO pin belongs to group, setup linked list
    group = pin_attr.get("group", None)
    if group is None:
        ccontent += "OS_NULL"
    else:
        g = known_groups.get(group, None)
        if g is None:
            g = "OS_NULL"
            known_groups.update( {group : full_pin_name} )

        else:
            known_groups[group] = full_pin_name
            g = "&" + g
            
        ccontent += g

    ccontent += "}"
    if pin_nr <= nro_pins:
        ccontent += ","
    ccontent += "\n"

def write_linked_list_heads():
    global prefix, known_groups

    for g, value in known_groups.items():
        varname = prefix + "_" +  g + "_group"
        cfile.write("const Pin *" + varname + " = &" + value + ";\n")
        hfile.write("extern const Pin *" + varname + ";\n")

def process_pin(pin_type, pin_attr):
    global device_name, ccontent
    global pin_nr

    pin_name = pin_attr.get("name", None)
    if pin_name == None:
        print("'name' not found for pin in " + device_name + " " + pin_type)
        exit()

    if pin_nr == 1:        
        ccontent += ', &' + prefix + '.' + pin_type + '.' + pin_name + '},\n'
    pin_nr = pin_nr + 1        

    write_pin_to_c_header(pin_name)
    write_pin_to_c_source(pin_type, pin_name, pin_attr)

def count_pins(pins):
    count = 0
    for pin in pins:
        count = count + 1
    return count        

def process_group_block(group):
    global nro_groups, group_nr, ccontent
    global nro_pins, pin_nr, pin_group_list

    hfile.write('\n  struct\n  {\n')
    hfile.write('    PinGroupHdr hdr;\n')

    pin_type = group.get("name", None)
    if pin_type == None:
        print("'name' not found for group in " + device_name)
        exit()

    pins = group.get("pins", None)
    if pins == None:
        print("'pins' not found for " + device_name + " " + pin_type)
        exit()

    group_nr = group_nr + 1

    pin_group_list.append(prefix + '.' + pin_type)

    nro_pins = count_pins(pins)
    pin_nr = 1

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

def process_io_device(io):
    global device_name, known_groups, prefix
    global nro_groups, group_nr, ccontent, pin_group_list

    device_name = io.get("name", "ioblock")
    groups = io.get("groups", None)
    prefix = io.get("prefix", "pins")
    pin_group_list = []

    hfile.write('typedef struct\n{')

    if groups == None:
        print("'groups' not found for " + device_name)
        exit()

    nro_groups = count_groups(groups)
    group_nr = 1;

    ccontent = '\nconst ' + prefix + '_t ' + prefix + ' =\n{'

    known_groups = {}

    for group in groups:
        process_group_block(group)

    ccontent += '};\n\n'
    cfile.write(ccontent)        

    list_name = prefix + "_group_list"
    cfile.write('static const PinGroupHdr *' + list_name + '[] =\n{\n  ')
    isfirst = True
    for p in pin_group_list:
        if not isfirst:
            cfile.write(',\n  ')
        isfirst = False
        cfile.write('&' + p + '.hdr')
    cfile.write('\n};\n\n')

    cfile.write('const IoPinsHdr pins_hdr = {' + list_name + ', sizeof(' + list_name + ')/' + 'sizeof(PinGroupHdr*)};\n')

    hfile.write('}\n' + prefix + '_t;\n\n')

    hfile.write('extern const IoPinsHdr ' + prefix + '_' + 'hdr;\n')
    hfile.write('extern const ' + prefix + '_t ' + prefix + ';\n')

    write_linked_list_heads()

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
    global cfilepath, hfilepath

    # Get options
    n = len(sys.argv)
    sourcefiles = []
    outpath = None
    expectpath = True
    for i in range(1, n):
        if sys.argv[i][0] is "-":
            if sys.argv[i][1] is "o":
                outpath = sys.argv[i+1]
                expectpath = False

        else:
            if expectpath:
                sourcefiles.append(sys.argv[i])

            expectpath = True

    if len(sourcefiles) < 1:
        print("No source files")
        exit()

#    sourcefiles.append('/coderoot/iocom/examples/gina/config/pins/carol/gina-io.json')
#    outpath = '/coderoot/iocom/examples/gina/config/include/carol/gina-io.c'

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
