#!/bin/bash

# ========================================================================= #
#  Script to produce genealogy information for XtalOpt run structure.       #
#  Samad Hajinazar          samadh~at~buffalo.edu                10/30/2023 #
# ========================================================================= #

# Output the genealogy of a structure from the XtalOpt run output,
#   i.e., parent information for the input structure, and offspring
#   created from the input structure.
#
# A structure tag, as [generation]x[id] (e.g., 2x1), must be given.
#
# The local working directory will be read in from xtalopt.in if
#   present, or can be provided as the second input argument.
#   Otherwise, by default, it's assumed to be "./local/"

#======================================================================
#======================================================================

dir="local"

if [ $# -lt 1 ]; then echo Error: no input structure!; exit; fi
str="$1"

dir=""
if [ $# -eq 2 ]; then
  dir="$2"
elif [ -e xtalopt.in ]; then
  dir=$(grep -i "localworkingdirectory" xtalopt.in | cut -d "=" -f2)
fi

if [ "$dir" == "" ] || [ ! -e $dir ]; then echo Error: no working directory "'$dir'"!; exit; fi

gen=$(echo $str|sed 's/x/   /'|awk '{printf "%05d",$1}')
id=$(echo $str|sed 's/x/   /'|awk '{printf "%05d",$2}')
fll=$gen"x"$id

if [ ! -e $dir/$fll/structure.state ]; then echo Error: no "'$dir/$fll/structure.state'" file!; exit; fi

printf "strc   % 12s   % 5s   % 5s   %s\n" "directory" "indx" "rank" "genealogy"
echo   "----------------------------------------------------------------------------"

#==== "Parent(s)"
#export GREP_COLOR='01;36'
rnk=$(grep rank= $dir/$fll/structure.state|cut -d '=' -f2)
ind=$(grep index= $dir/$fll/structure.state|cut -d '=' -f2)
prn=$(grep parents= $dir/$fll/structure.state| sed 's|parents=||' | sed 's|\"||g')
printf "this    % 12s   % 5d   % 5d   " $(echo $fll|grep --color=always -E "$fll") $ind $rnk
echo "$prn"

#==== "Children"
echo   "----------------------------------------------------------------------------"
avgrnk=0
avgind=0
minrnk="1000000"
maxrnk="-1000000"
counts=0
lst=($(grep parents= $dir/*/st*te| grep "$str" | awk -F "/structure.state" '{print $1}' | awk -F "/" '{print $NF}'))
#export GREP_COLOR='1;37;41'
for((i=0; i < ${#lst[@]}; i++)); do
  cld="${lst[$i]}"
  if [ -e $dir/$cld/structure.state ]; then
    rnk=$(grep rank= $dir/$cld/structure.state|cut -d '=' -f2)
    ind=$(grep index= $dir/$cld/structure.state|cut -d '=' -f2)
    prn=$(grep parents= $dir/$cld/structure.state| sed 's|parents=||' | sed 's|\"||g')
    all=($prn)
    for((j=0; j < ${#all[@]}; j++)); do
      if [ "${all[$j]}" == "$str" ]; then
        if [ $(echo "$rnk > ($maxrnk)"|bc) -eq 1 ]; then maxrnk="$rnk"; fi
        if [ $(echo "$rnk < ($minrnk)"|bc) -eq 1 ]; then minrnk="$rnk"; fi
        ((counts=$counts+1))
        ((avgind=$avgind+$ind))
        ((avgrnk=$avgrnk+$rnk))
        printf "chld   % 12s   % 5d   % 5d   " $cld $ind $rnk
        echo "$prn" | grep --color=always -E $str
        break
      fi
    done
  fi
done

#==== "Statistics"
echo   "----------------------------------------------------------------------------"
if [ $counts -gt 0 ]; then
  avgrnk=$(echo "scale=0; $avgrnk / $counts"|bc -l)
  avgind=$(echo "scale=0; $avgind / $counts"|bc -l)
  avgrng=$(echo "scale=0; (($minrnk)+($maxrnk))/2"|bc -l)
  rnkrng="$minrnk-$maxrnk:$avgrng"
else
  rnkrng="none"
fi
printf "avgs   % 12s   % 5d   % 5d   %s\n" "$counts" $avgind $avgrnk "$rnkrng"
