## Makefile.am for libsim
## Based on LibExample from
## http://www.openismus.com/documents/linux/building_libraries/building_libraries

## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.

AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS = -std=c++11
lib_LTLIBRARIES = libsim-@SIM_API_VERSION@.la
libsim_@SIM_API_VERSION@_la_SOURCES = sim/Simulation.cc sim/Simulation.hh sim/sim.hh
bin_PROGRAMS = testsim
testsim_SOURCES = src/testsim.cc src/test.cc src/test.hh
testsim_LDADD = libsim-@SIM_API_VERSION@.la

## Instruct libtool to include ABI version information in the generated shared
## library file (.so).  The library ABI version is defined in configure.ac, so
## that all version information is kept in one place.

libsim_@SIM_API_VERSION@_la_LDFLAGS = -version-info $(SIM_SO_VERSION)

sim_includedir = $(includedir)/sim-$(SIM_API_VERSION)

nobase_sim_include_HEADERS = 	sim/sim.hh \
				sim/Simulation.hh

## The generated configuration header is installed in its own subdirectory of
## $(libdir).  The reason for this is that the configuration information put
## into this header file describes the target platform the installed library
## has been built for.  Thus the file must not be installed into a location
## intended for architecture-independent files, as defined by the Filesystem
## Hierarchy Standard (FHS).
## The nodist_ prefix instructs Automake to not generate rules for including
## the listed files in the distribution on 'make dist'.  Files that are listed
## in _HEADERS variables are normally included in the distribution, but the
## configuration header file is generated at configure time and should not be
## shipped with the source tarball.

sim_libincludedir = $(libdir)/sim-$(SIM_API_VERSION)/include

## Install the generated pkg-config file (.pc) into the expected location for
## architecture-dependent package configuration information.  Occasionally,
## pkg-config files are also used for architecture-independent data packages,
## in which case the correct install location would be $(datadir)/pkgconfig.

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sim-$(SIM_API_VERSION).pc

## Define an independent executable script for inclusion in the distribution
## archive.  However, it will not be installed on an end user's system due to
## the noinst_ prefix.

## dist_noinst_SCRIPTS = autogen.sh

#EXTRA_DIST = autogen.sh

dist_doc_data = README
