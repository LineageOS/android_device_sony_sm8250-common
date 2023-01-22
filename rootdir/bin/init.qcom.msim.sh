#!/vendor/bin/sh

model=`sed -e '/[Mm][Oo][Dd][Ee][Ll]: /!d' -e 's/^.*[Mm][Oo][Dd][Ee][Ll]: \([A-Za-z0-9-]*\).*$/\1/' -e '/^$/d' /dev/block/bootdevice/by-name/LTALabel` 2> /dev/null

case "$model" in
    "XQ-AT42" | "XQ-AT52" | "XQ-AT72" | "XQ-AS42" | "XQ-AS52" | "XQ-AS62" | "XQ-AS72" )
        setprop vendor.radio.multisim.config dsds;;
    * )
        setprop vendor.radio.multisim.config ss;;
esac
