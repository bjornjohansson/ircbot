options = Options('custom.py')
options.AddOptions(('CPPPATH', 'Include path for preprocessor.', ''))
options.AddOptions(('CPPFLAGS', 'Flags for compiler.', ''))
options.AddOptions(('LINKFLAGS', 'Flags for linker.', ''))
options.AddOptions(('CPPDEFINES', 'Definitions for the preprocessor.', ''))

env = Environment(options=options)
env.SConscript(dirs=['src'], build_dir='build', duplicate=0, exports='env')

