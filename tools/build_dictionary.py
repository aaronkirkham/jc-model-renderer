# to build the dictionary, you must:

# (1) compile the lookup3 module
# git clone https://github.com/aaronkirkham/py-lookup3
# python setup.py build
# python setup.py install

# (2) download gibbed tools and update the GIBBED_PATH variable to
# point to the just cause 3 project files directory
# https://justcause3mods.com/mods/modified-gibbeds-tools/

import os
import json
import lookup3

OUT_PATH = "%s" % ("../assets/" if os.getcwd().find('tools') != -1 else "assets/")
GIBBED_PATH = "C:/Users/aaron/Downloads/Modified-Gibbeds-Tools/bin/projects/Just Cause 3/files"
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

# loop over all the things in the gibbed directory
for directory in os.listdir(GIBBED_PATH):
  DIR_ABS_PATH = os.path.join(GIBBED_PATH, directory)

  ###### TEMP ######
  if directory != "patch_win64":
    continue

  DICTIONARY[directory] = {}

  # skip any files
  if os.path.isdir(DIR_ABS_PATH):
    # list all files in the current directory
    for filename in os.listdir(DIR_ABS_PATH):
      FILE_ABS_PATH = os.path.join(DIR_ABS_PATH, filename)

      # TODO: Ignore weird files which just seem to be duplicates

      # read the filelist into the dictionary
      DICTIONARY[directory].update(read_filelist(FILE_ABS_PATH))

# write the output
OUTPUT_FILE = os.path.join(OUT_PATH, "dictionary.json")
with open(OUTPUT_FILE, "w") as outfile:
  json.dump(DICTIONARY, outfile)

print ("Finished. Wrote dictionary to '%s'." % os.path.abspath(OUTPUT_FILE))