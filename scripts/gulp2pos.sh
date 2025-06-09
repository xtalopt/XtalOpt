#!/bin/bash

# ========================================================================= #
#  Script to produce VASP format first/last structures from GULP output.    #
#  Samad Hajinazar          samadh~at~buffalo.edu                10/30/2023 #
# ========================================================================= #

# ===========================================================================
# Global variables and input analysis
# ===========================================================================

# ========== Global variables
inp="xtal.got"

# ========== Auxiliary functions
function write_options {
echo "======================================================================"
echo "Script to produce VASP format first/last structures from GULP output."
echo "Samad Hajinazar                                        10/30/2023"
echo "======================================================================"
echo "Valid input options are (format: '--option' 'value'):";echo
echo '  Flags:'
echo '    --input|-i      : GULP format input file (*default=xtal.got)'
echo '    --help|-h       : print this help!'
echo
echo '  Note:'
echo '  First structure is outputted as POSCAR and the last one as CONTCAR'
echo
echo '  Example:'
echo '    gulp2pos -i gulp.out'
echo
echo "======================================================================"
echo
}


# ===========================================================================
# Start and process the input
# ===========================================================================

# ========== Analyse the command line arguments
POSITIONAL_ARGS=(); c=0; unk=""
while [[ $# -gt 0 ]]; do
  case $1 in
    --input|-i)    inp="$2"; shift; shift ;;
    --help|-h)     write_options; exit ;;
    *)             POSITIONAL_ARGS+=("$1"); ((c=$c+1)); unk+=$1" "; shift ;;
  esac
done

# ========== Check for incorrect input
if [ $c -gt 0 ];then  write_options; echo " Error: input argument(s) not recognized:" $unk; exit; fi


# ===========================================================================
# Sanity checks before starting the job
# ===========================================================================

# ========== Check if required input files exist
if [ ! -e $inp ];then write_options; echo "Error: input file '$inp' does not exist!"; exit; fi

# ========== Basic check of the input file
if ! grep -q "Input for Configuration =   1" $inp || ! grep -q "General input information" $inp; then
  echo "Error: input file '$inp' does not seem a legit GULP output file!"; exit
fi


# ===========================================================================
# Extract general info from the input file.
# ===========================================================================

natm=$(awk '/Input for Configuration =   1/,/General input inform/' $inp| grep "Total number atoms" | awk '{print $5}')

typ0=$(awk '/Input for Configuration =   1/,/General input inform/' $inp| sed 's|*||g' | grep -A 2 "Label"|tail -n 1|awk '{print $2}')
spct=""
spcn=""
cnt0=0
for((i=1;i<=$natm;i++)); do
  ((ntmp=$i+1))
  coor=($(awk '/Input for Configuration =   1/,/General input inform/' $inp| sed 's|*||g' | grep -A $ntmp "Label" | tail -n 1))
  if [ $typ0 == "${coor[1]}" ]; then
    ((cnt0=$cnt0+1))
  else
    spcn+=" $cnt0"
    spct+=" $typ0"
    cnt0=1
    typ0=${coor[1]}
  fi
done

spcn+=" $cnt0"
spct+=" $typ0"


# ===========================================================================
# Extract structural info of the first structure in the input file.
# ===========================================================================

ilat=$(awk '/Input for Configuration =   1/,/General input inform/' $inp| grep -A 4 "lattice vec" | tail -n 3)
iatm=""
for((i=1;i<=$natm;i++)); do
  ((ntmp=$i+1))
  coor=($(awk '/Input for Configuration =   1/,/General input inform/' $inp| sed 's|*||g' | grep -A $ntmp "Label" | tail -n 1))
  iatm+=" ${coor[3]} ${coor[4]} ${coor[5]}"
done
ilat=($ilat)
iatm=($iatm)


# ===========================================================================
# Extract structural info of the final (if any) structure in the input file.
# ===========================================================================

iffi=0
if grep -q "Final fractional coordinates of atoms" $inp && grep -q "Final Cartesian lattice vectors" $inp; then
  iffi=1
  flat=$(grep -A 4 "Final Cartesian lattice vectors" $inp | tail -n 3)
  fatm=""
  for((i=1;i<=$natm;i++)); do
    ((ntmp=$i+5))
    coor=($(grep -A $ntmp "Final fractional coordinates of atoms" $inp | sed 's|*||g' | grep -A $ntmp "Label" | tail -n 1))
    fatm+=" ${coor[3]} ${coor[4]} ${coor[5]}"
  done
  flat=($flat)
  fatm=($fatm)
fi


# ===========================================================================
# Write the VASP format structures.
# ===========================================================================

sout="POSCAR"
printf "Structure from Gulp: initial\n"   > $sout
printf " 1.000\n"                        >> $sout
c=0
for((i=0;i<3;i++)); do
  for((j=0;j<3;j++)); do
    printf " %12.6lf" ${ilat[$c]}        >> $sout
    ((c=$c+1))
  done
  printf "\n"                            >> $sout
done
echo ${spct[@]}                          >> $sout
echo ${spcn[@]}                          >> $sout
echo "Direct"                            >> $sout
c=0
for((i=0;i<natm;i++)); do
  for((j=0;j<3;j++)); do
    printf " %12.6lf" ${iatm[$c]}        >> $sout
    ((c=$c+1))
  done
  printf "\n"                            >> $sout
done

# If no final structure; we're done here; just quit!
if [ $iffi -eq 0 ]; then exit; fi

sout="CONTCAR"
printf "Structure from Gulp: final\n"     > $sout
printf " 1.000\n"                        >> $sout
c=0
for((i=0;i<3;i++)); do
  for((j=0;j<3;j++)); do
    printf " %12.6lf" ${flat[$c]}        >> $sout
    ((c=$c+1))
  done
  printf "\n"                            >> $sout
done
echo ${spct[@]}                          >> $sout
echo ${spcn[@]}                          >> $sout
echo "Direct"                            >> $sout
c=0
for((i=0;i<natm;i++)); do
  for((j=0;j<3;j++)); do
    printf " %12.6lf" ${fatm[$c]}        >> $sout
    ((c=$c+1))
  done
  printf "\n"                            >> $sout
done
