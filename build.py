import os, sys, subprocess, argparse, hashlib

parser = argparse.ArgumentParser()
parser.add_argument('-d', '--debug', action='store_true', help='build in debug mode')
parser.add_argument('--rebuild', action='store_true', help='rebuild entire solution')

args, unknown_args = parser.parse_known_args()

# configuration
configuration = 'Release'
target        = 'Build'
if args.debug:
  configuration = 'Debug'
if args.rebuild:
  target = 'Rebuild'

def md5sum(filename, blocksize=65536):
  hash = hashlib.md5()
  with open(filename, 'rb') as f:
    for block in iter(lambda: f.read(blocksize), b""):
      hash.update(block)
  return hash.hexdigest()

def run(args, pipe=None):
  try:
    child = subprocess.Popen(args, stdout=pipe)
    out, err = child.communicate()
    if child.returncode != 0:
      sys.exit(1)
  except:
    print(f'ERROR: Failed to run {args[0]}')
    sys.exit(1)
  return out

# run premake and write cache file
def run_premake(binary, target):
  if not os.path.exists(binary):
    print('Downloading premake...')
    out_file = './windows.zip'
    build = True
    run(['curl', '-L', '-sS', 'https://github.com/premake/premake-core/releases/download/v5.0.0-beta1/premake-5.0.0-beta1-windows.zip', '-o', out_file])
    run(['unzip', '-q', out_file, '-d', '.'])
    os.remove(out_file)
  remaining_args = ' '.join(unknown_args)
  run([binary, target, remaining_args])

# run premake
run_premake('./premake5.exe', 'vs2022')

# find visual studio install path
msbuild_path = run(['C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe', '-property', 'installationPath'], subprocess.PIPE).strip().decode('utf-8')

# build with msbuild.exe
msbuild = f"{msbuild_path}/MSBuild/Current/Bin/MSBuild.exe"
run([msbuild, 'out/jc-model-renderer.sln', f'-t:{target}', '-v:minimal', f'-p:Configuration={configuration}'])