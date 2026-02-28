"C:\Users\verno\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\3.0.0/esptool.exe" --chip esp32 --port COM3 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 YardsTechPanelReader.ino.bootloader.bin 0x8000 YardsTechPanelReader.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 2025.11.15-200.bin 
pause
