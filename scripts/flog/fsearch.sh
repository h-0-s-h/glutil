#!/bin/sh
#@VERSION:0
#@REVISION:2
#
GLUTIL="/bin/glutil-chroot"
FILE_LOG_PATH=/ftp-data/glutil/db/filelog
#
############################################
#

echo "${@}" | egrep -q "([\`\>\<\|]|\$\()" && exit 2

vstr=`echo "${@}" | sed -r 's/ /.*/'`

[ -z "${vstr}" ] && {
	vstr=".*"
}

${GLUTIL} -q altlog --nobuffer --altlog ${FILE_LOG_PATH} --silent \
	iregexi basedir,"${vstr}" -print "{file}  [{?m:size/1024} k] [{?tl:time#%m/%d/%y}][{?tl:time#%H}:{?tl:time#%M}:{?tl:time#%S}]"

exit 0

