how to make:
user64bit && kernel64bit: make ARCH=arm64
user32bit && kernel32bit: make ARCH=arm32
user32bit && kernel64bit: make ARCH=arm32 matchbit=1