# Build the panda driver from my server fork so there's only one register/mmap ABI
SRC_URI:remove = "git://github.com/PandABlocks/PandABlocks-server;branch=main;protocol=https"
SRC_URI:prepend = "git://github.com/DiamondRC/panda-bridge;branch=main;protocol=https "
SRCREV = "5b77c68b292523b0f0745749a65fa7e53523ebb7"
S = "${WORKDIR}/git/panda-server"
