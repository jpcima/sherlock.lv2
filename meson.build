project('sherlock.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=1',
	'werror=false',
	'b_lto=true',
	'c_std=c11'])

add_project_arguments('-D_GNU_SOURCE', language : 'c')

conf_data = configuration_data()
cc = meson.get_compiler('c')

m_dep = cc.find_library('m')
lv2_dep = dependency('lv2', version : '>=1.14.0')
gl_dep = dependency('gl')
sratom_dep = dependency('sratom-0', version : '>=0.6.0')

pugl_inc = include_directories('pugl')
props_inc = include_directories('props.lv2')
osc_inc = include_directories('osc.lv2')
ser_inc = include_directories('ser_atom.lv2')
inc_dir = [pugl_inc, props_inc, osc_inc, ser_inc]

inst_dir = join_paths(get_option('libdir'), 'lv2', meson.project_name())

rawvers = run_command('cat', 'VERSION').stdout().strip()
version = rawvers.split('.')
conf_data.set('MAJOR_VERSION', version[0])
conf_data.set('MINOR_VERSION', version[1])
conf_data.set('MICRO_VERSION', version[2])

add_project_arguments('-DSHERLOCK_VERSION="'+rawvers+'"', language : 'c')

mod = shared_module('sherlock',
	'sherlock.c',
	'atom_inspector.c',
	'midi_inspector.c',
	'osc_inspector.c',
	c_args : ['-fvisibility=hidden', '-ffast-math'],
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : [m_dep, lv2_dep],
	install : true,
	install_dir : inst_dir)

ui = shared_module('sherlock_ui',
	'sherlock_nk.c',
	'atom_inspector_nk.c',
	'midi_inspector_nk.c',
	'osc_inspector_nk.c',
	c_args : ['-fvisibility=hidden', '-ffast-math'],
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : [m_dep, lv2_dep, gl_dep, sratom_dep],
	install : true,
	install_dir : inst_dir)

suffix = mod.full_path().strip().split('.')[-1]
conf_data.set('MODULE_SUFFIX', '.' + suffix)

if host_machine.system() == 'linux'
	conf_data.set('UI_TYPE', 'X11UI')
elif host_machine.system() == 'windows'
	conf_data.set('UI_TYPE', 'WindowsUI')
elif host_machine.system() == 'windows'
	conf_data.set('UI_TYPE', 'CocoaUI')
endif

configure_file(input : 'manifest.ttl.in', output : 'manifest.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)
configure_file(input : 'sherlock.ttl.in', output : 'sherlock.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)
configure_file(input : 'sherlock_ui.ttl.in', output : 'sherlock_ui.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)
