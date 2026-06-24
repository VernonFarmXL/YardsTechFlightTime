Import("env")
import os

usr = os.path.expanduser("~")
xtensa_path = os.path.join(usr, ".platformio", "tools", "toolchain-xtensa-esp-elf", "xtensa-esp-elf", "bin")
if os.path.isdir(xtensa_path):
    env.PrependENVPath("PATH", xtensa_path)
else:
    print("Warning: xtensa toolchain path not found:", xtensa_path)
