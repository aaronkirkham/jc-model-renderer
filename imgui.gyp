{
	'targets': [
	{
		'target_name': 'imgui',
		'type': 'static_library',
		'include_dirs': ['vendor/imgui', 'vendor/imgui/examples/directx11_example'],
		'sources': [
            'vendor/imgui/imconfig.h',
            'vendor/imgui/imgui.h',
			'vendor/imgui/imgui.cpp',
            'vendor/imgui/imgui_draw.cpp',
            'vendor/imgui/imgui_internal.h',
            'vendor/imgui/stb_rect_pack.h',
            'vendor/imgui/stb_textedit.h',
            'vendor/imgui/stb_truetype.h',
            'vendor/imgui/examples/directx11_example/imgui_impl_dx11.cpp',
            'vendor/imgui/examples/directx11_example/imgui_impl_dx11.h',
		],
        'direct_dependent_settings': {
            'include_dirs': ['vendor/imgui', 'vendor/imgui/examples/directx11_example'],
         },
	}
	]
}