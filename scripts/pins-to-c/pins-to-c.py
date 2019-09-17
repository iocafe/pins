# pins-to-c.py 16.9.2019/pekka
# Converts hardware IO specification(s) written in JSON to C code source and
# header files. 
# WARNING: Python 3.6 or newer is needed to preserve dictionary order.
# If run with older versions, IO pins in C are not in same order as in this C file,
# which can cause problems down the road since linked list order is may be used to
# give pins addresses is iocom memory block.
import json

pin_types = {
    "inputs" : "PIN_INPUT",
    "outputs" : "PIN_OUTPUT",
    "analog_inputs" : "PIN_ANALOG_INPUT",
    "analog_outputs" : "PIN_ANALOG_OUTPUT",
    "pwm" : "PIN_PWM", 
    "timers" : "PIN_TIMER"}

prm_type_list = {"speed": "PIN_SPEED", "delay": "PIN_DELAY"}

def start_c_files():
    global cfile, hfile
    cfile = open("/coderoot/pins/examples/jane/pins/carol/jane.c", "w")
    hfile = open("/coderoot/pins/examples/jane/pins/carol/jane.h", "w")

def finish_c_files():
    global cfile, hfile
    cfile.close()
    hfile.close()

def write_pin_to_c_header(pin_name):
    global block_name
    hfile.write("extern const Pin " + block_name + "_" + pin_name + ";\n")

def write_pin_to_c_source(pin_type, pin_name, pin_attr):
    global block_name, c_prev_pin_name, known_groups

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
        c_prm_array_name = block_name + "_prm_" + pin_name
        cfile.write("static os_short " + c_prm_array_name + "[]")
        cfile.write("= {" + c_prm_list + "};\n")

    # Write pin name and type
    full_pin_name = block_name + "_" + pin_name
    cfile.write("const Pin " + full_pin_name + ' = {"')
    cfile.write(pin_name + '", ' + pin_types[pin_type] + ", ")

    # Write pin address, merge addr and bank
    bank = pin_attr.get("bank", None)
    addr = pin_attr.get("addr", None)
    if addr is None:
        cfile.write("PIN_NO_ADDRESS")
    else:
        if bank is None:
            cfile.write(str(addr) + "<<PIN_ASHIFT")
        else:
            cfile.write(str(bank) + "|(" + str(addr) + "<<PIN_ASHIFT)")

    # Write pointer to parameter array, if any
    cfile.write(", " + c_prm_array_name + ", ")
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
            
        cfile.write(g)

    # Setup linked list for all pins in this definition block
    cfile.write(", " + c_prev_pin_name)
    c_prev_pin_name = full_pin_name
    cfile.write("};\n")

def write_linked_list_heads():
    global block_name, c_prev_pin_name, known_groups

    for g, value in known_groups.items():
        varname = block_name + "_" + g + "_group";
        cfile.write("const Pin *" + varname + " = " + value + ";\n")
        hfile.write("extern const Pin *" + varname + ";\n")

    if c_prev_pin_name is not "OS_NULL":
        varname = block_name + "_pins";
        cfile.write("const Pin *" + varname + " = " + c_prev_pin_name + ";\n")
        hfile.write("extern const Pin *" + varname + ";\n")


# const Pin alice_LIGHT_SWITCH = {"LIGHT_SWITCH", PIN_BIN, 4|(2 << 8), alice_prm_LIGHT_SWITCH, sizeof(alice_prm_LIGHT_SWITCH)/sizeof(os_short));    

def process_pin(pin_type, pin_name, pin_attr):
    global block_name
    write_pin_to_c_header(pin_name)
    write_pin_to_c_source(pin_type, pin_name, pin_attr)

def process_pin_type(pin_type, pins):
    for pin_name, pin_attr in pins.items():
        process_pin(pin_type, pin_name, pin_attr)

def process_pin_block(pin_block):
    global block_name, pin_types, c_prev_pin_name, known_groups
    block_name = pin_block.get("name", "iopin");
    c_prev_pin_name = "OS_NULL"
    known_groups = {}
    for pin_type in pin_types:
        pins = pin_block.get(pin_type, None)
        if pins is not None:
            process_pin_type(pin_type, pins)
            cfile.write("\n")
            hfile.write("\n")

    write_linked_list_heads()

def process_source_file(path):
    with open("/coderoot/pins/examples/jane/pins/carol/jane.json", "r") as read_file:
        data = json.load(read_file)

        for pin_block_tag, pin_block in data.items():
            if pin_block_tag == "pins":
                process_pin_block(pin_block)
            
def mymain():
    start_c_files()

    process_source_file("/coderoot/pins/examples/jane/pins/carol/jane.json")

    finish_c_files()

mymain()