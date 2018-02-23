{
	'target_defaults': {
		'conditions': [
			['OS == "win"', {
				'rules': [{
					'variables': {
						'inc_file': 'src/shaders/compiled/shaders.hpp',
						'output_file': 'src/shaders/compiled/<(RULE_INPUT_ROOT)_hlsl_compiled.hpp',
					},
					'rule_name': 'compile_hlsl',
					'extension': 'hlsl',
					'outputs': [
						'<(output_file)',
					],
					'action': [
						'python',
						'tools/compile_hlsl.py',
						'--fxc', 'C:/Program Files (x86)/Windows Kits/10/bin/x64/fxc.exe',
						'--input', '<(RULE_INPUT_PATH)',
						'--output', '<(output_file)',
						'--inc' ,'<(inc_file)',
					],
					'message': 'Generating shader from <(RULE_INPUT_PATH)',
					'process_outputs_as_sources': 0,
				}],
			}],
		],
	},
	'targets': [
	{
		'target_name': 'jc3-rbm-renderer',
		'type': 'executable',
		'dependencies': [
			'imgui.gyp:imgui',
			'zlib.gyp:zlib',
		],
		'defines': [
			'WIN32_LEAN_AND_MEAN',
			'CPPHTTPLIB_ZLIB_SUPPORT',
		],
		'msvs_configuration_attributes': {
			'CharacterSet': '0',
		},
		'msvs_settings': {
			'VCCLCompilerTool': {
				'AdditionalOptions': ['/std:c++latest'],
				'DisableSpecificWarnings': ['4244', '4267'],
			},
			'VCLinkerTool': {
				'SubSystem': '2',
			},
		},
		'variables': {
			'directx_sdk_path': '$(DXSDK_DIR)',
		},
		'libraries': [
			'Advapi32.lib',
			'Shell32.lib',
			'Gdi32.lib',
			'd3d11.lib',
			'dxgi.lib',
			'D3DCompiler.lib',
		],
		'include_dirs': [
			'src',
			'vendor/ksignals',
			'vendor/json/src',
			'vendor/glm/glm',
			'vendor/httplib/',
		],
		'sources': [
			'<!@pymod_do_main(glob-files src/**/*.cpp)',
			'<!@pymod_do_main(glob-files src/**/*.h)',
			'<!@pymod_do_main(glob-files src/shaders/**/*.hlsl)',
		],
	},
	]
}