IF (WIN32)
  INSTALL(FILES collect_data_enblend.bat collect_data_enfuse.bat enblend_droplet.bat
                enblend_droplet_360.bat enfuse_align_droplet.bat
                enfuse_auto_droplet.bat  enfuse_droplet.bat
                enfuse_droplet_360.bat exiftool_enblend_args.txt exiftool_enfuse_args.txt
                unique_filename.bat
          DESTINATION bin)
  INSTALL(FILES readme.txt
          DESTINATION doc/enblend)
ENDIF (WIN32)

