tell application "Finder"
	copy "" to enblendPath
	repeat while enblendPath is equal to ""
		copy (choose file with prompt "Where is enblend?" without invisibles) to enblendPath
		if the name of file enblendPath is not equal to "enblend" then
			copy "" to enblendPath
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
	duplicate file enblendPath to (folder "Contents:MacOS:" of file huginPath) with replacing
end tell