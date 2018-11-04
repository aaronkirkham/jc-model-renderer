{
	'targets': [
	{
		'target_name': 'jc3-rbm-renderer',
		'type': 'executable',
		'dependencies': [
			'imgui.gyp:imgui',
			'zlib.gyp:zlib',
		],
		'configurations': {
			'Debug': {
				'defines': [
					'_ITERATOR_DEBUG_LEVEL=0',
				],
			},
		},
		'defines': [
			'WIN32_LEAN_AND_MEAN',
			'CPPHTTPLIB_ZLIB_SUPPORT',
			'_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING',
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
			'Ole32.lib',
			'Comdlg32.lib',
		],
		'include_dirs': [
			'src',
			'vendor/ksignals',
			'vendor/json/single_include/nlohmann',
			'vendor/glm/glm',
			'vendor/httplib',
		],
		'sources': [
			'src/assets.rc',
			'<!@pymod_do_main(glob-files src/**/*.cpp)',
			'<!@pymod_do_main(glob-files src/**/*.h)',
		],
	},
	]
}