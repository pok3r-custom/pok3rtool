#!/usr/bin/env bash

FWDIR=../pok3r_re_firmware
CLI="python -m pok3rtool.cli"

$CLI -v extract maajonsn $FWDIR/vendor/vortex/POK3R_V113.exe output
$CLI -v extract maajonsn $FWDIR/vendor/vortex/POK3R_V114.exe output
$CLI -v extract maajonsn $FWDIR/vendor/vortex/POK3R_V115.exe output
$CLI -v extract maajonsn $FWDIR/vendor/vortex/POK3R_V116.exe output
$CLI -v extract maajonsn $FWDIR/vendor/vortex/POK3R_V117.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/POK3R2_V110.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/POK3R_RGB_V124.exe output
$CLI -v extract maav102 $FWDIR/vendor/vortex/POK3R_RGB_V130.exe output
$CLI -v extract maav102 $FWDIR/vendor/vortex/POK3R_RGB_V140.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/POK3R_RGB2_V105.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/CORE_V141.exe output
$CLI -v extract maav102 $FWDIR/vendor/vortex/CORE_V143.exe output
$CLI -v extract maav102 $FWDIR/vendor/vortex/CORE_V145.exe output
$CLI -v extract maav105 $FWDIR/vendor/vortex/CORE_MPC.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/CORE_RGB_V146.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/TAB_60_V1113.exe output

$CLI -v extract maav105 $FWDIR/vendor/vortex/TAB_75_V100.exe output

$CLI -v extract maav105 $FWDIR/vendor/vortex/TAB_90_V100.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/RACE_V121.exe output
$CLI -v extract maav102 $FWDIR/vendor/vortex/RACE_V124.exe output
$CLI -v extract maav102 $FWDIR/vendor/vortex/RACE_V125.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/VIBE_V113.exe output

$CLI -v extract maav102 $FWDIR/vendor/vortex/CYPHER_V136.exe output
