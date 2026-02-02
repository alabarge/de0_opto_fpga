..\..\utils\ci_check.exe de0_fw_ci.txt zipfs/de0_fw_ci.txt zipfs/de0_fw_ci.bin
rm -f zipfs.zip
cd zipfs
"C:\Program Files\7-Zip\7z.exe" a -tzip -mx0 ..\zipfs.zip
cd ..
