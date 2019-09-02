# to build the filelist, you must:

# (1) compile the lookup3 module
# git clone https://github.com/aaronkirkham/py-lookup3
# python setup.py install

import sys
import os
import struct
import lookup3
import argparse

parser = argparse.ArgumentParser(description='Build filelist files for jc-model-renderer')
parser.add_argument('--game', help='Game selector. (jc3 / jc4)')
parser.add_argument('--path', help='Path to root of game folder', required=True)
parser.add_argument('--filelist', help='Input filelist containing hints to use', required=True)
args = parser.parse_args()

# EXAMPLE: python build_filelist.py --game jc4 --path "D:/steam/steamapps/common/Just Cause 4" --filelist "C:/users/aaron/desktop/filelist.txt" 

ROOT = os.path.dirname(os.path.realpath(sys.argv[0]))

if args.game == "jc4":
  TAB_DIRECTORIES = [
    "archives_win64/boot", "archives_win64/boot/hires", "archives_win64/boot_patch", "archives_win64/cp_deathstalker", "archives_win64/main", "archives_win64/main/ara", "archives_win64/main/bra", "archives_win64/main/eng", "archives_win64/main/fre", "archives_win64/main/ger", "archives_win64/main/hires", "archives_win64/main/ita", "archives_win64/main/mex", "archives_win64/main/rus", "archives_win64/main/spa", "archives_win64/main_patch", "archives_win64/main_patch/ara", "archives_win64/main_patch/bra", "archives_win64/main_patch/eng", "archives_win64/main_patch/ger", "archives_win64/main_patch/hires", "archives_win64/main_patch/ita", "archives_win64/main_patch/rus"
  ]

  OUTPUT_DIRECTORY = "{0}/assets/jc4/filelist/".format(ROOT)
else:
  TAB_DIRECTORIES = [
    "archives_win64", "patch_win64", "dlc_win64/bavarium_sea_heist", "dlc_win64/bavarium_sea_heist/arc", "dlc_win64/mech_land_assault", "dlc_win64/mech_land_assault/arc", "dlc_win64/skin_flame", "dlc_win64/sky_fortress", "dlc_win64/v0300_preorder", "dlc_win64/v0604_preorder", "dlc_win64/v0805_sportmech", "dlc_win64/v0805_sportmech/arc", "dlc_win64/v1401_preorder", "dlc_win64/w142_preorder", "dlc_win64/w163_preorder", "dlc_win64/w901_scorpiongun", "dlc_win64/w901_scorpiongun/arc"
  ]

  OUTPUT_DIRECTORY = "{0}/assets/jc3/filelist/".format(ROOT)

HASH_LOOKUP = {}
FILELIST = {}
TOTAL_FILES = 0
TOTAL_FILES_FOUND = 0
TOTAL_FILES_NOT_FOUND = 0

def add_hash_to_filelist(directory, filename, hash):
  global TOTAL_FILES, TOTAL_FILES_FOUND, TOTAL_FILES_NOT_FOUND

  hash_str = "%08x" % hash
  TOTAL_FILES = TOTAL_FILES + 1

  if hash_str in HASH_LOOKUP:
    FILELIST[directory][filename].append(HASH_LOOKUP[hash_str])
    TOTAL_FILES_FOUND = TOTAL_FILES_FOUND + 1
    print("Found {0}".format(HASH_LOOKUP[hash_str]))
    return True

  TOTAL_FILES_NOT_FOUND = TOTAL_FILES_NOT_FOUND + 1
  return False

# generate the file lookup list from the filelist
with open(args.filelist) as file:
  for line in file.read().splitlines():
    if line:
      hash = lookup3.hashlittle(line)
      HASH_LOOKUP[hash] = line

# read all tab files in the game directory
for directory in TAB_DIRECTORIES:
  current_directory = "{0}/{1}".format(args.path, directory)
  for filename in os.listdir(current_directory):
    if not filename.endswith(".tab"):
      continue

    total_tab_files = 0
    found_tab_files = 0
    new_filename = filename.replace('.tab', '.filelist')

    if not directory in FILELIST:
      FILELIST[directory] = {}

    if not new_filename in FILELIST[directory]:
      FILELIST[directory][new_filename] = []

    # open the tab file
    with open("{0}/{1}".format(current_directory, filename), "rb") as file:
      magic, = struct.unpack('I', file.read(4))
      if magic != 0x424154:
        sys.exit("something is wrong! (input file was not TAB format)")

      if args.game == "jc4":
        # skip header
        file.seek(24)

        # skip compressed blocks
        count, = struct.unpack('I', file.read(4))
        file.seek(8 * count, 1)

        try:
          while True:
            hash, = struct.unpack('I', file.read(4))
            file.seek((4 + 4 + 4 + 1 + 1 + 1 + 1), 1)
            
            total_tab_files = total_tab_files + 1
            if add_hash_to_filelist(directory, new_filename, hash):
              found_tab_files = found_tab_files + 1
        except:
          pass
      else:
        # skip header
        file.seek(12)

        try:
          while True:
            hash, = struct.unpack('I', file.read(4))
            file.seek((4 + 4), 1)

            total_tab_files = total_tab_files + 1
            if add_hash_to_filelist(directory, new_filename, hash):
              found_tab_files = found_tab_files + 1
        except:
          pass

      percentage_found = "{0:.2f}".format((found_tab_files / total_tab_files) * 100)[:-3]
      FILELIST[directory][new_filename].append("; {0}/{1} ({2}%)".format(found_tab_files, total_tab_files, percentage_found))

# write the filelist
for directory, entries in FILELIST.items():
  # create output directories
  current_directory = "{0}/{1}".format(OUTPUT_DIRECTORY, directory)
  if not os.path.exists(current_directory):
    os.makedirs(current_directory)

  # write each game{x}.filelist files
  for filelist, filenames in FILELIST[directory].items():
    with open("{0}/{1}".format(current_directory, filelist), "w") as file:
      # write the filename
      for filename in sorted(filenames):
        file.write("{0}\n".format(filename))

#write the total stats file
with open("{0}/status.txt".format(OUTPUT_DIRECTORY), "w") as file:
  percentage_found = "{0:.2f}".format((TOTAL_FILES_FOUND / TOTAL_FILES) * 100)[:-3]
  file.write("{0}/{1} ({2}%)".format(TOTAL_FILES_FOUND, TOTAL_FILES, percentage_found))

print("\n{0} hints provided. {1} Found, {2} Unknown. ({3}%)".format(TOTAL_FILES, TOTAL_FILES_FOUND, TOTAL_FILES_NOT_FOUND, percentage_found))