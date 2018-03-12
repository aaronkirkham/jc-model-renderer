# to build the dictionary, you must:

# (1) compile the lookup3 module
# git clone https://github.com/aaronkirkham/py-lookup3
# python setup.py build
# python setup.py install

import sys
import os
import struct
import lookup3
import json
from decimal import Decimal

ROOT = os.path.dirname(os.path.dirname(os.path.realpath(sys.argv[0])))
JC3_DIRECTORY = "D:\\Steam\\steamapps\\common\\Just Cause 3"
OUT_PATH = "%s\\assets\\" % ROOT
FILELIST_PATH = "%s\\assets\\filelist" % ROOT
HINTS = {}
FILELIST = {}
WRITE_PRETTY_JSON = False

print ("Generating filelist dictionary from hints...")

def load_hints_file(filename):
  # does the hints file exists?
  if os.path.isfile(filename):
    with open(filename, "r") as file:
      # skip empty lines
      lines = (line.rstrip() for line in file) 
      lines = list(line for line in lines if line)

      for line in lines:
        line_hash = lookup3.hashlittle(line)
        HINTS[line_hash] = line

# load the hints files
for directory in os.listdir(FILELIST_PATH):
  DIR_ABS_PATH = os.path.join(FILELIST_PATH, directory)
  if os.path.isdir(DIR_ABS_PATH):
    if directory != "dlc_win64":
      load_hints_file(DIR_ABS_PATH + "\\hints.txt")
    else:
      for filename in os.listdir(DIR_ABS_PATH):
        FILE_ABS_PATH = os.path.join(DIR_ABS_PATH, filename)

        # specific folders with arc subdirectory
        if filename == "bavarium_sea_heist" or filename == "mech_land_assault" or filename == "v0805_sportmech" or filename == "w901_scorpiongun":
          filename += "\\arc"
          FILE_ABS_PATH = os.path.join(DIR_ABS_PATH, filename)

        load_hints_file(FILE_ABS_PATH + "\\hints.txt")

print("loaded %s hints" % len(HINTS))

def read_entry_from_tab_header(filename, abs_filepath):
  # we only care about the .tab files
  name, extension = os.path.splitext(filename)
  if extension != ".tab":
    return False

  # get the relative path to the file
  path = abs_filepath.replace(JC3_DIRECTORY, "").replace("\\", "/")[1:]
  directory, archive = os.path.split(path)
  archive_name = os.path.splitext(archive)[0]

  # stats
  found_hashes = 0
  total_hashes = 0

  if not directory in FILELIST:
    FILELIST[directory] = {}

  if not archive in FILELIST[directory]:
    FILELIST[directory][archive_name] = {}

  with open(abs_filepath, "rb") as file:
    # ensure the magic is correct
    magic, = struct.unpack('I', file.read(4))
    if magic != 0x424154:
      raise Exception("Invalid header magic! (File probably isn't an archive table file)")

    # skip the tab header
    file.seek(12)

    # read the rest of the file until we're done
    try:
      while True:
        # get the namehash and skip the extra
        hash, = struct.unpack('I', file.read(4))
        padding = file.read(8)

        hash_str = "%08x" % hash
        total_hashes += 1

        # do we have the hash string in the hints dictionary?
        if hash_str in HINTS:
          FILELIST[directory][archive_name][hash_str] = HINTS[hash_str]
          found_hashes += 1
    except:
      pass

  # stats
  percentage_found = Decimal(str(100 * (found_hashes / total_hashes))).to_integral()
  FILELIST[directory][archive_name]["0"] = "%s/%s %s%%" % (found_hashes, total_hashes, percentage_found)

# find the tab files
for directory in os.listdir(JC3_DIRECTORY):
  DIR_ABS_PATH = os.path.join(JC3_DIRECTORY, directory)
  if directory == "archives_win64" or directory == "dlc_win64" or directory == "patch_win64":
     # list all files in the current directory
    for filename in os.listdir(DIR_ABS_PATH):
      FILE_ABS_PATH = os.path.join(DIR_ABS_PATH, filename)

      # is the current file a directory? (dlc_win64)
      if os.path.isdir(FILE_ABS_PATH):
        # fix weird directories which have an extra archive
        if filename == "bavarium_sea_heist" or filename == "mech_land_assault" or filename == "v0805_sportmech" or filename == "w901_scorpiongun":
          filename += "\\arc"
          FILE_ABS_PATH = os.path.join(DIR_ABS_PATH, filename)

        for filename_ in os.listdir(FILE_ABS_PATH):
          read_entry_from_tab_header(filename_, os.path.join(FILE_ABS_PATH, filename_))
      else:
        read_entry_from_tab_header(filename, FILE_ABS_PATH)

# write the output
OUTPUT_FILE = os.path.join(OUT_PATH, "dictionary.json")
with open(OUTPUT_FILE, "w") as file:
  if WRITE_PRETTY_JSON:
    file.write(json.dumps(FILELIST, indent=4, sort_keys=True))
  else:
    json.dump(FILELIST, file)

print ("Finished. Wrote dictionary to '%s'." % os.path.abspath(OUTPUT_FILE))