#!/bin/bash
#
# SPDX-FileCopyrightText: 2016 The CyanogenMod Project
# SPDX-FileCopyrightText: 2017-2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

export TARGET_ENABLE_CHECKELF=true

# If XML files don't have comments before the XML header, use this flag
# Can still be used with broken XML files by using blob_fixup
export TARGET_DISABLE_XML_FIXING=true

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

ONLY_COMMON=
ONLY_FIRMWARE=
ONLY_TARGET=
KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-common)
            ONLY_COMMON=true
            ;;
        --only-firmware)
            ONLY_FIRMWARE=true
            ;;
        --only-target)
            ONLY_TARGET=true
            ;;
        -n | --no-cleanup)
            CLEAN_VENDOR=false
            ;;
        -k | --kang)
            KANG="--kang"
            ;;
        -s | --section)
            SECTION="${2}"
            shift
            CLEAN_VENDOR=false
            ;;
        *)
            SRC="${1}"
            ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

function blob_fixup() {
    case "${1}" in
    system_ext/lib64/libwfdnative.so | vendor/lib64/libvpplibrary.so | vendor/lib64/libswiqisettinghelper.so | /vendor/lib64/vendor.somc.hardware.swiqi@1.0-impl.so)
        [ "$2" = "" ] && return 0
        grep -q "android.hidl.base@1.0.so" "${2}" && sed -i "s/android.hidl.base@1.0.so/libhidlbase.so\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/g" "${2}"
        ;;
    product/lib64/libdpmframework.so)
        [ "$2" = "" ] && return 0
        grep -q "libhidltransport.so" "${2}" && sed -i "s/libhidltransport.so/libcutils-v29.so\x00\x00\x00/g" "${2}"
        ;;
    vendor/lib64/vendor.semc.hardware.extlight-V1-ndk_platform.so)
        [ "$2" = "" ] && return 0
        grep -q "android.hardware.light-V1-ndk.so" "${2}" || "${PATCHELF}" --replace-needed "android.hardware.light-V1-ndk_platform.so" "android.hardware.light-V1-ndk.so" "${2}"
        ;;
    vendor/lib64/vendor.somc.camera* | vendor/bin/hw/vendor.somc.hardware.camera.*)
        [ "$2" = "" ] && return 0
        grep -q "libutils-v32.so" "${2}" || "${PATCHELF}" --replace-needed "libutils.so" "libutils-v32.so" "${2}"
        grep -q "libhidlbase-v32.so" "${2}" || "${PATCHELF}" --replace-needed "libhidlbase.so" "libhidlbase-v32.so" "${2}"
        grep -q "libbinder-v32.so" "${2}" && return 0
        if ! "${PATCHELF}" --print-needed "${2}" | grep "libbinder.so" > /dev/null; then
            "${PATCHELF}" --add-needed "libbinder-v32.so" "${2}"
        else
            "${PATCHELF}" --replace-needed "libbinder.so" "libbinder-v32.so" "${2}"
        fi
        ;;
    vendor/lib/libiVptApi.so | vendor/lib64/libiVptApi.so)
        [ "$2" = "" ] && return 0
        grep -q "libiVptLibC.so" "${2}" || "${PATCHELF}" --add-needed "libiVptLibC.so" "${2}"
        ;;
    vendor/lib/libiVptLibC.so | vendor/lib64/libiVptLibC.so | vendor/lib/libHpEqApi.so | vendor/lib64/libHpEqApi.so)
        [ "$2" = "" ] && return 0
        grep -q "libcrypto.so" "${2}" || "${PATCHELF}" --add-needed "libcrypto.so" "${2}"
        grep -q "libiVptHkiDec.so" "${2}" || "${PATCHELF}" --add-needed "libiVptHkiDec.so" "${2}"
        ;;
    esac

    return 0
}

function blob_fixup_dry() {
    blob_fixup "$1" ""
}

if [ -z "${ONLY_FIRMWARE}" ] && [ -z "${ONLY_TARGET}" ]; then
    # Initialize the helper for common device
    setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" true "${CLEAN_VENDOR}"

    extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

if [ -z "${ONLY_COMMON}" ] && [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    source "${MY_DIR}/../${DEVICE}/extract-files.sh"
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    if [ -z "${ONLY_FIRMWARE}" ]; then
        extract "${MY_DIR}/../../${VENDOR}/${DEVICE}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
    fi

    if [ -z "${SECTION}" ] && [ -f "${MY_DIR}/../../${VENDOR}/${DEVICE}/proprietary-firmware.txt" ]; then
        extract_firmware "${MY_DIR}/../../${VENDOR}/${DEVICE}/proprietary-firmware.txt" "${SRC}"
    fi
fi

"${MY_DIR}/setup-makefiles.sh"
