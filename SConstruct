vars = Variables('custom.py')
vars.AddVariables(('CPPPATH', 'Include path for preprocessor.', ''))
vars.AddVariables(('CPPFLAGS', 'Flags for compiler.', ''))
vars.AddVariables(('LINKFLAGS', 'Flags for linker.', ''))
vars.AddVariables(('CPPDEFINES', 'Definitions for the preprocessor.', ''))

env = Environment(variables = vars)

debug = ARGUMENTS.get('debug', 0)
release = ARGUMENTS.get('release', 0)

if int(debug):
   env.Append(CCFLAGS = ['-g'])
   env.Append(CPPDEFINES = ['DEBUG'])
elif int(release):
   env.Append(CCFLAGS = ['-O2'])
else:
   print "Specify a build, release=1 or debug=1"
   Exit(1)

env.SConscript(dirs=['src'], build_dir='build', duplicate=0, exports='env')

