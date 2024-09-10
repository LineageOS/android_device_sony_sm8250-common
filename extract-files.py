#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixup_remove,
    lib_fixup_remove_arch_suffix,
    lib_fixup_vendorcompat,
    lib_fixups_user_type,
    libs_clang_rt_ubsan,
    libs_proto_3_9_1,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'hardware/qcom-caf/sm8250',
    'hardware/qcom-caf/wlan',
    'hardware/sony',
    'vendor/qcom/opensource/commonsys/display',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/qcom/opensource/dataservices',
    'vendor/qcom/opensource/display',
]


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None

def lib_fixup_remove_v32(lib: str, *args, **kwargs):
    return lib.replace('-v32', '') if '-v32' in lib else None


lib_fixups: lib_fixups_user_type = {
    libs_clang_rt_ubsan: lib_fixup_remove_arch_suffix,
    libs_proto_3_9_1: lib_fixup_vendorcompat,
    (
        'com.qualcomm.qti.dpm.api@1.0',
        'com.qualcomm.qti.imscmservice@1.0',
        'com.qualcomm.qti.imscmservice@2.0',
        'com.qualcomm.qti.imscmservice@2.1',
        'com.qualcomm.qti.imscmservice@2.2',
        'com.qualcomm.qti.uceservice@2.0',
        'com.qualcomm.qti.uceservice@2.1',
        'libmmosal',
        'vendor.qti.hardware.data.cne.internal.api@1.0',
        'vendor.qti.hardware.data.cne.internal.constants@1.0',
        'vendor.qti.hardware.data.cne.internal.server@1.0',
        'vendor.qti.hardware.data.connection@1.0',
        'vendor.qti.hardware.data.connection@1.1',
        'vendor.qti.hardware.data.dynamicdds@1.0',
        'vendor.qti.hardware.data.iwlan@1.0',
        'vendor.qti.hardware.data.qmi@1.0',
        'vendor.qti.hardware.fm@1.0',
        'vendor.qti.hardware.qseecom@1.0',
        'vendor.qti.hardware.tui_comm@1.0',
        'vendor.qti.hardware.wifidisplaysession@1.0',
        'vendor.qti.ims.callinfo@1.0',
        'vendor.qti.ims.rcsconfig@1.0',
        'vendor.qti.ims.rcsconfig@1.1',
        'vendor.qti.imsrtpservice@3.0',
        'vendor.somc.hardware.miscta@1.0',
    ): lib_fixup_vendor_suffix,
    (
        'libOmxCore',
        'libplatformconfig',
        'libwpa_client',
        'libwfdaac_vendor',
        'libc2dcolorconvert',
        'libril',
    ): lib_fixup_remove,
    (
        'libhidlbase-v32',
        'libbinder-v32',
        'libutils-v32',
    ): lib_fixup_remove_v32,
}

blob_fixups: blob_fixups_user_type = {
    (
        'system_ext/lib64/libwfdnative.so',
        'vendor/lib64/libvpplibrary.so',
        'vendor/lib64/libswiqisettinghelper.so',
        'vendor/lib64/vendor.somc.hardware.swiqi@1.0-impl.so',
    ): blob_fixup()
        #.binary_regex_replace(b'android.hidl.base@1.0.so', b'libhidlbase.so\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'),
        .replace_needed('android.hidl.base@1.0.so', 'libhidlbase.so'),
    (
        'product/lib64/libdpmframework.so',
    ): blob_fixup()
        #.binary_regex_replace(b'libhidltransport.so', b'libcutils-v29.so\x00\x00\x00'),
        .replace_needed('libhidltransport.so', 'libcutils-v29.so'),
    (
        'vendor/lib64/vendor.semc.hardware.extlight-V1-ndk_platform.so',
    ): blob_fixup()
        .replace_needed('android.hardware.light-V1-ndk_platform.so', 'android.hardware.light-V1-ndk.so'),
    (
        'vendor/lib64/vendor.somc.camera.device@3.2-impl.so',
        'vendor/lib64/vendor.somc.camera.device@3.3-impl.so',
        'vendor/lib64/vendor.somc.camera.device@3.4-impl.so',
        'vendor/lib64/vendor.somc.camera.device@3.5-impl.so',
        'vendor/bin/hw/vendor.somc.hardware.camera.provider@1.0-service',
    ): blob_fixup()
        .replace_needed('libutils.so', 'libutils-v32.so')
        .replace_needed('libhidlbase.so', 'libhidlbase-v32.so')
        .replace_needed('libbinder.so', 'libbinder-v32.so')
        .add_needed('libbinder-v32.so'),
    (
        'vendor/lib/libiVptApi.so',
        'vendor/lib64/libiVptApi.so',
    ): blob_fixup()
        .add_needed('libiVptLibC.so'),
    (
        'vendor/lib/libiVptLibC.so',
        'vendor/lib/libHpEqApi.so',
        'vendor/lib64/libiVptLibC.so',
        'vendor/lib64/libHpEqApi.so',
    ): blob_fixup()
        .add_needed('libcrypto.so')
        .add_needed('libiVptHkiDec.so'),
    (
        'vendor/lib/mediadrm/libwvdrmengine.so',
        'vendor/lib/libwvhidl.so',
    ): blob_fixup()
        .add_needed('libcrypto_shim.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'sm8250-common',
    'sony',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
    check_elf=True,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
