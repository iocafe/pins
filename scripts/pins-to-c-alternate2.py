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
    cfile.write('#include "pins.h"\n')
    hfile.write('/* This file is gerated by pins-to-c.py script, do not modify. */\n')
    hfile.write('OSAL_C_HEADER_BEGINS\n')

def finish_c_files():
    global cfile, hfile
    hfile.write('OSAL_C_HEADER_ENDS\n')
    cfile.close()
    hfile.close()

def write_pin_to_c_header(pin_name):
    global prefix
    hfile.write("extern const Pin " + prefix + pin_name + ";\n")

def write_pin_to_c_source(pin_type, pin_name, pin_attr):
    global c_prev_pin_name, known_groups, prefix

    # Generate C parameter list for the pin
    c_prm_list = ""
    for attr, value in pin_attr.items():
        c_attr_name = prm_type_list.get(attr, "")
        if c_attr_name is not "":
            if c_prm_list is not "":
                c_prm_list = c_prm_list + ", "    
                            
            c_prm_list = c_prm_list + c_attr_name + ", " + str(value);

    # If we have C attributes, write to C file
    c_prm_array_name = "OS_NULL"
    if c_prm_list is not "":
        c_prm_array_name = prefix + pin_name + "_prm"
        cfile.write("static os_short " + c_prm_array_name + "[]")
        cfile.write("= {" + c_prm_list + "};\n")

    # Write pin name and type
    full_pin_name = prefix + pin_name
    cfile.write("const Pin " + full_pin_name + ' = {"')
    cfile.write(pin_name + '", ' + pin_types[pin_type] + ", ")

    # Write pin address, merge addr and bank
    bank = pin_attr.get("bank", "0")
    cfile.write(str(bank) + ", ")
    addr = pin_attr.get("addr", "0")
    cfile.write(str(addr) + ", ")

    # Write pointer to parameter array, if any
    cfile.write(c_prm_array_name + ", ")
    if c_prm_array_name is "OS_NULL":
        cfile.write("0, ")
    else:
        cfile.write("sizeof(" + c_prm_array_name + ")/sizeof(os_short), ")

    # If IO pin belongs to group, setup linked list
    group = pin_attr.get("group", None)
    if group is None:
        cfile.write("OS_NULL")
    else:
        g = known_groups.get(group, None)
        if g is None:
            g = "OS_NULL"
            known_groups.update( {group : full_pin_name} )

        else:
            known_groups[group] = full_pin_name
            g = "&" + g;
            
        cfile.write(g)

    # Setup linked list for all pins in this definition block
    cfile.write(", " + c_prev_pin_name)
    c_prev_pin_name = "&" + full_pin_name
    cfile.write("};\n")

def write_linked_list_heads():
    global prefix, c_prev_pin_name, known_groups

    for g, value in known_groups.items():
        varname = prefix + g + "_group";
        cfile.write("const Pin *" + varname + " = &" + value + ";\n")
        hfile.write("extern const Pin *" + varname + ";\n")

    if c_prev_pin_name is not "OS_NULL":
        varname = prefix + "pins";
        cfile.write("const Pin *" + varname + " = " + c_prev_pin_name + ";\n")
        hfile.write("extern const Pin *" + varname + ";\n")

def process_pin(pin_type, pin_attr):
    global block_name
    pin_name = pin_attr.get("name", None)
    if pin_name == None:
        print("'name' not found for pin in " + block_name + " " + pin_type)
        exit()
    write_pin_to_c_header(pin_name)
    write_pin_to_c_source(pin_type, pin_name, pin_attr)

def process_group_block(group):
    global pin_types
    pin_type = group.get("name", None);
    if pin_type == None:
        print("'name' not found for group in " + block_name)
        exit()
    pins = group.get("pins", None);
    if pins == None:
        print("'pins' not found for " + block_name + " " + pin_type)
        exit()

    for pin in pins:
        process_pin(pin_type, pin)

def process_io_block(io):
    global block_name, c_prev_pin_name, known_groups, prefix

    block_name = io.get("name", "ioblock")
    groups = io.get("groups", None)
    prefix = io.get('prefix', 'io_')

    if groups == None:
        print("'groups' not found for " + block_name)
        exit()

    c_prev_pin_name = "OS_NULL"
    known_groups = {}

    for group in groups:
        process_group_block(group)

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
            process_io_block(io)

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
