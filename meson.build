project('qcolor', 'cpp') #, default_options : ['cpp_std=c++2a'])
add_project_arguments(['-std=c++17'], language : 'cpp')             # because default not taken into account
#add_project_link_arguments(['-lstdc++fs'], language : 'cpp')

# infer sources from src content
# if new files added => meson --reconfigure
sources = run_command('sh', '-c', 'ls -1 src/*.cc').stdout().strip().split('\n')
message('registered sources @0@'.format(sources))

vips   = dependency('vips-cpp ', version:'>=8.12.1')

# meson wrap install nlohmann_json
json = dependency('nlohmann_json', required: true)

# my own
incdir = include_directories('/home/yves/DEV/cpp/')

executable('qcolor', sources, dependencies: [ vips, json], include_directories : incdir)
