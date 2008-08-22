to open the droppedFiles
	repeat with eachFile in droppedFiles
		installAutoCP(eachFile)
	end repeat
end open

installAutoCP(choose file with prompt "Choose AutoCP plugin for Hugin" of type {"BNDL", "????"} without invisibles)

to installAutoCP(theFile)
	tell application "Finder" to set theExtension to the name extension of theFile
	if theExtension is not equal to "huginAutoCP" then
		display alert "Wrong file type" message Â
			"The file extension ." & theExtension & " indicates this file is not an AutoCP plugin for Hugin." buttons {"Quit"} as critical
		quit
	end if
	
	set theDestination to (the path to application support from user domain as text) & "Hugin:Autopano:"
	tell application "Finder"
		if not (exists theDestination) then do shell script "mkdir -p \"" & (POSIX path of theDestination) & "\""
		if (exists (theDestination & the name of theFile)) then delete (theDestination & the name of theFile)
		duplicate theFile to theDestination
	end tell
end installAutoCP