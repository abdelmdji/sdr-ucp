# contrib/sdr-ucp/CMakeLists.txt
# Modulation-aware PHY layer for Aqua-Sim NG.
# Kept as a SEPARATE contrib module so that contrib/aqua-sim-ng stays a
# pristine checkout of upstream commit 0fbc166 -- the reproducibility claim
# in the paper is then "stock Aqua-Sim NG @ 0fbc166 plus this module",
# which anyone can verify with a diff.
build_lib(
    LIBNAME sdr-ucp
    SOURCE_FILES
        model/sdr-ucp-modulation.cc
        model/sdr-ucp-mod-tag.cc
        model/sdr-ucp-phy.cc
    HEADER_FILES
        model/sdr-ucp-ber.h
        model/sdr-ucp-modulation.h
        model/sdr-ucp-mod-tag.h
        model/sdr-ucp-phy.h
    LIBRARIES_TO_LINK
        ${libaqua-sim-ng}
        ${libnetwork}
        ${libenergy}
        ${libmobility}
        ${libinternet}
)
