# to build the dictionary, you must:

# (1) compile the lookup3 module
# git clone https://github.com/aaronkirkham/py-lookup3
# python setup.py install

import sys
import os
import lookup3
import json
from pathlib import Path

ROOT = os.path.dirname(os.path.dirname(os.path.realpath(sys.argv[0])))
FILELIST_DIRECTORIES = ["archives_win64", "patch_win64", "dlc_win64/bavarium_sea_heist", "dlc_win64/bavarium_sea_heist/arc", "dlc_win64/mech_land_assault", "dlc_win64/mech_land_assault/arc", "dlc_win64/skin_flame", "dlc_win64/sky_fortress", "dlc_win64/v0300_preorder", "dlc_win64/v0604_preorder", "dlc_win64/v0805_sportmech", "dlc_win64/v0805_sportmech/arc", "dlc_win64/v1401_preorder", "dlc_win64/w142_preorder", "dlc_win64/w163_preorder", "dlc_win64/w901_scorpiongun", "dlc_win64/w901_scorpiongun/arc"]
PATH_SORT_HIERARCHY = ["dlc_win64", "patch_win64", "archives_win64"]
FILELIST = {}

# iterate over the filelist directories
for directory in FILELIST_DIRECTORIES:
  path = Path("{0}/assets/filelist/{1}".format(ROOT, directory))
  for filename in path.glob('*.filelist'):
    with open(filename, "r") as file:
      for line in file.read().splitlines():
        # ignore stats lines
        if not line.startswith(';'):
          path = "%s/%s" % (directory, os.path.basename(filename).replace('.filelist', ''))

          # append paths if we already have this line so patch_win64 works
          if not line in FILELIST:
            hash = lookup3.hashlittle(line)
            FILELIST[line] = { "path": [path], "hash": hash }
          else:
            FILELIST[line]["path"].append(path)

def sort_cmp(word):
  l = [PATH_SORT_HIERARCHY.index(i) for i in PATH_SORT_HIERARCHY if i in word]
  return l[0]

# order the filelist if it has multiple paths
# hierarchy - dlc_win64 > patch_win64 > archives_win64
for filename, data in FILELIST.items():
  if len(data["path"]) > 1:
    FILELIST[filename]["path"] = sorted(data["path"], key=sort_cmp)

# write the dictionary json
with open("{0}/assets/dictionary.json".format(ROOT), "w") as file:
  json.dump(FILELIST, file)