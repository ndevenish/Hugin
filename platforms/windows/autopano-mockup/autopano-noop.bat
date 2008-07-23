@ECHO OFF
REM Note for packagers: to configure hugin to use this notice instead
REM of autopano-complete.sh or autopano-c-complete.sh, simply perform
REM this substitution before compilation:
REM 
REM sed -i 's/"autopano-sift-c"/"autopano-noop.bat"/' \
REM     src/hugin1/hugin/config_defaults.h
REM 
REM ..and place this script somewhere in the $PATH.
ECHO READ THIS -- READ THIS -- READ THIS -- READ THIS -- READ THIS -- READ THIS
ECHO.
ECHO  If you see this message then your version of hugin has been
ECHO  configured without support for automatic generation of control
ECHO  points.
ECHO.
ECHO  This is probably because the SIFT algorithm used by autopano-sift
ECHO  and autopano-sift-C is encumbered by software patents in the
ECHO  United States of America.
ECHO.
ECHO  If this is in error and you do have access to one of these tools,
ECHO  then you can reconfigure hugin in the Preferences menu.
ECHO.
ECHO  Otherwise don't panic. Hugin is still very usable with control
ECHO  points set manually in the Control Points tab, see the tutorials
ECHO  on hugin.sourceforge.net for more details.
ECHO.
PAUSE
