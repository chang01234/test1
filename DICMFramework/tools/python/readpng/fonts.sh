#!/bin/bash
function usage()
{
  echo "Usage:"
  echo " ${ZSH_SCRIPT} [-s <color>] [-o] -c <string> -f <file> -p #"
  echo " ${ZSH_SCRIPT} -h"
  echo "   -s <color>   stroke or fill color white|'#ddeeff'"
  echo "   -o           output to font_size directory"
  echo "   -c <string>  quoted list of characters to render for example:"
  echo "                \" \!\\\"#$%&'()*+,-./0123456789:;<=>?@AZ[\\\\\\]^_\\\`abcdefghijklmnopqrstuvwxyz{|}~\"\$(printf '\\\\ub0')"
  echo "   -f <file>    font file to use for rendering"
  echo "   -p #         point size"
  echo "   -h           this help"
  exit 2
}

fill=white
odir=false
dir=.

while getopts ':hoc:f:p:s:' opt
do
	case ${opt} in
		c) fstr=${OPTARG}  ;;  # string of chars to render
		f) font=${OPTARG}  ;;  # font to use
		p) psize=${OPTARG} ;;  # point size
		s) fill=${OPTARG}  ;;  # fill or stroke color
		o) odir=true ;;
		h)
			usage
			exit 0
			;;
		*)
			usage
			exit 2
			;;
	esac
done

[[ -z "${fstr}"  ]] && usage
[[ -z "${font}"  ]] && usage
[[ -z "${psize}" ]] && usage
[[ ! -e "${font}"  ]] && echo "$0: unable to read font: failed to open '${font}'" >&2 && exit 1

if ${odir}; then
	ffile=${font##*/}
	if [[ ${fill} == "white" ]]; then
		dir=${ffile%.*}_${psize}
	else
		dir=${ffile%.*}${fill}_${psize}
	fi
	mkdir -p $dir
fi

for (( i=0; i<${#fstr}; i++ )); do # for each i from 0 to length of fstr
	l=${fstr:$i:1}            #   l = fstr[i]
	aval=$(printf %d \"${l}\") #   aval = ascii val of l char
	echo "${l} to ${dir}/${aval}.png"
	case ${aval} in
		32|92) # escape ' ' and '\' special chars
			magick -background black -fill $fill -font $font -pointsize $psize label:"\\$l" -crop 0x${psize}+0+0 -fill "#FF00FF" -opaque "#000000" PNG8:${dir}/${aval}.png || exit $?
			;;
		*)
			magick -background black -fill $fill -font $font -pointsize $psize label:$l -crop 0x${psize}+0+0 -fill "#FF00FF" -opaque "#000000" PNG8:${dir}/${aval}.png || exit $?
			;;
	esac
done
