# Dedicated LQR image
RDEPENDS:${PN}:remove = " \
    panda-fpga-pandabox-no-fmc \
    panda-fpga-pandabox-fmc-24vio \
    panda-fpga-pandabox-fmc-acq427 \
    panda-fpga-pandabox-fmc-acq430 \
    panda-fpga-pandabox-fmc-lback-sfp-lback \
"
RDEPENDS:${PN}:append = " panda-fpga-pandabox-lqr"
