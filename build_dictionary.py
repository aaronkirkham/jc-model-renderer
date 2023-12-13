import sys, os, json, argparse
from pathlib import Path

parser = argparse.ArgumentParser(description='Build dictionary lookup files for jc-model-renderer')
parser.add_argument('--pretty', help='Pretty print output JSON', action='store_true')
parser.add_argument('--game', help='Game selector. (jc3 / jc4)')
args = parser.parse_args()

FILELIST = {}

# jc4 directories
if args.game == "jc4":
  ASSETS = os.path.dirname(os.path.realpath(sys.argv[0])) + "/assets/jc4"

  FILELIST_DIRECTORIES = [
    "archives_win64/boot", "archives_win64/boot/hires", "archives_win64/boot_patch",
    "archives_win64/cp_deathstalker", "archives_win64/main", "archives_win64/main/ara",
    "archives_win64/main/bra", "archives_win64/main/eng", "archives_win64/main/fre",
    "archives_win64/main/ger", "archives_win64/main/hires", "archives_win64/main/ita",
    "archives_win64/main/mex", "archives_win64/main/rus", "archives_win64/main/spa",
    "archives_win64/main_patch", "archives_win64/main_patch/ara", "archives_win64/main_patch/bra",
    "archives_win64/main_patch/eng", "archives_win64/main_patch/ger", "archives_win64/main_patch/hires",
    "archives_win64/main_patch/ita", "archives_win64/main_patch/rus"
  ]
  FILELIST_SORT_HIERARCHY = [
    "archives_win64/main_patch", "archives_win64/main", "archives_win64/boot_patch",
    "archives_win64/boot", "archives_win64/cp_deathstalker"
  ]
# jc3 directories
else:
  ASSETS = os.path.dirname(os.path.realpath(sys.argv[0])) + "/assets/jc3"

  FILELIST_DIRECTORIES = [
    "archives_win64", "patch_win64", "dlc_win64/bavarium_sea_heist", "dlc_win64/bavarium_sea_heist/arc",
    "dlc_win64/mech_land_assault", "dlc_win64/mech_land_assault/arc", "dlc_win64/skin_flame", "dlc_win64/sky_fortress",
    "dlc_win64/v0300_preorder", "dlc_win64/v0604_preorder", "dlc_win64/v0805_sportmech", "dlc_win64/v0805_sportmech/arc",
    "dlc_win64/v1401_preorder", "dlc_win64/w142_preorder", "dlc_win64/w163_preorder", "dlc_win64/w901_scorpiongun",
    "dlc_win64/w901_scorpiongun/arc"
  ]
  FILELIST_SORT_HIERARCHY = [
    "dlc_win64", "patch_win64", "archives_win64"
  ]

# iterate over the filelist directories
for directory in FILELIST_DIRECTORIES:
  path = Path("{0}/filelist/{1}".format(ASSETS, directory))
  for filename in path.glob('*.filelist'):
    with open(filename, "r") as file:
      for line in file.read().splitlines():
        # ignore stats lines
        if not line.startswith(';'):
          path = "%s/%s" % (directory, os.path.basename(filename).replace('.filelist', ''))

          # append paths if we already have this line so patch_win64 works
          if not line in FILELIST:
            FILELIST[line] = [path]
          else:
            FILELIST[line].append(path)

def sort_cmp(word):
  l = [FILELIST_SORT_HIERARCHY.index(i) for i in FILELIST_SORT_HIERARCHY if i in word]
  return l[0]

# order the filelist if it has multiple paths
for filename, data in FILELIST.items():
  if len(data) > 1:
    FILELIST[filename] = sorted(data, key=sort_cmp)

# write the dictionary json
with open("{0}/dictionary.json".format(ASSETS), "w") as file:
  if args.pretty:
    file.write(json.dumps(FILELIST, indent=4, sort_keys=True))
  else:
    file.write(json.dumps(FILELIST, sort_keys=True))