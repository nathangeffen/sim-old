## configure.ac

AC_INIT([libsim], [0.1], [nathangeffen@quackdown.info], [sim],
	[https://github.com/nathangeffen/sim])
AC_PREREQ([2.69])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.10 subdir-objects -Wall no-define])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])
AM_PROG_AR
AC_PROG_CXX
LT_INIT([disable-static])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strtol])
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Define these substitions here to keep all version information in one place.
# For information on how to properly maintain the library version information,
# refer to the libtool manual, section "Updating library version information":
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
AC_SUBST([SIM_SO_VERSION], [1:1:0])
AC_SUBST([SIM_API_VERSION], [0.1])

# Override the template file name of the generated .pc file, so that there
# is no need to rename the template file when the API version changes.
AC_CONFIG_FILES([Makefile
                 sim-${SIM_API_VERSION}.pc:sim.pc.in])
AC_OUTPUT
