# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os.path
import re
import subprocess
import sys

def ConvertToCamelCase(input):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', input)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

def ExtractShaderTargetNamesFromSource(source_hlsl_file):
	"""Parses '@gyp_compile' and '@gyp_namespace' metadata from an .hlsl file."""
	# matches strings like // @gyp_compile(arg_a, arg_b) ...
	gyp_compile = re.compile('^// *@gyp_compile\( *(?P<profile>[a-zA-Z0-9_]+) *,'
							 ' *(?P<function_name>[a-zA-Z0-9_]+) *\).*')
		
	# matches strings like // @gyp_namespace(arg_a) ...
	gyp_namespace = re.compile('^// *@gyp_namespace\( *(?P<namespace>[a-zA-Z0-9_]) *\).*')
	
	gyp_namespace = re.compile(
		'^// *@gyp_namespace\( *(?P<namespace>[a-zA-Z0-9_]+) *\).*')

	shader_targets = []  # tuples like ('vs_2_0', 'vertexMain')
	namespace = None
	
	with open(source_hlsl_file) as hlsl:
		for line_number, line in enumerate(hlsl.read().splitlines(), 1):
			m = gyp_compile.match(line)
			if m:
				shader_targets.append((m.group('profile'), m.group('function_name')))
				continue
				
			m = gyp_namespace.match(line)
			if m:
				namespace = m.group('namespace')
				continue
				
			if '@gyp' in line:
				print '%s(%d) : warning: ignoring malformed @gyp directive ' % (source_hlsl_file, line_number)

	if not shader_targets:
		print ("""%s(%d) : error: Reached end of file without finding @gyp_compile directive.

		By convention, each HLSL source must contain one or more @gyp_compile
		directives in its comments, as metadata informing the Chrome build tool
		which entry points should be compiled. For example, to specify compilation
		of a function named 'vertexMain' as a shader model 2 vertex shader:

		// @gyp_compile(vs_2_0, vertexMain)

		Or to compile a pixel shader 2.0 function named 'someOtherShader':

		// @gyp_compile(ps_2_0, someOtherShader)

		To wrap everything in a C namespace 'foo_bar', add a line somewhere like:

		// @gyp_namespace(foo_bar)

		(Namespaces are optional)
		""" % (source_hlsl_file, line_number))
		
		sys.exit(1)
	return (shader_targets, namespace)

def GetCppVariableName(function_name):
	return 'k%s' % ConvertToCamelCase(function_name)

def CompileMultipleHLSLShadersToOneHeaderFile(fxc_compiler_path, source_hlsl_file, namespace, shader_targets, output_file):
	"""Compiles specified shaders from an .hlsl file into a single C header."""
	header_output = []
	
	# Invoke the compiler one at a time to write the c header file,
	# then read that header file into |header_output|.
	for (compiler_profile, hlsl_function_name) in shader_targets:
		file_name_only = os.path.basename(source_hlsl_file)
		base_filename, _ = os.path.splitext(file_name_only)
		cpp_global_var_name = GetCppVariableName(hlsl_function_name)

		command = [fxc_compiler_path,
			source_hlsl_file,            # From this HLSL file
			'/E', hlsl_function_name,    # Compile one function
			'/T', compiler_profile,      # As a vertex or pixel shader
			'/Vn', cpp_global_var_name,  # Into a C constant thus named
			'/Fh', output_file,   		 # Declared in this C header file.
			'/O3']                       # Fast is better than slow.
			
		(out, err) = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False).communicate()
		if err:
			print 'Error while compiling %s in file %s' % (hlsl_function_name, source_hlsl_file)
			print err
			sys.exit(1)
			
		with open(output_file, 'r') as header:
			header_output.append(header.read())

	# Now, re-write the .h and .cc files with the concatenation of all
	# the individual passes.
	classname = 'hlsl_%s' % (ConvertToCamelCase(base_filename))
	preamble = '\n'.join([
		'/' * 77,
		'// This file is auto-generated from %s' % file_name_only,
		'//',
		"// To edit it directly would be a fool's errand.",
		'/' * 77,
		'',
		''])

	with open(output_file, 'w') as cc:
		cc.write(preamble)
		cc.write('#pragma once\n\n')
		cc.write('#include <windows.h>\n\n')

		if namespace:
			cc.write('namespace %s {\n\n' % namespace)
			
		cc.write('namespace %s {\n\n' % classname)
		cc.write(''.join(header_output))
		cc.write('\n}  // namespace %s\n' % classname)

		if namespace:
			cc.write('\n}  // namespace %s\n' % namespace)

if __name__ == '__main__':
	parser = optparse.OptionParser()
	parser.add_option('--shader_compiler_tool', dest='compiler')
	parser.add_option('--output_file', dest='output_file')
	parser.add_option('--input_hlsl_file', dest='hlsl_file')
	(options, args) = parser.parse_args()

	hlsl_file = os.path.abspath(options.hlsl_file)
	shader_targets, namespace = ExtractShaderTargetNamesFromSource(hlsl_file)

	CompileMultipleHLSLShadersToOneHeaderFile(options.compiler, hlsl_file, namespace, shader_targets, options.output_file)