; Installer for Hugin Release
; defines will be generated using CMake directives
  
  ; Arch type (todo: with CMAKE)
  !define ARCH_TYPE "32"
  !define HUGIN_VERSION "@HUGIN_PACKAGE_VERSION@"
  !define HUGIN_VERSION_BUILD "hg@HUGIN_WC_REVISION@"  

  ;Default installation folder (todo: with CMAKE)
  InstallDir "$PROGRAMFILES\Hugin"
  ; or
  ; InstallDir "$PROGRAMFILES64\Hugin"

; ----------------------------------------
; Now the common part
!include HuginSetup_common.nsh