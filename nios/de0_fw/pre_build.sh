../../utils/fw_ver.exe build.inc build.h ../../ ../../de0_top/PR_RF/de0_fpga.qsf
cp -f build.h share/build.h
cp -f cp_srv/cp_msg.h share/cp_msg.h
cp -f daq_srv/daq_msg.h share/daq_msg.h
cp -f ../de0_bsp/system.h share/system.h
cp -f ../de0_bsp/linker.h share/linker.h
