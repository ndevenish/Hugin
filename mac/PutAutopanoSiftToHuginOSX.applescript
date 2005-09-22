display alert "This feature is currently not supported." message Â
	"Using autopano-sift files inside HuginOSX application package is not stable and hence not recommended nor supported. If it did not work for you and would like to remove those files from the package, please use 'Show Package Contents' in Finder and delete autopano-sift folder in the Resources folder." as informational buttons {"Cancel", "OK"} cancel button 1 default button 2

display alert "Autopano-SIFT uses patented algorithm" message Â
	"Please read its readme and license files carefully before use." as warning

tell application "Finder"
	copy "" to autopanoDirPath
	repeat while autopanoDirPath is equal to ""
		copy (choose folder with prompt "Where is autopano-sift bin folder?" without invisibles) to autopanoDirPath
		if (not (exists file "autopano.exe" of folder autopanoDirPath)) Â
			or (not (exists file "generatekeys-sd.exe" of folder autopanoDirPath)) Â
			or (not (exists file "libsift.dll" of folder autopanoDirPath)) then
			copy "" to autopanoDirPath
			display alert "Error: enblend file name do not match" as warning
		end if
	end repeat
	copy "" to huginPath
	repeat while huginPath is equal to ""
		copy (choose file of type {"APPL"} with prompt "Where is HuginOSX.app?" without invisibles) to huginPath
		if the name of file huginPath is not equal to "HuginOSX.app" then
			copy "" to huginPath
			display alert "Error: HuginOSX.app file name do not match" as warning
		end if
	end repeat
	if exists folder "autopano-sift" of folder "Contents:Resources:" of file huginPath then delete folder "autopano-sift" of folder "Contents:Resources:" of file huginPath
	
	copy (make new folder with properties {name:"autopano-sift"} at folder "Contents:Resources:" of file huginPath) to autopanoSiftFolder
	duplicate file "autopano.exe" of folder autopanoDirPath to autopanoSiftFolder
	duplicate file "generatekeys-sd.exe" of folder autopanoDirPath to autopanoSiftFolder
	duplicate file "libsift.dll" of folder autopanoDirPath to autopanoSiftFolder
end tell