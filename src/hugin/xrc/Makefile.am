# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

SUBDIRS=data

SUFFIXES=.xrs 

XRC_FILES = about.xrc anchor_orientation.xrc cp_editor_panel.xrc cp_list_frame.xrc edit_script_dialog.xrc edit_text.xrc help.xrc image_center.xrc \
            images_panel.xrc keyboard_help.xrc lens_dialog.xrc lens_panel.xrc main_frame.xrc main_menubar.xrc main_menu.xrc main_tool.xrc nona_panel.xrc \
            optimize_panel.xrc pano_panel.xrc preview_frame.xrc ptstitcher_panel.xrc run_optimizer_frame.xrc run_stitcher_frame.xrc 

XRS_FILES = about.xrs anchor_orientation.xrs cp_editor_panel.xrs cp_list_frame.xrs edit_script_dialog.xrs edit_text.xrs help.xrs image_center.xrs \
            images_panel.xrs keyboard_help.xrs lens_dialog.xrs lens_panel.xrs main_frame.xrs main_menubar.xrs main_menu.xrs main_tool.xrs nona_panel.xrs \
            optimize_panel.xrs pano_panel.xrs preview_frame.xrs ptstitcher_panel.xrs run_optimizer_frame.xrs run_stitcher_frame.xrs 

xrcdir = @datadir@/hugin/xrc

all-local: $(XRS_FILES)
	xrclist='$(XRC_FILES)'; \
	for p in $$xrclist; do \
	    if test -e `basename $$p`; then :; else \
		( $(LN_S) $$p ) ; \
	    fi; \
	done

dist_xrc_DATA = $(XRC_FILES)

.xrc.xrs: 
	-wxrc -g -o $@ $<
