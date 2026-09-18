# Build the LQR-bridge fork of the server from github.com/DiamondRC/panda-bridge.
# TODO - hardcoded to my personal repo!
#
# meta-panda's file:// entries (CONFIG_server, services, wrappers) and do_install/systemd
SRC_URI:remove = "git://github.com/PandABlocks/PandABlocks-server;branch=main;protocol=https"
SRC_URI:prepend = "git://github.com/DiamondRC/panda-bridge;branch=main;protocol=https "
SRCREV = "5b77c68b292523b0f0745749a65fa7e53523ebb7"
S = "${WORKDIR}/git/panda-server"

# Drop docs
# TODO - add proper Myst docs
do_configure:append() {
    sed -i 's/^DEFAULT_TARGETS = .*/DEFAULT_TARGETS = server slow_load/' ${S}/CONFIG
}
do_compile:append() {
    mkdir -p ${S}/build/html
}
