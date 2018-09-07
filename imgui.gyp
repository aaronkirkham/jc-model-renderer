{
	'targets': [
	{
		'target_name': 'imgui',
		'type': 'static_library',
		'defines': [
			'IMGUI_DISABLE_OBSOLETE_FUNCTIONS',
		],
		'include_dirs': ['vendor/imgui', 'vendor/imgui/examples'],
		'sources': [
			'vendor/imgui/imconfig.h',
			'vendor/imgui/imgui.cpp',
			'vendor/imgui/imgui.h',
			'vendor/imgui/imgui_draw.cpp',
			'vendor/imgui/imgui_internal.h',
			'vendor/imgui/imgui_tabs.cpp',
			'vendor/imgui/imgui_tabs.h',
			'vendor/imgui/imgui_widgets.cpp',
			'vendor/imgui/stb_rect_pack.h',
			'vendor/imgui/stb_textedit.h',
			'vendor/imgui/stb_truetype.h',
			'vendor/imgui/examples/imgui_impl_win32.cpp',
			'vendor/imgui/examples/imgui_impl_win32.h',
			'vendor/imgui/examples/imgui_impl_dx11.cpp',
			'vendor/imgui/examples/imgui_impl_dx11.h',
		],
		'direct_dependent_settings': {
			'include_dirs': ['vendor/imgui', 'vendor/imgui/examples'],
		},
	}
	]
}