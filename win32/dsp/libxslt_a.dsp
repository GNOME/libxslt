# Microsoft Developer Studio Project File - Name="libxslt_a" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libxslt_a - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libxslt_a.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libxslt_a.mak" CFG="libxslt_a - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libxslt_a - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libxslt_a - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libxslt_a - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libxslt_a"
# PROP Intermediate_Dir "libxslt_a"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /I "../.." /I "../../libxslt" /I "../../../gnome-xml/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "IN_LIBXSLT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"libxslt_a\libxslt.lib"

!ELSEIF  "$(CFG)" == "libxslt_a - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libxslt_a"
# PROP Intermediate_Dir "libxslt_a"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /Zi /Od /I "../.." /I "../../libxslt" /I "../../../gnome-xml/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "IN_LIBXSLT" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"libxslt_a\libxslt.lib"

!ENDIF 

# Begin Target

# Name "libxslt_a - Win32 Release"
# Name "libxslt_a - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\libxslt\attributes.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\documents.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\extensions.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\extra.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\functions.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\imports.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\keys.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\namespaces.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\numbers.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\pattern.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\preproc.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\templates.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\transform.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\variables.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\xslt.c
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\xsltutils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\libxslt\attributes.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\documents.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\extensions.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\extra.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\functions.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\imports.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\keys.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\libxslt.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\namespaces.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\numbersInternals.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\pattern.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\preproc.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\templates.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\transform.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\variables.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\win32config.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\xslt.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\xsltInternals.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\xsltutils.h
# End Source File
# Begin Source File

SOURCE=..\..\libxslt\xsltwin32config.h
# End Source File
# End Group
# End Target
# End Project
