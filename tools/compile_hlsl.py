import optparse
import os.path
import re
import subprocess
import sys

def ConvertToCamelCase(input):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', input)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

def CompileMultipleHLSLShadersToOneHeaderFile(fxc_compiler_path, source_hlsl_file, output_file, inc_file):
	"""Compiles shader from a .hlsl file into a single header."""
	header_output = []
	
	file_name_only = os.path.basename(source_hlsl_file)
	base_filename, _ = os.path.splitext(file_name_only)
	shader_model = ''
	output_name = ''
	
	# get the actual file name and extension now
	the_filename, the_extension = os.path.splitext(base_filename)
	file_type = the_extension[1:]
	
	# skip this file if we don't know the type
	if not file_type == 'vs' and not file_type == 'ps' and not file_type == 'gs':
		return
		
	shader_model = '%s_5_0' % file_type
	output_name = '%s_main' % file_type
	
	# setup the compiler options
	command = [fxc_compiler_path,
		source_hlsl_file,
		'/E', 'main',
		'/T',  shader_model,
		'/Vn', output_name,
		'/Fh', output_file,
		'/O3'
	]
	
	# compile the shader
	(out, err) = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False).communicate()
	if err:
		print 'Error while compiling file %s' % (source_hlsl_file)
		print err
		sys.exit(1)
		
	with open(output_file, 'r') as header:
		header_output.append(header.read())
		
	shaderName, _ = os.path.splitext(base_filename)
	classname = 'hlsl_%s' % (ConvertToCamelCase(shaderName))
	with open(output_file, 'w') as cc:
		cc.write('#pragma once\n\n')
		
		cc.write('namespace %s {\n\n' % classname)
		cc.write(''.join(header_output))
		cc.write('\n}  // namespace %s\n' % classname)
		
	skip_pragma = os.path.isfile(inc_file)
	include_file_str = '#include "' + os.path.basename(output_file)  + '"';
	write_include_str = True
	
	with open(inc_file, 'a+') as cc:
		if not skip_pragma:
			cc.write('#pragma once\n\n')
			cc.seek(0)
		
		# should we write this line?
		for line_number, line in enumerate(cc.read().splitlines(), 1):
			if line == include_file_str:
				write_include_str = False
			
	if write_include_str:
		with open(inc_file, 'a') as cc:
			cc.write(include_file_str + '\n')
	
if __name__ == '__main__':
	parser = optparse.OptionParser()
	parser.add_option('--fxc', dest='compiler')
	parser.add_option('--input', dest='hlsl_file')
	parser.add_option('--output', dest='output_file')
	parser.add_option('--inc', dest='inc_file')
	(options, args) = parser.parse_args()
	
	hlsl_file = os.path.abspath(options.hlsl_file)
	CompileMultipleHLSLShadersToOneHeaderFile(options.compiler, hlsl_file, options.output_file, options.inc_file)