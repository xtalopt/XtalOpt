#!/bin/bash

# ========================================================================= #
#  Script to convert MTP CFG format to VASP POSCAR/OUTCAR file.             #
#  Samad Hajinazar          samadh~at~buffalo.edu                11/05/2023 #
# ========================================================================= #

# ===========================================================================
# Global variables
# ===========================================================================

# ========== Global variables
inp="input.cfg"  # CFG format input structure file
str="CONTCAR"    # VASP format output structure file
efs="OUTCAR"     # VASP format output e-f-s file 
spl=""           # list of atomic type symbols

# ========== Pressure conversions
gpa2kb="10.0"
gpa2ev="0.006241509125883"
ev2kb="1602.1766208"

# ========== Auxiliary functions
function write_options {
echo "======================================================================"
echo "Script to convert MTP CFG format to VASP POSCAR/OUTCAR file."
echo "Samad Hajinazar                                        11/05/2023"
echo "======================================================================"
echo "Valid input options are (format: '--option' 'value'):";echo
echo '  Flags:'
echo '    --input|-i      : CFG format input structure file (*default=input.cfg)'
echo '    --species|-s    : A string, if provided, will be used as the element symbols'
echo '    --help|-h       : print this help!'
echo '  Example:'
echo '    cfg2vasp -i myfile.cfg -s " Cu Ag"'
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
    --species|-s)  spl="$2"; shift; shift ;;
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
if [ ! -e $inp ]; then write_options; echo "Error: no $inp file exists!"; exit; fi

# ===========================================================================
# Collect required info
# ===========================================================================

# ========== Find the total number of atoms (nat) and coordinate type (direct/cart)
nat=$(grep -A 1 Size $inp | tail -n 1)
direct=0
line=$(grep AtomData $inp)
if [[ $line == *"direct"* ]]; then
  direct=1
fi

# ========== Find number of species: ntp; and unique type ids' array: spt
spc=($(grep -A $nat AtomData $inp | tail -n $nat | awk '{print $2}'))
ntp=${spc[$nat-1]}
((ntp=$ntp+1))

spt=($(echo "${spc[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))
ntp=${#spt[@]}

# ========== Find the list of number of atoms of each species: spn
spn=""
for((i=0;i<ntp;i++)); do
  c=0
  for((j=0;j<nat;j++)); do
    if [ ${spc[$j]} -eq ${spt[$i]} ];then
      ((c=$c+1))
    fi
  done
  spn=$spn" "$c
done

# ========== If not user-provided, produce a "placeholder": spl; and its array: spa
if [ "$spl" == "" ];then
for((i=0;i<ntp;i++));do
  spl=$spl" A"$i
done
fi
spa=($spl)

# A sanity check
if [ ${#spa[@]} -ne $ntp ]; then
  echo "Error: number of element symbols and species types don't match!"; exit
fi

# ========== Lattice vector components (array of 9 numbers)
lat=($(grep -A 3 Supercell $inp | tail -n 3))

# === Total volume of the output unit cell (A^3)
tmp0=$(echo "(((${lat[4]})*(${lat[8]}))-((${lat[7]})*(${lat[5]})))*(${lat[0]})" | bc -l)
tmp1=$(echo "(((${lat[6]})*(${lat[5]}))-((${lat[3]})*(${lat[8]})))*(${lat[1]})" | bc -l)
tmp2=$(echo "(((${lat[3]})*(${lat[7]}))-((${lat[6]})*(${lat[4]})))*(${lat[2]})" | bc -l)
vol=$(echo "($tmp0) + ($tmp1) + ($tmp2)"|bc -l)

# ========== Pressure for the relaxation job; will be read from the output (GPa)
prs="0"
if grep -q "pressure =" $inp
then
  prs=$(grep "pressure =" $inp | cut -d "=" -f2)
fi

# ========== Pressure (kB)
pkb=$(echo "($prs)*($gpa2kb)" | bc -l)

# ========== Atomic posisitions and possibly forces (vectors of length nat*3)
# (MTP outputs extra "id" and "tag" in front of each line, so 5 or 8 entries per line!)
tmp=($(grep -A $nat AtomData $inp | tail -n $nat))
# Do a sanity check, and see if we have forces in the input
hasforces=0
tmp0=${#tmp[@]}
((tmp1=$nat*5))
((tmp2=$nat*8))
if [ $tmp0 -eq $tmp1 ]; then
  factor=5
elif [ $tmp0 -eq $tmp2 ]; then
  factor=8
  hasforces=1
else
  echo Error: number of entry per atom not recognized!; exit
fi
pos=""
frc=""
drf=("0.0" "0.0" "0.0")
for((i=0;i<nat;i++));do
  for((j=0;j<3;j++));do
    ((k=factor*$i+2+$j))
    pos=$pos" "${tmp[$k]}
  done
  if [ $hasforces -eq 1 ]; then
    for((j=0;j<3;j++));do
      ((k=factor*$i+5+$j))
      frc=$frc" "${tmp[$k]}
      drf[$j]=$(echo "(${drf[$j]})+(${tmp[$k]})" | bc -l)
    done
  fi
done
pos=($pos)
frc=($frc)

# ========== CHECK IF DATA FOR OUTCAR ARE PRESENT
makeoutcar=0
if grep -q Energy $inp; then
  if grep -q PlusStress $inp; then
    makeoutcar=1
    # ========== Total energy (eV)
    ene=$(grep -A 1 Energy $inp | tail -n 1 | awk '{print $1}')

    # ========== Enthalpy (different than energy only if prs!=0) and PV (eV)
    ent="$ene"
    pvt="0"
    if [ "$prs" != "0" ];then
      pvt=$(echo "($prs) * ($gpa2ev) * ($vol)" | bc -l)
      ent=$(echo "($ene) + ($pvt)" | bc -l)
    fi

    # ========== Stress in MTP output is [virial_stress*cell_volume] in eV; we convert it to kB here
    # (MTP has "xx yy zz YZ XZ XY" format; while vasp/maise "in kB" is "xx yy zz XY YZ ZX")
    tmp=($(grep -A 1 PlusStress $inp | tail -n 1))
    ikb=""
    for i in 0 1 2 5 3 4; do
    tmp0=$(echo "(${tmp[$i]}) * ($ev2kb) / ($vol)" | bc -l)
    ikb=$ikb"  "$(printf "% 14.8lf" $tmp0)
    done
  fi
fi

# If we don't have forces; we will ignure OUTCAR writing, anyways
if [ $hasforces -eq 0 ]; then
  makeoutcar=0
fi

# ===========================================================================
# Output the VASP CONTCAR file
# ===========================================================================

# ========== First, setup the output file name
out="$str"

# ========== Write the CONTCAR file
echo "cfg 2 poscar"  > $out
echo "1.000"       >> $out
grep -A 3 Supercell $inp | tail -n 3 >> $out
echo $spl >> $out
echo $spn >> $out
if [ $direct -eq 1 ];then
  echo "Direct" >> $out
else
  echo "Cartesian" >> $out
fi
grep -A $nat AtomData $inp | tail -n $nat | awk '{printf "% 22.14lf  % 22.14lf  % 22.14lf\n",$3,$4,$5}' >> $out

# ===========================================================================
# Output the VASP OUTCAR file
# ===========================================================================

# ========== Proceed to OUTCAR only if the data were available
if [ $makeoutcar -eq 0 ]; then
  exit
fi

# ========== First, setup the output file name
out="$efs"

# ========== Write a header and ...
printf "\n"                                                                          > $out
printf "=======================================================================\n"  >> $out
printf "The file is produced from MTP output to mimic the VASP OUTCAR file!    \n"  >> $out
printf "=======================================================================\n"  >> $out
printf "\n"                                                                         >> $out

# ========== Write general info
for((i=0; i<ntp; i++)); do
  printf "  VRHFIN =%s: aa\n" ${spa[$i]}                                            >> $out
done
printf "\n"                                                                         >> $out
echo   "  ions per type = " "$spn"                                                  >> $out
printf "\n"                                                                         >> $out
printf "  number of dos  NEDOS = 00000  number of ions     NIONS = % 3d\n" $nat     >> $out
printf "\n"                                                                         >> $out

# ========== Write configuration info
printf "  volume of cell :      % 12.2lf\n" $vol                                    >> $out
printf "\n"                                                                         >> $out
echo   " in kB " "$ikb"                                                             >> $out
printf "  external pressure =  % 12.2lf  kB  Pullay stress =  00000  kB\n" $pkb     >> $out
printf "\n"                                                                         >> $out
printf "   direct lattice vectors                     reciprocal lattice vectors\n" >> $out
for((i=0; i<3; i++)); do
  for((j=0; j<3; j++)); do
    ((k=$i*3+$j))
    printf "  % 12.6lf" ${lat[$k]}                                                  >> $out
  done
  printf "     00000     00000     00000\n"                                         >> $out
done
printf "\n"                                                                         >> $out
printf "   POSITION                                       TOTAL-FORCE (eV/Angst)\n" >> $out
printf "   ---------------------------------------------------------------------\n" >> $out
for((i=0;i<nat;i++));do
  for((j=0;j<3;j++));do
    ((k=$i*3+$j))
    printf "  % 12.6lf" ${pos[$k]}                                                  >> $out
  done
  for((j=0;j<3;j++));do
    ((k=$i*3+$j))
    printf "  % 12.6lf" ${frc[$k]}                                                  >> $out
  done
  printf "\n"                                                                       >> $out
done
printf "   ---------------------------------------------------------------------\n" >> $out
printf "   total drift:                           "                                 >> $out
printf "  % 12.6lf  % 12.6lf  % 12.6lf\n"  ${drf[0]} ${drf[1]} ${drf[2]}            >> $out
printf "\n"                                                                         >> $out
printf "  free  energy   TOTEN  =  % 16.8lf   eV\n" $ene                            >> $out
printf "\n"                                                                         >> $out
printf "  enthalpy is  TOTEN    =  % 16.8lf   eV   P V=  % 16.8lf\n" $ent $pvt      >> $out
printf "\n"                                                                         >> $out
printf "energy  without entropy= % 16.8lf  energy(sigma->0) = % 16.8lf\n" $ene $ene >> $out
printf "\n"                                                                         >> $out
printf "\n"                                                                         >> $out

# ========== Write the timing info
printf "\n"                                                                         >> $out
printf "  Total CPU time used (sec):   % 12.4lf\n"  0.0                             >> $out

exit
