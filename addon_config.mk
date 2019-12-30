meta:
	ADDON_NAME 			= ofxStructureCore
	ADDON_DESCRIPTION 	= Addon for Occipital Structure Core Sensor / Structure SDK
	ADDON_AUTHOR 		= Tyler Henry
	ADDON_TAGS 			= "device","depth","camera"
	ADDON_URL 			= http://github.com/tyhenry/ofxStructureCore

common:

	ADDON_INCLUDES  = src/
	ADDON_INCLUDES 	+= libs/Structure/include/

osx:
vs:
	ADDON_LIBS 		+= libs/Structure/lib/vs/x64/Structure.lib
	DLLS_TO_COPY 	+= libs/Structure/libs/vs/x64/Structure.dll

linux64:
linuxarmv6l:
linuxarmv7l:
msys2:
