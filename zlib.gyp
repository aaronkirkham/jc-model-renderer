{
	'targets': [
	{
		'target_name': 'zlib',
		'type': 'static_library',
		'defines': [
			'DEF_WBITS=-15',
		],
		'msvs_settings': {
			'VCCLCompilerTool': {
				'DisableSpecificWarnings': ['4267'],
			},
		},
		'include_dirs': ['vendor/zlib'],
		'sources': [
			'<!@pymod_do_main(glob-files vendor/zlib/*.c)',
			'<!@pymod_do_main(glob-files vendor/zlib/*.h)',
		],
		'direct_dependent_settings': {
			'include_dirs': ['vendor/zlib'],
		},
	}
	]
}