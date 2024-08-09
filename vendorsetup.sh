if [ -n "${CLEAN_DT_REPOS}" ]; then
    if [ "$CLEAN_DT_REPOS" = "True" ]; then
        echo "Cleaning old repos before cloning"
        rm -rf vendor/oplus
        rm -rf kernel/oneplus
        rm -rf device/mediatek/sepolicy_vndr
        rm -rf device/oplus
        rm -rf hardware/oplus
        rm -rf hardware/lineage/compat
        rm -rf hardware/mediatek
        unset CLEAN_DT_REPOS
    fi
fi
echo start cloning repos
VT=vendor/oplus/ivan/ivan-vendor.mk
if ! [ -a $VT ]; then git clone https://gitlab.com/notganesh/vendor_oneplus_ivan.git -b lineage-20 vendor/oplus/ivan
fi
KT=kernel/oneplus/ivan/Makefile
if ! [ -a $KT ]; then git clone https://github.com/Notganesh/kernel_oneplus_ivan.git -b main kernel/oneplus/ivan
fi
MTK_SEPOLICY=device/mediatek/sepolicy_vndr/SEPolicy.mk
if ! [ -a $MTK_SEPOLICY ]; then git clone --depth=1 https://github.com/LineageOS/android_device_mediatek_sepolicy_vndr.git -b lineage-21 device/mediatek/sepolicy_vndr
fi
COMPACT=hardware/lineage/compat/Android.bp
if ! [ -a $COMPACT ]; then git clone --depth=1 https://github.com/LineageOS/android_hardware_lineage_compat -b lineage-21.0 hardware/lineage/compat
fi
MTK=hardware/mediatek/Android.bp
if ! [ -a $MTK ]; then git clone --depth=1 https://github.com/LineageOS/android_hardware_mediatek.git -b lineage-21 hardware/mediatek
fi
CLANG17=prebuilts/clang/host/linux-x86/clang-r383902/bin/clang
if ! [ -a $CLANG17 ]; then git clone --depth=1 https://github.com/oneplus-mediatek-dev/android_prebuilts_clang_host_linux-x86_clang-6443078.git -b 11.0.1 prebuilts/clang/host/linux-x86/clang-r383902
fi
WLAN=hardware/mediatek/wlan/Android.mk
if ! [ -a $WLAN ]; then git clone --depth=1 https://github.com/nishant6342/android_hardware_mediatek_wlan hardware/mediatek/wlan
fi
OPLUS=hardware/oplus/Android.mk
if ! [ -a $OPLUS ]; then git clone --depth=1 https://github.com/LineageOS/android_hardware_oplus.git -b lineage-21 hardware/oplus
fi
echo end cloning
