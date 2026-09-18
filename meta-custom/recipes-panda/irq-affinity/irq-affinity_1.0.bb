SUMMARY = "Steer device IRQs to CPU0, off the isolated LQR RT core (CPU1)"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI = " \
    file://irq-affinity.service \
    file://irq-affinity \
"

do_install() {
    install -d ${D}${systemd_system_unitdir} ${D}${bindir}
    install -m 0644 ${WORKDIR}/irq-affinity.service ${D}${systemd_system_unitdir}
    install -m 0755 ${WORKDIR}/irq-affinity ${D}${bindir}
}

inherit systemd
SYSTEMD_SERVICE:${PN} = "irq-affinity.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"
