cpp = meson.get_compiler('cpp')

#TODO: cmake dependency doesn't work 
#casc_dep = dependency('CascLib', required : true)

casc_dep = cpp.find_library('casc', required : true)

executable('cascer',
		'cascer.cpp',
		dependencies : [casc_dep],
		install : true)