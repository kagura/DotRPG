require 'yaml'
require 'fileutils'

recipe = YAML.load_file("Makefile.recipe")
SRC = recipe["SRC"]
CFLAGS = recipe["CFLAGS"]
CINCLUDES = recipe["CINCLUDES"]
LFLAGS = recipe["LFLAGS"]
OBJDIR = recipe["OBJDIR"]

def getIncludePath(filename)
	searchPath = CINCLUDES.find{|includeSearchPath|
		File.exist?(File.join(includeSearchPath, filename))
	}
	if searchPath
		File.join(searchPath, filename)
	else
		#puts filename + " not found."
		return nil
	end
end

def getIncludeFiles(filename)
	includeNames = []
	IO.read(filename).scan(/#include \"(.+)\"/){|s|
		includeName = s[0]
		includePath = getIncludePath(includeName)
		if includePath
			includeNames << includePath
			includeNames.concat(getIncludeFiles(includePath))
		end
	}
	return includeNames
end

FileUtils.mkdir_p(OBJDIR)

command = "setlocal\n"
command += "call \"C:\\Program Files\\Microsoft Visual Studio 9.0\\VC\\bin\\vcvars32.bat\"\n"
SRC.each{|src|
	obj = File.join(OBJDIR, File.basename(src, ".*") + ".obj")
	includeNames = getIncludeFiles(src)
	if ARGV[0] == "-a" or 
		!File.exist?(obj) or
		File.mtime(src) > File.mtime(obj) or
		includeNames.any?{|includeName| File.mtime(includeName) > File.mtime(obj) }
		command += CFLAGS + " " + CINCLUDES.map{|searchPath| "/I" + searchPath }.join(" ") + " /Fo" + obj + " " + src + "\n"
	end
}
objListStr = SRC.map{|src| File.join(OBJDIR, File.basename(src, ".*") + ".obj") }.join(" ")
linkCommand = "link " + objListStr + " " + LFLAGS
command += linkCommand + "\n"
command += "endlocal\n"
puts command