# RT config fragment for the LQR servo core (stacks on meta-panda's bbappend).
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append = " file://rt.cfg"
