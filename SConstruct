vars = Variables('custom.py')
vars.AddVariables(('CPPPATH', 'Include path for preprocessor.', ''))
vars.AddVariables(('CPPFLAGS', 'Flags for compiler.', ''))
vars.AddVariables(('LINKFLAGS', 'Flags for linker.', ''))
vars.AddVariables(('CPPDEFINES', 'Definitions for the preprocessor.', ''))
vars.AddVariables(('LIBPATH', 'Include path for linker.', ''))

env = Environment(variables = vars)

debug = ARGUMENTS.get('debug', 0)
release = ARGUMENTS.get('release', 0)

build_dir = 'build'
if int(debug):
   env.Append(CCFLAGS = ['-g'])
   env.Append(CPPDEFINES = ['DEBUG'])
   build_dir = 'build/debug'
elif int(release):
   env.Append(CCFLAGS = ['-pipe', '-O3', '-fomit-frame-pointer', '-march=native'])
   build_dir = 'build/release'
else:
   print "Specify a build, release=1 or debug=1"
   Exit(1)

env.SConscript(dirs=['src'], build_dir=build_dir, duplicate=0, exports='env')

