export MYAPP=gina
export MYCODEROOT=/coderoot
export MYSCRIPTS=${MYCODEROOT}/pins/scripts
export MYCONFIG=${MYCODEROOT}/iocom/examples/${MYAPP}/config

export MYHW=carol

python3 ${MYSCRIPTS}/pins-to-c.py ${MYCONFIG}/pins/${MYHW}/${MYAPP}-io.json -o ${MYCONFIG}/include/${MYHW}/${MYAPP}-io.c
