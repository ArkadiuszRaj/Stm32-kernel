
Before trying to build examples, please read the following page carefully:
https://dfrank.bitbucket.io/kernelkernel_api/latest/html/building.html

In short, you need to copy configuration file in the kernel directory to
build it.  Each example has `kernel_cfg_appl.h` file, and you should either create
a symbolic link to this file from `kernel/src/kernel_cfg.h` or just copy this
file as `kernel/src/kernel_cfg.h`.

