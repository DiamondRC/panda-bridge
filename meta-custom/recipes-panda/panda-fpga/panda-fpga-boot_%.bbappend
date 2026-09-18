# Use my LQR image instead of upstream's prebuilt gitlab ipk.
# My boot.bin (FSBL + LQR bitstream + u-boot) and system.dtb carry RT
# bootargs (isolcpus=1 nohz_full=1 rcu_nocbs=1 irqaffinity=0) and the
# lqr-state@3fff0000 reserved-memory node.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:pandabox = " \
    file://boot/boot.bin \
    file://boot/system.dtb \
    file://boot/target-defs \
"
