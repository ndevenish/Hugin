display alert "Do you know what you are doing?" message Â
	"Please read the readme and license files carefully before use. You'd better know what you are doing before you proceed." buttons {"Quit", "OK"} cancel button 1 as warning

tell application "Finder"
	copy "" to autopanoDirPath
	repeat while autopanoDirPath is equal to ""
		copy (choose folder with prompt "Where is autopano-sift bin folder?" without invisibles) to autopanoDirPath
		if (not (exists file "autopano.exe" of folder autopanoDirPath)) Â
			or (not (exists file "generatekeys-sd.exe" of folder autopanoDirPath)) Â
			or (not (exists file "libsift.dll" of folder autopanoDirPath)) then
			copy "" to autopanoDirPath
			display alert "Error: autopano-sift file names do not match" as warning
		end if
	end repeat
	copy "" to huginPath
	repeat while huginPath is equal to ""
		copy (choose file of type {"APPL"} with prompt "Where is Hugin.app?" without invisibles) to huginPath
		if the name of file huginPath is not equal to "Hugin.app" then
			copy "" to huginPath
			display alert "Error: Hugin.app file name do not match" as warning
		end if
	end repeat
	if exists folder "autopano-sift" of folder "Contents:Resources:" of file huginPath then delete folder "autopano-sift" of folder "Contents:Resources:" of file huginPath
	
	copy (make new folder with properties {name:"autopano-sift"} at folder "Contents:Resources:" of file huginPath) to autopanoSiftFolder
	duplicate file "autopano.exe" of folder autopanoDirPath to autopanoSiftFolder
	duplicate file "generatekeys-sd.exe" of folder autopanoDirPath to autopanoSiftFolder
	duplicate file "libsift.dll" of folder autopanoDirPath to autopanoSiftFolder
end tell