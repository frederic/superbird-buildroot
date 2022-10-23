
step1:add the macros for trumpet support in buildroot/configs/xxx_defconfig
#alsa-plugin for dolby daws
BR2_PACKAGE_ALSA_PLUGINS=y
BR2_PACKAGE_LIBEVDEV=y
BR2_PACKAGE_DOLBY_DAWS_PLUGIN=y
BR2_PACKAGE_DOLBY_DAWS_SERVICE=y
BR2_PACKAGE_DOLBY_DAWS_SECURITY=y

step2:add trumpet config in asound.conf, path: buildroot/board/amlogic/xxx/rootfs/etc/asound.conf
pcm.daws {
     type daws
     channels 4
     slave.pcm "hw:0,2"
     dump_enable 0
     dumpin_name "/mnt/plugin.raw"
     dumpout_name "/mnt/plugout.raw"
}
#explain:
#   type:           plugin name
#   channels:       output channels
#   dump_enable:    dump data enable
#   dumpin_name:    dump plug input path
#   dumpout_name:   dump plug output path

step3: compile package or stepwise compilation
    make alsa-plugins-rebuild
    make libevdev-rebuild
    make dolby_daws_plugin-rebuild
    make dolby_daws_service-rebuild
    make dolby_daws_security-rebuild
    make
