!define PRODUCT_NAME      "Mastering Compressor"
!define PRODUCT_VERSION   "1.0.0"
!define PRODUCT_PUBLISHER "Jtekkk"
!define VST3_DIR          "C:\Program Files\Common Files\VST3"
!define STANDALONE_DIR    "C:\Program Files\Jtekkk\Mastering Compressor"
!define UNINST_KEY        "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

Name              "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile           "Mastering_Compressor_Setup.exe"
InstallDir        "${STANDALONE_DIR}"
RequestExecutionLevel admin
SetCompressor     /SOLID lzma
Unicode           True

;---------------------------------------------------------------------------
; Modern UI
;---------------------------------------------------------------------------
!include "MUI2.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON         "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON       "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_HEADERIMAGE
!define MUI_BGCOLOR      "1A1A2E"
!define MUI_TEXTCOLOR    "FFFFFF"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;---------------------------------------------------------------------------
; Sections
;---------------------------------------------------------------------------
Section "VST3 Plugin" SEC_VST3
    SectionIn RO          ; required

    SetOutPath "${VST3_DIR}\${PRODUCT_NAME}.vst3\Contents\x86_64-win"
    File "build-win\MasteringCompressor_artefacts\Release\VST3\Mastering Compressor.vst3\Contents\x86_64-win\Mastering Compressor.vst3"
    File "build-win\MasteringCompressor_artefacts\Release\VST3\Mastering Compressor.vst3\Contents\x86_64-win\libgcc_s_seh-1.dll"
    File "build-win\MasteringCompressor_artefacts\Release\VST3\Mastering Compressor.vst3\Contents\x86_64-win\libstdc++-6.dll"
    File "build-win\MasteringCompressor_artefacts\Release\VST3\Mastering Compressor.vst3\Contents\x86_64-win\libwinpthread-1.dll"

    SetOutPath "${VST3_DIR}\${PRODUCT_NAME}.vst3\Contents\Resources"
    File "build-win\MasteringCompressor_artefacts\Release\VST3\Mastering Compressor.vst3\Contents\Resources\moduleinfo.json"

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayName"     "${PRODUCT_NAME}"
    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
    WriteRegStr   HKLM "${UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
    WriteRegStr   HKLM "${UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify"        1
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair"        1
SectionEnd

Section "Standalone Application" SEC_STANDALONE
    SetOutPath "$INSTDIR"
    File "build-win\MasteringCompressor_artefacts\Release\Standalone\Mastering Compressor.exe"
    File "build-win\MasteringCompressor_artefacts\Release\Standalone\libgcc_s_seh-1.dll"
    File "build-win\MasteringCompressor_artefacts\Release\Standalone\libstdc++-6.dll"
    File "build-win\MasteringCompressor_artefacts\Release\Standalone\libwinpthread-1.dll"

    CreateDirectory "$SMPROGRAMS\${PRODUCT_PUBLISHER}"
    CreateShortcut  "$SMPROGRAMS\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}.lnk" \
                    "$INSTDIR\Mastering Compressor.exe"
    CreateShortcut  "$DESKTOP\${PRODUCT_NAME}.lnk" \
                    "$INSTDIR\Mastering Compressor.exe"
SectionEnd

;---------------------------------------------------------------------------
; Section descriptions
;---------------------------------------------------------------------------
LangString DESC_VST3       ${LANG_ENGLISH} "VST3 plugin for use in your DAW (Ableton, Reaper, FL Studio, etc.)"
LangString DESC_STANDALONE ${LANG_ENGLISH} "Standalone application with built-in audio I/O"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_VST3}       $(DESC_VST3)
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STANDALONE} $(DESC_STANDALONE)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;---------------------------------------------------------------------------
; Uninstaller
;---------------------------------------------------------------------------
Section "Uninstall"
    RMDir /r "${VST3_DIR}\${PRODUCT_NAME}.vst3"
    RMDir /r "$INSTDIR"
    Delete   "$SMPROGRAMS\${PRODUCT_PUBLISHER}\${PRODUCT_NAME}.lnk"
    RMDir    "$SMPROGRAMS\${PRODUCT_PUBLISHER}"
    Delete   "$DESKTOP\${PRODUCT_NAME}.lnk"
    DeleteRegKey HKLM "${UNINST_KEY}"
SectionEnd
