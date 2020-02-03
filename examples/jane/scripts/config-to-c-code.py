import os
import platform

MYAPP = 'jane'
MYHW = 'carol'
if platform.system() == 'Windows':
    MYPYTHON = 'python'
    MYCODEROOT = 'c:/coderoot'
else:
    MYPYTHON = 'python3'
    MYCODEROOT = '/coderoot'
PINSTOC = MYPYTHON + ' ' + MYCODEROOT + '/pins/scripts/pins-to-c.py'

MYCONFIG = MYCODEROOT + '/pins/examples/' + MYAPP + '/config'
MYINCLUDE = MYCONFIG + '/include/' + MYHW
MYPINS = MYCONFIG + '/pins/' + MYHW + '/pins-io'

def runcmd(cmd):
    stream = os.popen(cmd)
    output = stream.read()
    print(output)

runcmd(PINSTOC + ' ' + MYPINS + '.json -o ' + MYINCLUDE + '/' + MYAPP + '-io.c')

print("*** Check that the output files have been generated (error checks are still missing).")
print("*** You may need to recompile all C code since generated files in config/include folder are not in compiler dependencies.")
