#!/usr/bin/env python3

# ==================================================================== #
#  Wrapper script to perform VASP I/O format optimization with ML-UIPs #
#                                                                      #
#  Copyright (c) 2024, Samad Hajinazar                                 #
#  samadh~at~buffalo.edu                                               #
# ==================================================================== #

# NOTE: This script is tested with MACE v0.3.10 and CHGNet v0.3.8
#       The syntax and details of loading models might differ
#         especially with older versions.
#       Here we use "custom"-style models for MACE
#         (i.e., ".model file full path"), and "version"-based
#         models for CHGNet (i.e., "0.3.0" or "0.2.0").
#       For MatterSim, the model is loaded from a local file.
#       For Orb and SevenNet, the pretrained models are loaded as
#         "orb-v3-conservative-inf-omat" and
#         "7net-mf-ompa"/"mpa", respectively.

# ======== Load basic modules
import warnings
warnings.filterwarnings('ignore')
import os
import io
import sys
import time
import copy
import argparse
from math import sqrt, floor

# ======== Print the header
print("\n===================================================================\n"
      "Wrapper script to perform VASP I/O format optimization with ML-UIPs\n"
      "Samad Hajinazar                                                v1.4\n"
      "===================================================================\n")

# ======== Default models (if needed)
def_mace_mdl   = "/Users/sam/CODES/uip_models/mace-mpa-0-medium.model"
def_chgnet_mdl = "0.3.0"
def_mattersim_mdl = "/Users/sam/CODES/uip_models/mattersim-v1.0.0-5M.pth"
def_orb_mdl = "orb-v3-conservative-inf-omat"
def_sevennet_mdl = ["7net-mf-ompa", "mpa"]

# ======== Constants and conversion factors
gpa2evan =   0.00624150913
evan2gpa = 160.21766197432
gpa2kbar =  10.0
aseoptmz = { "BFGS": "BFGS", "BFGSLINESEARCH": "BFGSLineSearch",
             "LBFGS": "LBFGS", "LBFGSLINESEARCH": "LBFGSLineSearch",
             "GPMIN": "GPMin", "MDMIN": "MDMin", "FIRE": "FIRE"}

# ======== Input arguments (and defaults)
class CustomHelpFormatter(argparse.HelpFormatter):
  def _format_action_invocation(self, action):
    if action.choices:
      metavar = f"{{{','.join(action.choices)}}}"
      return ', '.join(action.option_strings) + ' ' + metavar
    elif action.option_strings:
      parts = ', '.join(action.option_strings)
      if action.metavar is not None:
        parts += "" #f" {action.metavar}"
      elif action.type is not None:
        parts += "" #f" {action.type.__name__}"
      return parts
    return super()._format_action_invocation(action)

parser = argparse.ArgumentParser(formatter_class=CustomHelpFormatter)
parser.add_argument("-u", "--uip", type=str.upper, default="MACE", metavar='UIP',
   choices=['MACE', 'CHGNET', 'MATTERSIM', 'ORB', 'SEVENNET'],
   help="UIP type [%(default)s]")
parser.add_argument("-a", "--algorithm", type=str.upper, default="FIRE", metavar='ALGORITHM',
   choices=['FIRE','BFGS','LBFGS', 'BFGSLINESEARCH', 'LBFGSLINESEARCH','GPMIN','MDMIN'],
   help="optimization algorithm [%(default)s]")
parser.add_argument("-i", "--inputfile", type=str, default="POSCAR",
   help="input structure file name [%(default)s]")
parser.add_argument("-n", "--numsteps", type=int, default=0,
   help="number of optimization steps [%(default)s]")
parser.add_argument("-r", "--relaxtype", type=int, default=3, metavar='RELAXTYPE',
   help="relaxation scheme according to VASP ISIF flag [%(default)s]")
parser.add_argument("-p", "--pressure", type=float, default=0.0,
   help="pressure in GPa [%(default)s]")
parser.add_argument("-c", "--convergence", type=float, default=0.05,
   help="convergence threshold for force in eV/A [%(default)s]")
parser.add_argument("-d", "--distancelimit", type=float, default=1.0,
   help="min acceptable post-opt atomic distance in A; negative: no check [%(default)s]")
parser.add_argument("-f", "--forcelimit", type=float, default=10.0,
   help="max acceptable post-opt atomic force in eV/A; negative: no check [%(default)s]")
parser.add_argument("-m", "--model", type=str, default="default",
   help="UIP-ML potential model [%(default)s]")
parser.add_argument("-s", "--symprec", type=float, default=0.01,
   help="SPGLIB symmetry precision [%(default)s]")
parser.add_argument("-o", "--onlif", action="store_true", default=False,
   help="output only initial and final steps in OUTCAR")

args = parser.parse_args()

# ======== Initiate run variables
default_uip = args.uip.upper()
default_inp = args.inputfile
default_alg = aseoptmz[args.algorithm.upper()]
default_stp = args.numsteps
default_rlx = args.relaxtype
default_cnv = args.convergence
default_gpa = args.pressure
minimum_dis = args.distancelimit
maximum_frc = args.forcelimit
default_mdl = args.model
default_tol = args.symprec
default_onl = args.onlif

# ======== Some initializations and sanity checks
# Does the input file exist?
if not os.path.exists(default_inp):
  print("Error: input file '%s' does not exist!" % default_inp)
  sys.exit()

# Number of steps can't be negative!
if default_stp < 0:
  print("Error: number of steps must be a non-negative integer!")
  sys.exit()

# Pressure in other units
default_pev = default_gpa * gpa2evan

# ======== Load ASE module and set its optimizer
try:
  from ase.io.vasp import read_vasp, write_vasp
  from ase import Atoms, optimize
  from ase.spacegroup import get_spacegroup
  from ase.io.trajectory import Trajectory
  from ase.constraints import FixAtoms
  from ase.filters import FrechetCellFilter
except Exception as e:
  print("Error: failed to load ASE modules");
  sys.exit()

default_opt = eval('optimize.'+default_alg)

# ======== Load UIP modules and set the calculator
if default_uip == 'MACE': ########## MACE UIP
  try:
    from mace.calculators import MACECalculator
    from mace import __version__
  except Exception as e:
    print("Error: failed to load MACE modules.")
    exit()
  # set the default model if none given in the input
  default_ver = __version__
  if default_mdl == 'default':
    default_mdl = def_mace_mdl
  if not os.path.exists(default_mdl):
    print("Error: MACE model file '%s' does not exist!" % default_mdl)
    sys.exit()
  default_cal = MACECalculator(model_paths=default_mdl, models=None,
                   default_dtype='float64') #, device='cuda')
elif default_uip == 'CHGNET': ########## CHGNET UIP
  try:
    from chgnet.model.dynamics import CHGNetCalculator
    from chgnet.model.model import CHGNet
    from chgnet import __version__
  except Exception as e:
    print("Error: failed to load CHGNET modules.")
    exit()
  # set the default model if none given in the input
  default_ver = __version__
  if default_mdl == 'default':
    default_mdl = def_chgnet_mdl
  default_cal = CHGNetCalculator(model=CHGNet.load(model_name=default_mdl))
elif default_uip == 'MATTERSIM': ########## MATTERSIM UIP
  try:
    from mattersim.forcefield import MatterSimCalculator
  except Exception as e:
    print("Error: failed to load MATTERSIM modules.")
    exit()
  # set the default model if none given in the input
  default_ver = "0.0"
  if default_mdl == 'default':
    default_mdl = def_mattersim_mdl
  if not os.path.exists(default_mdl):
    print("Error: MATTERSIM model file '%s' does not exist!" % default_mdl)
    sys.exit()
  import torch
  device = "cuda" if torch.cuda.is_available() else "cpu"
  default_cal = MatterSimCalculator(load_path=default_mdl, device = device)
elif default_uip == "ORB": ########## ORB UIP
  try:
    from orb_models.forcefield import pretrained
    from orb_models.forcefield.calculator import ORBCalculator
  except Exception as e:
    print("Error: failed to load ORB modules.")
    exit()
  #
  default_ver = "0.0"
  if default_mdl == 'default':
    default_mdl = def_orb_mdl
  device="cpu" # or device="cuda"
  orbff = pretrained.ORB_PRETRAINED_MODELS[default_mdl](precision="float32-high")
  default_cal = ORBCalculator(orbff, device=device)
elif default_uip == 'SEVENNET': ########## SEVENNET UIP
  try:
    from sevenn.calculator import SevenNetCalculator
  except Exception as e:
    print("Error: failed to load SEVENNET modules.")
    exit()
  # set the default model if none given in the input
  default_ver = "0.0"
  if default_mdl == 'default':
    default_mdl = def_sevennet_mdl[0]+"_"+def_sevennet_mdl[1]
  default_cal = SevenNetCalculator(def_sevennet_mdl[0], modal=def_sevennet_mdl[1])
else:
  print("Error: unknown UIP type '%s'.")
  exit()

# ======== Output the general run parameters
print("\n====== Inputs\n")
print("Start:                                        %s" % time.ctime())
print("UIP:                                      % 12s" % default_uip)
print("UIP version:                              % 12s" % default_ver)
print("Model:                                        %s" % default_mdl.split('/')[-1])
print("Input file name:                          % 12s" % default_inp)
print("Relaxation algorithm:                     % 12s" % default_alg)
print("Relaxation steps:                         % 12d" % default_stp)
print("Relaxation type:                          % 12d" % default_rlx)
print("Pressure (GPa):                           % 12.6lf" % default_gpa)
print("Force convergence threshold (eV/A):       % 12.6lf" % default_cnv)
print("Smallest acceptable interatomic dist (A): % 12.6lf" % minimum_dis)
print("Largest acceptable atomic force (eV/A):   % 12.6lf" % maximum_frc)
print("Spglib tolerance:                         % 12.6lf" % default_tol)
print("Only initial and final steps in OUTCAR:       %r" % (default_onl))

# ======== Read the input data
# Read the original input structure
org_struc = read_vasp(default_inp)

# ======== Set up the relaxation constrains
fix_atom = fix_cell = fix_volm = False
if default_rlx in {5,6,7}:
  fix_atom = True
if default_rlx in {2,7,8}:
  fix_cell = True
if default_rlx in {2,4,5}:
  fix_volm = True

# ======== Create empty output files for consistency!
f = open("OUTCAR", "w")
f.close()
f = open("CONTCAR", "w")
f.close()

# ======== Relaxation task
print("\n====== Relaxation\n")

# To keep track of the total run time(s)
waltim = 0.0
cputim = 0.0

walt_0 = time.time()
cput_0 = time.process_time()

# Set up the atoms object
atoms = copy.deepcopy(org_struc)
atoms.set_calculator(default_cal)

# Apply constrains
if fix_atom:
  atoms.set_constraint(FixAtoms(mask=[True for atom in atoms]))
fil=FrechetCellFilter(atoms, hydrostatic_strain=fix_cell,
                      constant_volume=fix_volm, scalar_pressure=default_pev)

# Set up the relaxer
relax = default_opt(fil)

# Set up the trajectory
trajectory_buffer = io.BytesIO()
trajc = Trajectory(trajectory_buffer, mode='w', atoms=atoms)
relax.attach(trajc, interval=1)

# Perform the calculation
relax.run(steps=default_stp, fmax=default_cnv)

# Collect the results
result = [atoms for atoms in Trajectory(trajectory_buffer, mode='r')]

cputim += time.process_time() - cput_0
waltim += time.time() - walt_0

# ======== Collect run info for general output and analysis
# Actual number of optimization steps that are performed
act_steps = len(result) - 1

# Initial structure info
ini_struc = result[0]
ini_natms = len(ini_struc)
ini_atsmb = ini_struc.get_chemical_symbols()
ini_types = list(dict.fromkeys(ini_atsmb))
ini_count = [ini_atsmb.count(item) for item in ini_types]
ini_systm = "".join([t+str(c) for t, c in zip(ini_types, ini_count)])
ini_symmt = get_spacegroup(ini_struc, symprec=default_tol).no
ini_energ = ini_struc.get_total_energy()
ini_volum = ini_struc.cell.volume
ini_pvtrm = default_pev * ini_volum
ini_entha = ini_energ + ini_pvtrm

# Final structure info
fin_struc = result[-1]
fin_dists = fin_struc.get_all_distances(mic=True)
fin_symmt = get_spacegroup(fin_struc, symprec=default_tol).no
fin_energ = fin_struc.get_total_energy()
fin_volum = fin_struc.cell.volume
fin_pvtrm = default_pev * fin_volum
fin_entha = fin_energ + fin_pvtrm
fin_coord = fin_struc.get_positions()
fin_cells = fin_struc.get_cell()
fin_recip = fin_struc.get_cell().reciprocal()
fin_lencl = fin_struc.get_cell().lengths()
fin_lenrc = fin_struc.get_cell().reciprocal().lengths()
fin_force = fin_struc.get_forces()

# ======== Check if distances are "acceptable" (if needed)
cnvrg_dists = True
minds = min(fin_lencl) # to take care of 1 atom cells
for i in range(0, len(fin_dists)):
  for j in range(0, len(fin_dists[i])):
    if i != j and fin_dists[i][j] < minds:
      minds = fin_dists[i][j]
      if minds < minimum_dis:
        cnvrg_dists = False
if minimum_dis < 0.0 or default_stp == 0:
  cnvrg_dists = True

# ======== Check if forces are "acceptable" (if needed)
cnvrg_force = True
maxmf = -1.0
for i in range(0, ini_natms):
  totlf = 0.0
  for j in range(0, 3):
    totlf += fin_force[i][j]*fin_force[i][j]
  totlf = sqrt(totlf)
  if totlf > maxmf:
    maxmf = totlf
  if totlf > maximum_frc:
    cnvrg_force = False
if maximum_frc < 0.0 or default_stp == 0:
  cnvrg_force = True

# ======== Output VASP format CONTCAR
atoms = copy.deepcopy(fin_struc)
atoms.wrap()
write_vasp('CONTCAR', atoms, direct=True)

# ======== Output VASP format OUTCAR
f = open("OUTCAR", "w")
f.write("\n==========================================================\n")
f.write("VASP-format OUTCAR produced from optimization with ML-UIPs\n")
f.write("==========================================================\n\n")

# Write general info
for item in ini_types:
  f.write("  VRHFIN =%s: aa\n" % (item))

f.write("\n  ions per type = ")
for item in ini_count:
  f.write(" % 3d" % (item))
f.write("\n")

f.write("\n  number of dos  NEDOS = 00000")
f.write("  number of ions     NIONS = % 3d\n" % ini_natms)
f.write("\n  NSW    =    % d\n" % (default_stp))
f.write("  ISIF   =    % d\n" % (default_rlx))
f.write("  PSTRESS=    % 9.1lf pullay stress\n" % (default_gpa * gpa2kbar))

# Write structure-specific data (all or last step)
if default_onl:
  slst = [0] if len(result) == 1 else [0, len(result) - 1]
else:
  slst = range(0, len(result))
for s in slst:
  natms = len(result[s])
  cells = result[s].get_cell()
  recip = result[s].get_cell().reciprocal()
  lencl = result[s].get_cell().lengths()
  lenrc = result[s].get_cell().reciprocal().lengths()
  coord = result[s].get_positions()
  force = result[s].get_forces()
  stres = -1.0 * evan2gpa * gpa2kbar * result[s].get_stress()
  exprs = sum(stres[0:3]) / 3.0
  energ = result[s].get_total_energy()
  volum = result[s].cell.volume
  pvtrm = default_pev * volum
  entha = energ + pvtrm

  f.write("\n\n" + "-" * 39 + " Ionic step %8d  " % s + "-" * 40 + "\n")

  f.write("\n  in kB")
  for i in {0,1,2,5,3,4}:
    f.write(" % 14.5lf" % (stres[i]))

  f.write("\n  external pressure =       % 9.2lf kB" % exprs)
  f.write("  Pullay stress =        % 9.2lf kB\n" % 0.0)

  f.write("\n  volume of cell :     % 12.2lf\n" % (volum))
  f.write("      direct lattice vectors                 reciprocal lattice vectors\n")
  for i in range(0, 3):
    for j in range(0, 3):
      f.write(" % 14.9lf" % (cells[i][j]))
    for j in range(0, 3):
      f.write(" % 14.9lf" % (recip[i][j]))
    f.write("\n")

  f.write("\n  length of vectors\n")
  for i in range(0,3):
    f.write("  % 14.9lf" % (lencl[i]))
  for i in range(0,3):
    f.write("  % 14.9lf" % (lenrc[i]))
  f.write("\n")

  f.write("\n  POSITION                ")
  f.write("                       TOTAL-FORCE (eV/Angst)\n")
  f.write("  " + "-" * 83 + "\n")
  for i in range(0,natms):
    for j in range(0, 3):
      f.write(" % 13.6lf" % (coord[i][j]))
    f.write("   ")
    for j in range(0, 3):
      f.write(" % 13.6lf" % (force[i][j]))
    f.write("\n")
  f.write("  " + "-" * 83 + "\n")

  f.write("  total drift:" + " " * 31)
  drift = [sum(col) for col in zip(*force)]
  for item in drift:
    f.write(" % 13.6lf" % (item))
  f.write("\n")

  # Write energies
  f.write("\n  free  energy   TOTEN  =   % 16.8lf eV\n" % energ)
  f.write("\n  energy  without entropy=  % 16.8lf" % energ)
  f.write("  energy(sigma->0) =  % 16.8lf\n" % energ)
  f.write("  enthalpy is  TOTEN    =   % 16.8lf eV" % entha)
  f.write("   P V=  % 16.8lf\n" % pvtrm)

# Write the timing info to OUTCAR
# For consistency with XtalOpt workflow, and only for relaxations, the
#  total cpu time will be written with a slight modification if forces/distances
#  thresholds (if any) are not met! This is only to signal XtalOpt of a failed run.
# In general, this can be just removed to produce a normal output.
if default_stp > 0 and not (cnvrg_force and cnvrg_dists):
  f.write("\n  Total_CPU_time_used_(sec):   % 12.4lf\n" % (cputim))
  f.write("         Elapsed time (sec):   % 12.4lf\n" % (waltim))
else:
  # For old XtalOpt compatibility!
  f.write("General timing and accounting informations for this job:\n")
  f.write("\n  Total CPU time used (sec):   % 12.4lf\n" % (cputim))
  f.write("         Elapsed time (sec):   % 12.4lf\n" % (waltim))
  f.write("\n")

# Output error messages if forces/distances are not acceptable!
if default_stp > 0 and not (cnvrg_force and cnvrg_dists):
  f.write("Error: didn't converge in % 5d steps - check forces/distances!\n" % act_steps);
  print("\nError: didn't converge in % 5d steps - check forces/distances!\n\n" % act_steps);

f.close()

# ======== Finally, print overview of the run results
print("\n====== Results\n")
print("Number of atoms and types in unit cell:     % 10d    % 11d" %
       (ini_natms, len(ini_types)))
print("Chemical formula of the cell:                 %s" %
       ini_systm)
print("Min dist and max atomic force (A, eV/A):  % 12.6lf   % 12.6lf" %
       (minds,maxmf))
print("Symmetry of initial and final structures:   % 10d    % 11d" %
       (ini_symmt, fin_symmt))
print("Total initial and final energy (eV):      % 12.6lf   % 12.6lf" %
       (ini_energ, fin_energ))
print("PV terms (eV):                            % 12.6lf   % 12.6lf" %
       (ini_pvtrm, fin_pvtrm))
print("Cell initial and final enthalpy (eV):     % 12.6lf   % 12.6lf" %
       (ini_entha, fin_entha))
print("Energy per atom init final (eV/atom):     % 12.6lf   % 12.6lf" %
       (ini_energ/ini_natms, fin_energ/ini_natms))
print("Enthalpy per atom init final (eV/atom):   % 12.6lf   % 12.6lf" %
       (ini_entha/ini_natms, fin_entha/ini_natms))
print("Optimization performed and max steps:       % 10d    % 11d" %
       (act_steps, default_stp))
print("CPU and wall time of calculations (sec):  % 12.6lf   % 12.6lf" %
       (cputim, waltim))
print("Finish:                                       %s\n" %
        time.ctime())

sys.exit()
