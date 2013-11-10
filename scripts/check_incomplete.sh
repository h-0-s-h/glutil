#!/bin/bash
#
#  Copyright (C) 2013 NixNodes
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# DO NOT EDIT/REMOVE THESE LINES
#@VERSION:1
#@REVISION:0
#@MACRO:incomplete:{m:exe} -d -execv "{m:spec1} {dir} {exe} {glroot} {m:arg1}" --silent
#@MACRO:incomplete-c:{m:exe} -d -execv "{m:spec1} {dir} {exe} {glroot} {m:arg2}" --iregex "{m:arg1}" --silent
#
## Checks a release for incomplete/corrupt files by comparing SFV data with filesystem
#
## Usage (manual): /glroot/bin/glutil -d -exec "/glftpd/bin/scripts/check_incomplete.sh '{dir}' '{exe}' '{glroot}'"
#
## Usage (macro): ./glutil -m incomplete                             (full dirlog)
##                ./glutil -m incomplete-c -arg1="Unforgettable"     (regex filtered dirlog scan)
#
##  To use this macro, place script in the same directory (or any subdirectory) where glutil is located
#
## See ./glutil --help for more info about options
#
###########################[ BEGIN OPTIONS ]#############################
#
## Verbose output
VERBOSE=0
#
## Optional corruption checking (CRC32 calc & match against .sfv)
CHECK_CORRUPT=0
#
############################[ END OPTIONS ]##############################

[ -z "$1" ] && exit 1
[ -z "$2" ] && exit 1

GLROOT=$3

c_dir() {
	while read l; do
		FFL=$(echo "$l" | sed -r 's/ [A-Fa-f0-9]*([ ]*|$)$//')
		FFT=$(dirname "$1")/$FFL
		! [ -f "$FFT" ] && echo "WARNING: $DIR: incomplete, missing file: $FFL" && continue
		[ $CHECK_CORRUPT -gt 0 ] && { 
			FCRC=$(echo "$l" | rev | cut -d " " -f -1 | rev) && CRC32=$($EXE --crc32 $FFT) && 
				[ $CRC32 != $FCRC ] && echo "WARNING: $DIR: corrupted: $FFL, CRC32: $CRC32, should be: $FCRC" && continue
		}
		[ $VERBOSE -gt 0 ] && echo "OK: $FFT: $FCRC"
	done < "$1"
}

[ "$4" = "cdir" ] && {
	DIR="$1"; EXE=$2
	[ -n "$5" ] && VERBOSE=1
	c_dir "$DIR"
	exit 1
}

DIR=$GLROOT$1
[ -n "$4" ] && VERBOSE=1

! [ -d "$DIR" ] && exit 1

$2 -x "$DIR" --iregexi "\.sfv$" -execv "$0 {path} $2 $3 cdir $4" --silent -recursive

exit 1


