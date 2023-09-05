import os

if hasattr(os, 'add_dll_directory'):
    os.add_dll_directory(os.path.join(os.getcwd(), '..', '..', 'libxslt', '.libs'))
    os.add_dll_directory(os.path.join(os.getcwd(), '..', '..', 'libexslt', '.libs'))
    libxml_src = os.getenv('LIBXML_SRC')
    if libxml_src is not None and libxml_src != '':
        os.add_dll_directory(os.path.join(libxml_src, '.libs'))
