export MYAPP=iosimulation
export MYCODEROOT=/coderoot
export MYBIN=${MYCODEROOT}/bin/linux
export MYSCRIPTS=${MYCODEROOT}/iocom/scripts
export MYCONFIG=${MYCODEROOT}/pins/extensions/${MYAPP}/config

export MYHW=simulation

${MYCODEROOT}/bin/linux/json --t2b -title ${MYCONFIG}/signals/${MYAPP}-signals.json ${MYCONFIG}/signals/${MYAPP}-signals.json-bin
python3 ${MYSCRIPTS}/signals-to-c.py ${MYCONFIG}/signals/${MYAPP}-signals.json -o ${MYCONFIG}/include/${MYHW}/${MYAPP}-signals.c
