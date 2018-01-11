#!/usr/bin/env python
import optparse
import os
import pprint
import re
import shlex
import subprocess
import sys
import shutil
import string

# gcc and g++ as defaults matches what GYP's Makefile generator does,
# except on OS X.
CC = os.environ.get('CC', 'cc' if sys.platform == 'darwin' else 'gcc')
CXX = os.environ.get('CXX', 'c++' if sys.platform == 'darwin' else 'g++')

root_dir = os.path.dirname(__file__)
sys.path.insert(0, os.path.join(root_dir, 'tools', 'gyp', 'pylib'))
from gyp.common import GetFlavor

# parse our options
parser = optparse.OptionParser()

valid_os = ('win', 'mac', 'solaris', 'freebsd', 'openbsd', 'linux', 'android')
valid_arch = ('arm', 'arm64', 'ia32', 'mips', 'mipsel', 'x32', 'x64', 'x86')
valid_arm_float_abi = ('soft', 'softfp', 'hard')
valid_mips_arch = ('loongson', 'r1', 'r2', 'r6', 'rx')
valid_mips_fpu = ('fp32', 'fp64', 'fpxx')
valid_mips_float_abi = ('soft', 'hard')
valid_intl_modes = ('none', 'small-icu', 'full-icu', 'system-icu')

parser.add_option('--debug',
    action='store_true',
    dest='debug',
    help='also build debug build')

parser.add_option('--dest-cpu',
    action='store',
    dest='dest_cpu',
    choices=valid_arch,
    help='CPU architecture to build for ({0})'.format(', '.join(valid_arch)))

parser.add_option('--xcode',
    action='store_true',
    dest='use_xcode',
    help='Generate xcode files')

parser.add_option('--dest-os',
    action='store',
    dest='dest_os',
    choices=valid_os,
    help='operating system to build for ({0})'.format(', '.join(valid_os)))

(options, args) = parser.parse_args()

def warn(msg):
  warn.warned = True
  prefix = '\033[1m\033[93mWARNING\033[0m' if os.isatty(1) else 'WARNING'
  print('%s: %s' % (prefix, msg))

# track if warnings occured
warn.warned = False

def b(value):
  """Returns the string 'true' if value is truthy, 'false' otherwise."""
  if value:
    return 'true'
  else:
    return 'false'

def cc_macros():
  """Checks predefined macros using the CC command."""

  try:
    p = subprocess.Popen(shlex.split(CC) + ['-dM', '-E', '-'],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
  except OSError:
    print '''jc3-handling-editor configure error: No acceptable C compiler found!

        Please make sure you have a C compiler installed on your system and/or
        consider adjusting the CC environment variable if you installed
        it in a non-standard prefix.
        '''
    sys.exit()

  p.stdin.write('\n')
  out = p.communicate()[0]

  out = str(out).split('\n')

  k = {}
  for line in out:
    lst = shlex.split(line)
    if len(lst) > 2:
      key = lst[1]
      val = lst[2]
      k[key] = val
  return k


def host_arch_cc():
  """Host architecture check using the CC command."""

  k = cc_macros()

  matchup = {
    '__aarch64__' : 'arm64',
    '__i386__'    : 'ia32',
    '__x86_64__'  : 'x64',
  }

  rtn = 'ia32' # default

  for i in matchup:
    if i in k and k[i] != '0':
      rtn = matchup[i]
      break

  return rtn


def host_arch_win():
  """Host architecture check using environ vars (better way to do this?)"""

  observed_arch = os.environ.get('PROCESSOR_ARCHITECTURE', 'x86')
  arch = os.environ.get('PROCESSOR_ARCHITEW6432', observed_arch)

  matchup = {
    'AMD64'  : 'x64',
    'x86'    : 'ia32',
    'arm'    : 'arm',
    'mips'   : 'mips',
  }

  return matchup.get(arch, 'ia32')

def write(filename, data):
  filename = os.path.join(root_dir, filename)
  print 'creating ', filename
  f = open(filename, 'w+')
  f.write(data)

do_not_edit = '# Do not edit. Generated by the configure script.\n'

output = {
  'variables': { 'python': sys.executable,
                  'deps_path': os.path.relpath('deps/') },
  'include_dirs': [],
  'libraries': [],
  'defines': [],
  'cflags': [],
}

host_arch = 'x64'
target_arch = options.dest_cpu or host_arch
  # ia32 is preferred by the build tools (GYP) over x86 even if we prefer the latter
  # the Makefile resets this to x86 afterward
if target_arch == 'x86':
  target_arch = 'ia32'

output['variables']['host_arch'] = host_arch
output['variables']['target_arch'] = target_arch

# Should we add a compiler check here?

# determine the "flavor" (operating system) we're building for,
# leveraging gyp's GetFlavor function
flavor_params = {}
if (options.dest_os):
  flavor_params['flavor'] = options.dest_os
  
flavor = GetFlavor(flavor_params)

# variables should be a root level element,
# move everything else to target_defaults
variables = output['variables']
del output['variables']

# make_global_settings should be a root level element too
if 'make_global_settings' in output:
  make_global_settings = output['make_global_settings']
  del output['make_global_settings']
else:
  make_global_settings = False

output = {
  'variables': variables,
  'target_defaults': output,
}
if make_global_settings:
  output['make_global_settings'] = make_global_settings

pprint.pprint(output, indent=2)

write('config.gypi', do_not_edit +
      pprint.pformat(output, indent=2) + '\n')

config = {
  'BUILDTYPE': 'Debug' if options.debug else 'Release',
 # 'USE_XCODE': str(int(options.use_xcode or 0)),
  'PYTHON': sys.executable,
}

config = '\n'.join(map('='.join, config.iteritems())) + '\n'

write('config.mk',
      '# Do not edit. Generated by the configure script.\n' + config)

gyp_args = [sys.executable, 'tools/gyp_jc3_rbm_renderer.py', '--no-parallel']

if options.use_xcode:
  gyp_args += ['-f', 'xcode']
elif flavor == 'win' and sys.platform != 'msys':
  gyp_args += ['-f', 'msvs', '-G', 'msvs_version=auto']
else:
  gyp_args += ['-f', 'make-' + flavor]

gyp_args += args

if warn.warned:
  warn('warnings were emitted in the configure phase')

sys.exit(subprocess.call(gyp_args))