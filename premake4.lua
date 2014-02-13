local GCC_AVR_CC_PATH = "/usr/bin/avr-gcc"
local GCC_AVR_CPP_PATH = "/usr/bin/avr-g++"
local GCC_AVR_AR_PATH = "/usr/bin/avr-ar"

table.insert(premake.option.list["platform"].allowed, { "avr" })

premake.platforms.avr = {
	cfgsuffix = "avr",
	iscrosscompiler = true
}

table.insert(premake.fields.platforms.allowed, "avr")


if(_OPTIONS.platform == 'avr') then
	premake.gcc.cc = GCC_AVR_CC_PATH
	premake.gcc.cxx = GCC_AVR_CPP_PATH 
	premake.gcc.ar = GCC_AVR_AR_PATH
end


-- stromplatine solution config
solution "stromplatine"
	configurations { "Debug", "Release" }
	platforms { "Native", "avr" }


	project "stromplatine"
		kind "ConsoleApp"
		language "c"

		targetname("stromplatine.elf")
		includedirs {"src/", "src/framework/include", "/usr/avr/include"}

		-- include/exclude Source Files
		local includeFiles = {
			"src/**.h",
			"src/**.c"
		}
		files(includeFiles)

		local excludeFiles = {
		}
		excludes(excludeFiles) 

		buildoptions {"-std=c99"}

		-- set symbols
		configuration "debug"
			defines { "DEBUG" }
			buildoptions{" -O0" , "-g3"}
			targetdir "build/debug"
			objdir "build/debug/obj"
			postbuildcommands { "avr-objcopy -O ihex build/debug/stromplatine.elf build/debug/stromplatine.hex", "avr-size build/debug/stromplatine.elf"}

		configuration "release"
			targetdir "build/release"
			buildoptions{" -Os"}
			objdir "build/release/obj"
			postbuildcommands { "avr-objcopy -O ihex build/release/stromplatine.elf build/release/stromplatine.hex", 
								"avr-size build/release/stromplatine.elf",
								"avr-objdump -h -S build/release/stromplatine.elf  > build/release/stromplatine.lss",
								}
		
		configuration {"avr", "gmake" }
			-- Run shell commands before build is started
			prebuildcommands { '@echo "\\n\\n--- Starting to build: `date` ---\\n\\n"' }

			-- Run shell commands after build is finished
			postbuildcommands { '@echo "\\n\\n--- Finished build ---\\n\\n"' }

			-- Sets the right mmcu
			buildoptions {"-Wall -fpack-struct -fshort-enums -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega88"}
			linkoptions {
						"-mmcu=atmega88",
						"-T src/target/atmega88/avr4.ld",
						"-Wl,-Map,stromplatine.map",
			 }

		defines { "F_CPU=16000000UL" }

