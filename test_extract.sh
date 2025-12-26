#!/usr/bin/env bash

set -e

function extract() {
    format=$1
    file="$2"
    (
        set -x
        python -m pok3rtool.cli -v extract $format "$FWDIR/$file" output
    )
}

FWDIR="$1"
if [ -z "$FWDIR" ]; then
    echo "usage: $0 <directory with updater EXEs>"
    exit 1
fi

extract maajonsn vortex/POK3R_V113.exe
extract maajonsn vortex/POK3R_V114.exe
extract maajonsn vortex/POK3R_V115.exe
extract maajonsn vortex/POK3R_V116.exe
extract maajonsn vortex/POK3R_V117.exe

extract maav102 vortex/POK3R2_V110.exe

extract maav102 vortex/POK3R_RGB_V124.exe
extract maav102 vortex/POK3R_RGB_V130.exe
extract maav102 vortex/POK3R_RGB_V140.exe

extract maav102 vortex/POK3R_RGB2_V105.exe

extract maav102 vortex/CORE_V141.exe
extract maav102 vortex/CORE_V143.exe
extract maav102 vortex/CORE_V145.exe

extract maav102 vortex/CORE_RGB_V146.exe

extract maav102 vortex/TAB_60_V1113.exe

extract maav102 vortex/RACE_V121.exe
extract maav102 vortex/RACE_V124.exe
extract maav102 vortex/RACE_V125.exe

extract maav102 vortex/VIBE_V113.exe

extract maav102 vortex/CYPHER_V136.exe

extract maav102 coolermaster/masterkeys-pro-l-white-v1.08.exe
extract maav102 coolermaster/masterkeys-pro-m-white-v1.06.exe

extract maav102 tex/AP_0163_1.01.01r.exe

extract maav102 mistel/BOROCO_MD200.exe

extract maav102 mistel/BOROCO_MD600.exe

extract maav105 vortex/CORE_MPC.exe

extract maav105 vortex/TAB_75_V100.exe

extract maav105 vortex/TAB_90_V100.exe

extract kbp_cykb kbp/cykb112_v106.exe
extract kbp_cykb kbp/cykb112_v107.exe

extract kbp_cykb kbp/cykb129_v106.exe
