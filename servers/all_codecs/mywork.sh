#!/bin/bash -
#===============================================================================
#
#          FILE: mywork.sh
#
#         USAGE: ./mywork.sh
#
#   DESCRIPTION:
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: YOUR NAME (),
#  ORGANIZATION:
#       CREATED: 2014年10月10日 09:26
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error
make clean
make
echo -e "\n\nend of make\n\n"
packagepath=/usr/local/icetek-dm6446-kbe/dvsdk_2_00_00_22/dm6446_dvsdk_combos_2_05/packages/ti/sdo/servers/all_codecs
x64Ppath=/opt/nfs/home/root/dm6446
cp bin/ti_platforms_evmDM6446/all.x64P ${x64Ppath}
rm -fr ${packagepath}/*
mkdir -p ${packagepath}/package/info
cp package/info/bin/ti_platforms_evmDM6446/all.x64P.info.js ${packagepath}/package/info
cp package.xdc ${packagepath}

