#!/vendor/bin/sh

model=`grep -aim1 'model:' /dev/block/by-name/LTALabel | sed -e 's/^.*model:[ ]*\([A-Za-z0-9-]*\).*$/\1/I'` 2> /dev/null

if [ "$model" = "" ]; then
    model=`grep -aEm1 '(A002SO|SOG01|SOG02|SO-51A|SO-52A))&nbsp;' /dev/block/by-name/LTALabel 2> /dev/null | sed -nE 's/.*((A002SO|SOG01|SOG02|SO-51A|SO-52A))&nbsp;.*/\1/p'`
fi

case "$model" in
    "XQ-AT42" | "XQ-AT52" | "XQ-AT72" | "XQ-AS42" | "XQ-AS52" | "XQ-AS62" | "XQ-AS72" )
        setprop vendor.radio.hardware.sku ds;;
    * )
        setprop vendor.radio.hardware.sku ss;;
esac

if [ "$model" == "" ]; then
    setprop vendor.radio.ltalabel.model "unknown"
else
    setprop vendor.radio.ltalabel.model "$model"
fi
