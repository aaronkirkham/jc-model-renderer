#!/usr/bin/env python

import glob
import os
import sys
import subprocess

def main():
	os.chdir('../')

	for file in glob.glob('assets/shaders/*.hlsl'):
		if ".vs.hlsl" in file:
			subprocess.call([sys.executable, 'tools/compile_shader.py', file, '-e', 'main', '-p', 'vs_5_0'])
		elif ".ps.hlsl" in file:
			subprocess.call([sys.executable, 'tools/compile_shader.py', file, '-e', 'main', '-p', 'ps_5_0'])
	
if __name__ == '__main__':
  sys.exit(main())