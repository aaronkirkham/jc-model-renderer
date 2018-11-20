# to build the namehash lookup, you must:

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

ROOT = os.path.dirname(os.path.realpath(sys.argv[0]))
GENERATED_HASHES = "%s\\assets\\filelist\\namehashlookup_generated.txt" % ROOT
CUSTOM_HASHES = "%s\\assets\\filelist\\namehashlookup_custom.txt" % ROOT
OUT_PATH = "%s\\assets\\" % ROOT
DATA = {}
WRITE_PRETTY_JSON = False

print ("Generating namehash lookup...")

with open(GENERATED_HASHES, "r") as file:
  # skip empty lines
  lines = (line.rstrip() for line in file) 
  lines = list(line for line in lines if line)

  for line in lines:
    line_hash = lookup3.hashlittle(line)
    DATA[line_hash] = line
	
with open(CUSTOM_HASHES, "r") as file:
  # skip empty lines
  lines = (line.rstrip() for line in file) 
  lines = list(line for line in lines if line)

  for line in lines:
    line_hash = lookup3.hashlittle(line)
    DATA[line_hash] = line

# write the output
OUTPUT_FILE = os.path.join(OUT_PATH, "namehashlookup.json")
with open(OUTPUT_FILE, "w") as file:
  if WRITE_PRETTY_JSON:
    file.write(json.dumps(DATA, indent=4, sort_keys=True))
  else:
    json.dump(DATA, file)

print ("Finished. Wrote namehash lookup to '%s'." % os.path.abspath(OUTPUT_FILE))