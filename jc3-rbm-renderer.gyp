{
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
			'Gdi32.lib',
			'd3d11.lib',
			'dxgi.lib',
			'D3DCompiler.lib',
		],
		'include_dirs': ['src', 'vendor/ksignals', 'vendor/json/src', 'vendor/glm/glm'],
		'sources': [
			'<!@pymod_do_main(glob-files src/**/*.cpp)',
			'<!@pymod_do_main(glob-files src/**/*.h)',
			'<!@pymod_do_main(glob-files assets/shaders/*.hlsl)',
		],
		'actions': [
		{
			'action_name': 'build_assets',
			'inputs': ['tools/build_assets.py'],
			'outputs': ['test'],
			'action': ['python', '<@(_inputs)', '>', '<@(_outputs)'],
		},
		]
	},
	]
}