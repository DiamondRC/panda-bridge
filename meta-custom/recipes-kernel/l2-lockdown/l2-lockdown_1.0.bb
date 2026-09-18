SUMMARY = "PL310 L2 lockdown-by-master: confine the ACP to one L2 way for RT determinism"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit module

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI = " \
    file://l2_lockdown.c \
    file://Kbuild \
    file://Makefile \
"
S = "${WORKDIR}"
