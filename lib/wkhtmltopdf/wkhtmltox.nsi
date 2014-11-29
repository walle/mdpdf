!include "MUI2.nsh"
!include "x64.nsh"

Name             "wkhtmltox ${VERSION}"
OutFile          "static-build\wkhtmltox-${VERSION}_${TARGET}.exe"
InstallDir       "$PROGRAMFILES64\wkhtmltopdf"
VIProductVersion "${SIMPLE_VERSION}"
VIAddVersionKey  "ProductName"     "wkhtmltox"
VIAddVersionKey  "FileDescription" "wkhtmltox ${VERSION}"
VIAddVersionKey  "LegalCopyright"  "wkhtmltopdf authors"
VIAddVersionKey  "FileVersion"     "${VERSION}"

CRCCheck             force
SetCompressor /SOLID lzma
SetCompressorDictSize 64
RequestExecutionLevel admin

!insertmacro MUI_PAGE_LICENSE "COPYING"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

!macro DeleteFiles un
Function ${un}DeleteFiles
; remove as per old installer layout
  Delete "$INSTDIR\libgcc_s_dw2-1.dll"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "$INSTDIR\ssleay32.dll"
  Delete "$INSTDIR\libeay32.dll"
  Delete "$INSTDIR\wkhtmltopdf.exe"
  Delete "$INSTDIR\wkhtmltoimage.exe"
; remove as per current installer layout
  Delete "$INSTDIR\bin\wkhtmltoimage.exe"
  Delete "$INSTDIR\bin\wkhtmltopdf.exe"
  Delete "$INSTDIR\bin\wkhtmltox.dll"
  Delete "$INSTDIR\lib\wkhtmltox.lib"
  Delete "$INSTDIR\include\wkhtmltox\dllbegin.inc"
  Delete "$INSTDIR\include\wkhtmltox\dllend.inc"
  Delete "$INSTDIR\include\wkhtmltox\pdf.h"
  Delete "$INSTDIR\include\wkhtmltox\image.h"
  Delete "$INSTDIR\uninstall.exe"
  RMDir  "$INSTDIR\bin"
  RMDir  "$INSTDIR\include\wkhtmltox"
  RMDir  "$INSTDIR\include"
  RMDir  "$INSTDIR\lib"
  RMDir  "$INSTDIR"
FunctionEnd
!macroend

!insertmacro DeleteFiles ""
!insertmacro DeleteFiles "un."

Section "Install"
  Call DeleteFiles

  SetOutPath "$INSTDIR"
  SetOutPath "$INSTDIR\bin"
  File static-build\${TARGET}\app\bin\wkhtmltoimage.exe
  File static-build\${TARGET}\app\bin\wkhtmltopdf.exe
  File static-build\${TARGET}\app\bin\wkhtmltox.dll

  SetOutPath "$INSTDIR\lib"
  File static-build\${TARGET}\app\bin\wkhtmltox.lib

  SetOutPath "$INSTDIR\include\wkhtmltox"
  File include\wkhtmltox\dllbegin.inc
  File include\wkhtmltox\dllend.inc
  File include\wkhtmltox\pdf.h
  File include\wkhtmltox\image.h

  WriteRegStr HKLM "Software\wkhtmltopdf" "InstallPath" "$INSTDIR"
  WriteRegStr HKLM "Software\wkhtmltopdf" "Version"     "${VERSION}"
  WriteRegStr HKLM "Software\wkhtmltopdf" "DllPath"     "$INSTDIR\bin\wkhtmltox.dll"
  WriteRegStr HKLM "Software\wkhtmltopdf" "PdfPath"     "$INSTDIR\bin\wkhtmltopdf.exe"
  WriteRegStr HKLM "Software\wkhtmltopdf" "ImagePath"   "$INSTDIR\bin\wkhtmltoimage.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\wkhtmltopdf" \
                   "DisplayName" "wkhtmltox ${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\wkhtmltopdf" \
                   "UninstallString" "$\"$INSTDIR\uninstall.exe$\""

  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
  Call un.DeleteFiles
  DeleteRegKey HKLM "Software\wkhtmltopdf"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\wkhtmltopdf"
SectionEnd

Function .onInit
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
FunctionEnd
