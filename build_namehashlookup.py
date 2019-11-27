import sys
import os
import struct
import json
import argparse
from decimal import Decimal

parser = argparse.ArgumentParser(description='Build namehash lookup files for jc-model-renderer')
parser.add_argument('--pretty', help='Pretty print output JSON', action='store_true')
args = parser.parse_args()

ROOT = os.path.dirname(os.path.realpath(sys.argv[0]))
GENERATED_HASHES = "%s\\assets\\namehashlookup_generated.txt" % ROOT
CUSTOM_HASHES = "%s\\assets\\namehashlookup_custom.txt" % ROOT
JC4_HASHES = "%s\\assets\\namehashlookup_jc4.txt" % ROOT
OUT_PATH = "%s\\assets\\" % ROOT
DATA = []

print ("Generating namehash lookup...")

def load_hints(filename):
  with open(filename, "r") as file:
    # skip empty lines
    lines = (line.rstrip() for line in file) 
    lines = list(line for line in lines if line)

    for line in lines:
      DATA.append(line)

# load hints
load_hints(GENERATED_HASHES)
load_hints(CUSTOM_HASHES)
load_hints(JC4_HASHES)

# write the output
OUTPUT_FILE = os.path.join(OUT_PATH, "namehashlookup.json")
with open(OUTPUT_FILE, "w") as file:
  if args.pretty:
    file.write(json.dumps(DATA, indent=4, sort_keys=True))
  else:
    json.dump(DATA, file)

print ("Finished. Wrote namehash lookup to '%s'." % os.path.abspath(OUTPUT_FILE))