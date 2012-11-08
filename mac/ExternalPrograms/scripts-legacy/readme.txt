This "scripts-legacy" folder contains the scripts that were used to create 32-bit/64-bit universal
intel builds.

Previously they could also be used to build Hugin on PPC and that's still possible with these scripts
up to Leopard. Snow Leopard and newer no longer support PPC.
Hugin itself was always 32bit due to the fact that wxWindows a.k.a. wxWidgets a.k.a. wxMac was 
carbon based meaning that it wasn't possible to build a 64bit Hugin Gui.

As PPC and Tiger are no longer supported, and wxWidgets > 2.9.0 can be built against 64-bit Cocoa, we 
switch to a fully 64-bit Hugin. The scripts for that build can be found in the "scripts" folder. 
