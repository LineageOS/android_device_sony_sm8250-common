#!/vendor/bin/sh

model=`grep -aim1 'model:' /dev/block/by-name/LTALabel | sed -e 's/^.*model:[ ]*\([A-Za-z0-9-]*\).*$/\1/I'` 2> /dev/null

case "$model" in
    "XQ-AT42" | "XQ-AT52" | "XQ-AT72" | "XQ-AS42" | "XQ-AS52" | "XQ-AS62" | "XQ-AS72" )
        setprop vendor.radio.hardware.sku ds;;
    * )
        setprop vendor.radio.hardware.sku ss;;
esac

if [ "$model" == "" ]; then
    setprop vendor.radio.ltalabel.model "JP"
else
    setprop vendor.radio.ltalabel.model "$model"
fi
