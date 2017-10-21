{
	'target_defaults': {
		'conditions': [
			['OS == "win"', {
				'include_dirs': [
					'src/shaders/compiled',
				],
				'rules': [{
					'variables': {
						'output_file': 'src/shaders/compiled/<(RULE_INPUT_ROOT)_hlsl_compiled.h',
					},
					'rule_name': 'compile_hlsl',
					'extension': 'hlsl',
					'outputs': [
						'<(output_file)',
					],
					'action': [
						'python',
						'tools/compile_hlsl.py',
						'--shader_compiler_tool', 'C:/Program Files (x86)/Windows Kits/10/bin/x64/fxc.exe',
						'--output_file', '<(output_file)',
						'--input_hlsl_file', '<(RULE_INPUT_PATH)',
					],
					'message': 'Generating shaders from <(RULE_INPUT_PATH)...',
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
			'WIN32_LEAN_AND_MEAN'
		],
		'msvs_configuration_attributes': {
			'CharacterSet': '0',
		},
		'msvs_settings': {
			'VCCLCompilerTool': {
				'AdditionalOptions': ['/std:c++latest'],
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
			'vendor/glm/glm'
		],
		'sources': [
			'<!@pymod_do_main(glob-files src/**/*.cpp)',
			'<!@pymod_do_main(glob-files src/**/*.h)',
			'src/shaders/DebugRenderer.hlsl',
			'src/shaders/CarPaintMM.hlsl',
			'src/shaders/Character.hlsl',
			'src/shaders/GeneralJC3.hlsl',
		],
	},
	]
}