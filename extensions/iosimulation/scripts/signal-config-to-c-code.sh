export MYAPP=gina
export MYCODEROOT=/coderoot
export MYBIN=${MYCODEROOT}/bin/linux
export MYSCRIPTS=${MYCODEROOT}/iocom/scripts
export MYCONFIG=${MYCODEROOT}/iocom/examples/${MYAPP}/config

export MYHW=carol

${MYCODEROOT}/bin/linux/json --t2b -title ${MYCONFIG}/signals/${MYAPP}-signals.json ${MYCONFIG}/signals/${MYAPP}-signals.json-bin
python3 ${MYSCRIPTS}/signals-to-c.py ${MYCONFIG}/signals/${MYAPP}-signals.json -p ${MYCONFIG}/pins/${MYHW}/${MYAPP}-io.json -o ${MYCONFIG}/include/${MYHW}/${MYAPP}-signals.c
