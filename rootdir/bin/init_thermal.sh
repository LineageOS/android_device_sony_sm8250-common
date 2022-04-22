#!/vendor/bin/sh
SENSOR_LIST="sdm_modem_skin_sa"

MODEL=$(getprop ro.product.model)
I=0;
while true; do
	if [ ! -d /sys/class/thermal/thermal_zone${I} ]; then
		break;
	fi;
	TYPE=$(cat /sys/class/thermal/thermal_zone${I}/type)
	for J in ${SENSOR_LIST}; do
		if [ "${TYPE}" = "${J}" ]; then
			echo enabled > /sys/class/thermal/thermal_zone${I}/mode
			break;
		fi;
	done
	let I=${I}+1;
done
