#!/bin/bash
# ========================================================================= #
#  Script to produce ancestry information for XtalOpt run structure.        #
#  Samad Hajinazar          samadh~at~buffalo.edu                10/30/2023 #
# ========================================================================= #

# Output the ancestry of a structure from the XtalOpt run output,
#   i.e., the sequence of parents and genetic operations that
#   have created a given structure.
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

#==== "Parent(s)"

all=($str)
while [ ${#all[@]} -gt 0 ]; do
  npr=""
  lns=0
  trc=""
  for((i=0; i < ${#all[@]}; i++)); do
    str=${all[$i]}
    if [[ $trc == *" $str "* ]];then
      continue;
    fi
    trc=$trc" $str "
    printf " % 12s " "[$str]"
    gen=$(echo $str|sed 's/x/   /'|awk '{printf "%05d",$1}')
    id=$(echo $str|sed 's/x/   /'|awk '{printf "%05d",$2}')
    fll=$gen"x"$id
    tmp=$(grep parents= $dir/$fll/structure.state| sed 's|parents=||' | sed 's|\"||g')
    # the operation is what comes before the first ":" or "(" of the string;
    #   a structure tag is not a part of the operation
    out=$(echo "$tmp" | sed 's|[:(].*||' | sed 's|[0-9][0-9]*x[0-9][0-9]*||g')
    # the operation is always written as a single word in the output
    out=$(echo "$out" | sed 's| *$||' | sed 's|  *|_|g')
    # shorten the supercell prefix, e.g., "Supercell[2]-Stripple" to "sc2Stripple"
    out=$(echo "$out" | sed 's|Supercell\[|sc|' | sed 's|\]-||')
    # the parents are the "[generation]x[id]" tags that appear in the string
    for prt in $(echo "$tmp" | grep -owE '[0-9]+x[0-9]+'); do
      out=$out"_"$prt
      npr=$npr" $prt"
    done
    printf " %-22s" "$out"
    ((lns=$lns+1))
    if [ $(($lns % 2)) -eq 0 ]; then printf "\n"; fi
  done
  if [ $(($lns % 2)) -ne 0 ]; then printf "\n"; fi
  echo "------------------------------------------------------------"
  all=($npr)
done

exit
