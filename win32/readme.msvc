libxslt Readme For Usage With MSVC
----------------------------------

If you would like to compile libxslt using Microsoft Visual
C/C++ IDE, then you must know that it cannot work out of the
box. You have to modify the project files.

This is not happening just in order to be inconvenient. The fact
is that libxslt needs libxml in order to compile and work and that
I have no way to know where you keep libxml on your system.

In order to compile, you must tell the compiler where to look
for the libxml headers. Likewise, you must tell the linker where
to look for libxml library.


Adapting The Header Search Path
-------------------------------

In the MSVC IDE, go to Project->Settings and choose the C/C++
options and select the Preprocessor category. Now, there is a list
of additional include directories, separated by comma. The last
entry is the location of libxml headers and this is the one which
you must adapt to your environment.

Adapting The Library Search Path
--------------------------------

In the MSVC IDE, go to Project->Settings and choose the Link
options and select the Input category. Now, there is an Additional 
Library Path which contains the list of additional directories, 
separated by comma. The last entry is the location of the libxml 
library and this is the one which you must adapt to your environment.

If Something Goes Wrong
-----------------------

Don't panic. Use your common sense and investigate the problem.
If you cannot escape the dread, send me an email and tell me your
problems


27. July 2001, Igor Zlatkovic [igor@stud.fh-frankfurt.de]