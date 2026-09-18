SUMMARY = "LQR non-linear controller FPGA app (bitstream + config) for PandABox"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"


FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI = " \
    file://config_d \
    file://ipmi.ini \
    file://extensions \
    file://panda_top.bin \
"

# Just packaged files - no compile, and the .bin is not an ELF to strip/split.
do_configure[noexec] = "1"
do_compile[noexec] = "1"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_SYSROOT_STRIP = "1"

do_install() {
    install -d ${D}/opt/share/${PN}
    cp -rf ${WORKDIR}/config_d ${D}/opt/share/${PN}/config_d
    cp -rf ${WORKDIR}/extensions ${D}/opt/share/${PN}/extensions
    install -m 0644 ${WORKDIR}/ipmi.ini ${D}/opt/share/${PN}/ipmi.ini
    install -m 0644 ${WORKDIR}/panda_top.bin ${D}/opt/share/${PN}/panda_top.bin
}

FILES:${PN} += "/opt"
# The bitstream is machine code for the PL, not the host CPU - skip QA arch check.
INSANE_SKIP:${PN} += "arch"
