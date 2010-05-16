# Check if executable is compiled
if [ ! -e $XTALOPT_SO ]
then
    FAIL="$FAIL\nxtalopt.so has not been generated, or the wrong path is specified."
fi

SYMBOLS="`(ldd -d -r $XTALOPT_SO) 2>&1 |grep undefined |awk '{print $3}'|xargs`"

if [ ! -z "$SYMBOLS" ]
then 
    FAIL="$FAIL"'\nThere are undefined symbols'
fi

for S in $SYMBOLS
do 
    echo "WARNING: UNDEFINED SYMBOL: `nm $XTALOPT_SO | grep $S | c++filt`"
done
