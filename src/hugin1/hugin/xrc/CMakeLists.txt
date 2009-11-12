SET(XRCFILES
    about.xrc 
    crop_panel.xrc
    cp_list_frame.xrc
    preview_frame.xrc
    edit_script_dialog.xrc
    main_menu.xrc
    main_tool.xrc
    pref_dialog.xrc
    cpdetector_dialog.xrc
    reset_dialog.xrc
    vig_corr_dlg.xrc
    optimize_photo_panel.xrc
    cp_editor_panel.xrc
    images_panel.xrc
    lens_panel.xrc
    assistant_panel.xrc
    main_frame.xrc
    optimize_panel.xrc
    pano_panel.xrc
    batch_frame.xrc
    batch_menu.xrc
    batch_toolbar.xrc)


INSTALL(FILES ${XRCFILES} DESTINATION ${HUGINDATADIR}/xrc)

add_subdirectory(data)
