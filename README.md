# Dandelions: B Cell Receptor Sequence Clustering and Lineage Graph Generation

## Getting Started

Dandelions is developed and tested on Microsoft Windows 11 and WSL2 with a
cross-platform UI built from wxWdigets. Some differences in graphical output
will be present due to differences in the GDI and GTK2 backends of wxWidgets.
In general, the Windows version will give better results.

## Prerequisites

The Windows version can be built with recent versions (>=17) of Visual Studio
Community Edition. Compilation on Linux requires a C++ compiler and standard 
library with support for C++20 features, (std::filesystem, std::span and the 
like). The Linux build was tested in WSL2 with clang++ version 12. Beyond the
C++ standard libraries, Dandelions also depends on wxWidgets. Up to date 
Windows installers for wxWidgets can be found at 
https://www.wxwidgets.org/downloads/. The visual studio project file in this
repository expects to find wxWidgets in C:\wxWidgets-3.2.2.1 so you will need
to update the project setting if you install a more recent version of wx or
install it somewhere else. For the Linux build use the package
manager to install the wx dependencies. For WSL2 this should be wx-common and
libwxgtk3.0-gtk3-dev. Currently the Linux port suffers from some issues
regarding gdk, tooltip windows, and grabbing the mouse pointer so we recommend
the Windows version. There is no Intel-specific SIMD code so in theory
Dandelions should run on MacOS if Homebrew can provide modern clang++ and a 
wxWidgets package but I have not tested it.

## Installing

For a Windows install, we recommend simply downloading the binary installer
in the installers folder of the repository (dandelions_setup.msi). For the 
Linux build, clone the Git repository, cd into it, run make, and copy the 
executable bin/dandelions to a directory in your shell's path. For example...

```
$cd ~
$git clone https://github.com/baileych-bi/dandelions
$cd dandelions/src
$make
$sudo cp bin/dandelions /usr/local/bin
$dandelions
```

..should do the trick if the correct compilers and dependencies are
installed.


## Author

Charles C Bailey

## Acknowledgements

Dandelions makes uses the excellent cross-platform GUI toolkit wxWidgets
(https://wiki.wxwidgets.org/) for its UI. The contrasting palette of
centroid colors was taken from Sasha Trubetskoy's website at
https://sashamaps.net/docs/resources/20-colors/.

