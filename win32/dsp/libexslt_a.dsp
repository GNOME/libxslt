# Microsoft Developer Studio Project File - Name="libexslt_a" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libexslt_a - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libexslt_a.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libexslt_a.mak" CFG="libexslt_a - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libexslt_a - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libexslt_a - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libexslt_a - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libexslt_a"
# PROP Intermediate_Dir "libexslt_a"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "../.." /I "../../../gnome-xml/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "IN_LIBEXSLT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"libexslt_a\libexslt.lib"

!ELSEIF  "$(CFG)" == "libexslt_a - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libexslt_a"
# PROP Intermediate_Dir "libexslt_a"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /Zi /Od /I "../.." /I "../../../gnome-xml/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "IN_LIBEXSLT" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"libexslt_a\libexslt.lib"

!ENDIF 

# Begin Target

# Name "libexslt_a - Win32 Release"
# Name "libexslt_a - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\libexslt\common.c
# End Source File
# Begin Source File

SOURCE=..\..\libexslt\exslt.c
# End Source File
# Begin Source File

SOURCE=..\..\libexslt\functions.c
# End Source File
# Begin Source File

SOURCE=..\..\libexslt\math.c
# End Source File
# Begin Source File

SOURCE=..\..\libexslt\sets.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\libexslt\exslt.h
# End Source File
# Begin Source File

SOURCE=..\..\libexslt\exsltconfig.h
# End Source File
# End Group
# End Target
# End Project
