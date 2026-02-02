..\..\utils\fw_ver.exe build.inc build.h ..\..\ ..\..\de0_top\PR_RF\de0_fpga.qsf
robocopy . share build.h /NFL /NDL /NJH /NJS /NS /NC /NP
robocopy cp_srv share cp_msg.h /NFL /NDL /NJH /NJS /NS /NC /NP
robocopy daq_srv share daq_msg.h /NFL /NDL /NJH /NJS /NS /NC /NP
robocopy ..\de0_bsp share system.h /NFL /NDL /NJH /NJS /NS /NC /NP
robocopy ..\de0_bsp share linker.h /NFL /NDL /NJH /NJS /NS /NC /NP
