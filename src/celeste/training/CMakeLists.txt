#    Copyright (C) 2008 by Tim Nugent
#    timnugent@gmail.com
#    License: GPLv2+

ADD_EXECUTABLE( celeste_train CelesteTrain.cpp )
target_link_libraries( celeste_train celeste ${image_libs} ${wxWidgets_LIBRARIES})
