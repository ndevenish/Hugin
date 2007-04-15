# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

SUBDIRS=data

SUFFIXES=.xrs 

if HAVE_MINGW 
xrcdir = $(bindir)/xrc
else
xrcdir = $(datadir)/hugin/xrc
endif

XRC_FILES = about.xrc anchor_orientation.xrc cp_editor_panel.xrc cp_list_frame.xrc crop_panel.xrc \
            edit_script_dialog.xrc edit_text.xrc help.xrc \
            images_panel.xrc keyboard_help.xrc lens_panel.xrc main_frame.xrc main_menubar.xrc main_menu.xrc main_tool.xrc nona_panel.xrc \
            optimize_panel.xrc pano_panel.xrc pref_dialog.xrc preview_frame.xrc ptstitcher_panel.xrc run_optimizer_frame.xrc run_stitcher_frame.xrc \
            cp_editor_panel-2.5.xrc images_panel-2.5.xrc lens_panel-2.5.xrc main_frame-2.5.xrc optimize_panel-2.5.xrc pano_panel-2.5.xrc vig_corr_dlg.xrc assistant_panel.xrc \
            optimize_photo_panel.xrc

XRS_FILES = about.xrs anchor_orientation.xrs cp_editor_panel.xrs cp_list_frame.xrs crop_panel.xrs \
            edit_script_dialog.xrs edit_text.xrs help.xrs \
            images_panel.xrs keyboard_help.xrs lens_panel.xrs main_frame.xrs main_menubar.xrs main_menu.xrs main_tool.xrs nona_panel.xrs \
            optimize_panel.xrs pano_panel.xrs pref_dialog.xrs preview_frame.xrs ptstitcher_panel.xrs run_optimizer_frame.xrs run_stitcher_frame.xrs \
            cp_editor_panel-2.5.xrs images_panel-2.5.xrs lens_panel-2.5.xrs main_frame-2.5.xrs optimize_panel-2.5.xrs pano_panel-2.5.xrs vig_corr_dlg.xrs assistant_panel.xrs \
            optimize_photo_panel.xrs

all-local: stamp-xrc stamp-xrs

stamp-xrc: $(XRC_FILES)
	for p in $?; do \
	    if test -e `basename $$p` ; then :; else \
		( $(LN_S) $$p . ) ; \
	    fi; \
	done
	@echo "touch stamp-xrc"
	@echo timestamp > stamp-xrcT
	@mv stamp-xrcT stamp-xrc

stamp-xrs: $(XRS_FILES)
	@echo "touch stamp-xrs"
	@echo timestamp > stamp-xrsT
	@mv stamp-xrsT stamp-xrs
			

dist_xrc_DATA = $(XRC_FILES)

.xrc.xrs: 
	-wxrc -g -o $(srcdir)/$@ $<
