# to build the dictionary, you must:

# (1) compile the lookup3 module
# git clone https://github.com/aaronkirkham/py-lookup3
# python setup.py build
# python setup.py install

import sys
import os
import json
import lookup3

ROOT = os.path.dirname(os.path.dirname(os.path.realpath(sys.argv[0])))
OUT_PATH = "%s\\assets\\" % ROOT
FILELIST_PATH = "%s\\assets\\filelist" % ROOT
DICTIONARY = {}

print ("Building filelist dictionary...")

# read the current filelist item, hash the filename and append it to the dictionary
def read_filelist(filename):
  FILE_BASE_NAME = os.path.splitext(os.path.basename(filename))[0]
  filelist_dict = { FILE_BASE_NAME: {} }

  with open(filename, "r") as file:
    # remove the first element as it's just a % of how many hashes are found
    data = file.read().splitlines()
    data.pop(0)

    for filelist_file in data:
      hash = lookup3.hashlittle(filelist_file)
      filelist_dict[FILE_BASE_NAME][hash] = filelist_file

  return filelist_dict

# loop over all the things in the filelist directory
for directory in os.listdir(FILELIST_PATH):
  DIR_ABS_PATH = os.path.join(FILELIST_PATH, directory)

  # skip any files
  if os.path.isdir(DIR_ABS_PATH):
    # list all files in the current directory
    for filename in os.listdir(DIR_ABS_PATH):
      FILE_ABS_PATH = os.path.join(DIR_ABS_PATH, filename)

      # is the current file a directory? (dlc_win64)
      if os.path.isdir(FILE_ABS_PATH):
        # fix weird directories which have an extra archive
        if filename == "bavarium_sea_heist" or filename == "mech_land_assault" or filename == "v0805_sportmech" or filename == "w901_scorpiongun":
          filename += "\\arc"
          FILE_ABS_PATH = os.path.join(DIR_ABS_PATH, filename)

        # update the directory
        key = "%s/%s" % (directory, filename.replace("\\", "/"))

        if not key in DICTIONARY:
          DICTIONARY[key] = {}

        # list all the files in the child directory
        for filename_ in os.listdir(FILE_ABS_PATH):
          # skip the hints file
          if filename_ == "hints.txt":
            continue

          NEW_FILE_ABS_PATH = os.path.join(FILE_ABS_PATH, filename_)
          DICTIONARY[key].update(read_filelist(NEW_FILE_ABS_PATH))
      else:
        if filename == "hints.txt":
            continue

        if not directory in DICTIONARY:
          DICTIONARY[directory] = {}

        DICTIONARY[directory].update(read_filelist(FILE_ABS_PATH))

# create the output directory if we need to
if not os.path.isdir(OUT_PATH):
  os.makedirs(OUT_PATH)

# write the output
OUTPUT_FILE = os.path.join(OUT_PATH, "dictionary.json")
with open(OUTPUT_FILE, "w") as outfile:
  json.dump(DICTIONARY, outfile)

print ("Finished. Wrote dictionary to '%s'." % os.path.abspath(OUTPUT_FILE))