# build patched filelist based on .ee.toc
# NOTE: dictionary must be generated before and after this is run!

import sys
import os
import struct
import lookup3
import json

ROOT = os.path.dirname(os.path.dirname(os.path.realpath(sys.argv[0])))
JC3_DIRECTORY = "D:\\Steam\\steamapps\\common\\Just Cause 3"
DICTIONARY = "%s\\assets\\dictionary.json" % ROOT
OUT_GENERATED_FILE = "%s\\assets\\filelist\\archives_win64\\generated.txt" % ROOT
OUT_PATHCED_FILE = "%s\\assets\\filelist\\patch_win64\\generated.txt" % ROOT
TOCS_OFFSETS_TO_READ = {}
TOCS_TO_READ = {}
GENERATED_FILELIST = []
PATCHED_FILELIST = []

# exit early if no dictionary is present
if not os.path.isfile(DICTIONARY):
  sys.exit("Couldn't find dictionary.json. Please run `build_dictionary.py` first.")

# get a list of tocs to read
with open(DICTIONARY) as file:
  data = json.load(file)
  for k, v in data.items():
    archive = v['path'][0]

    # generate the .toc names
    if ".ee" in k and not ".toc" in k:
      toc_name = k + '.toc'
      toc_name_hash = lookup3.hashlittle(toc_name)
      GENERATED_FILELIST.append(toc_name)

      # if we generated the name, add this to to the TOC list
      if not archive in TOCS_OFFSETS_TO_READ:
        TOCS_OFFSETS_TO_READ[archive] = []
        TOCS_TO_READ[archive] = []
      TOCS_OFFSETS_TO_READ[archive].append({ 'name': toc_name, 'name_hash': toc_name_hash })

    # tocs to read
    if ".toc" in k:
      if not archive in TOCS_OFFSETS_TO_READ:
        TOCS_OFFSETS_TO_READ[archive] = []
        TOCS_TO_READ[archive] = []
      TOCS_OFFSETS_TO_READ[archive].append({ 'name': k, 'name_hash': v['namehash'] })

print("reading file offsets from tab...")

# read the tocs from the tab file
total_files_found = 0
for k, v in TOCS_OFFSETS_TO_READ.items():
  # get the path to the tab file
  tab_path = JC3_DIRECTORY + "\\" + k + ".tab"

  with open(tab_path, "rb") as file:
    # ensure the magic is correct
    magic, = struct.unpack('I', file.read(4))
    if magic != 0x424154:
      raise Exception("Invalid header magic! (File probably isn't an archive table file)")

    # skip the tab header
    file.seek(12)

    # read the file until we find the file we're looking for
    try:
      while True:
        hash, = struct.unpack('I', file.read(4))
        offset, = struct.unpack('I', file.read(4))
        size, = struct.unpack('I', file.read(4))

        hash_str = "%08x" % hash

        # try find the hash in the TOCS to read list
        for x in v:
          if x['name_hash'] == hash_str:
            TOCS_TO_READ[k].append({ 'offset': offset, 'size': size })
            total_files_found += 1
    except:
      pass

print("done. found %s files." % total_files_found)
print("reading file data from arc...")

# read the tocs from the arc file
for k, v in TOCS_TO_READ.items():
  arc_path = JC3_DIRECTORY + "\\" + k + ".arc"
  with open(arc_path, "rb") as file:
    # read each file from the archive
    for x in v:
      file.seek(x['offset'])

      # read and parse the TOC data
      current_offset = 0
      while current_offset < x['size']:
        length = struct.unpack('I', file.read(4))[0]
        filename = file.read(length).decode('utf-8')
        offset = struct.unpack('I', file.read(4))[0]
        size = struct.unpack('I', file.read(4))[0]

        # we only care about patched files
        if offset == 0:
          PATCHED_FILELIST.append(filename)

        current_offset = (current_offset + 4 + length + 4 + 4)

# write the new generated names
with open(OUT_GENERATED_FILE, "w") as file:
  for filename in GENERATED_FILELIST:
    file.write("%s\n" % filename);

# write the patched file stuff
with open(OUT_PATHCED_FILE, "w") as file:
  for filename in PATCHED_FILELIST:
    file.write("%s\n" % filename)

print("finished. now you should rebuild the dictionary.")