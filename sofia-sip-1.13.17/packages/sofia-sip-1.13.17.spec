# Sofia SIP UA Library

Summary: Sofia SIP User-Agent library
Name: sofia-sip
Version: 1.13.17
Release: 1%{?dist}
License: LGPL
Group: System Environment/Libraries
URL: http://sf.net/projects/sofia-sip
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: pkgconfig

%define opt_with() %{expand:%%global with_%{1} %%{?_with_%{1}:1}%%{?!_with_%{1}:0}}
%define opt_without() %{expand:%%global with_%{1} %%{!?_without_%{1}:1}%%{?_without_%{1}:0}}

# Options:
%opt_with doxygen	- Generate documents using doxygen and dot
%opt_with check		- Run tests
%opt_with openssl	- Always use OpenSSL (TLS)
%opt_with glib		- Always use glib-2.0 (>= 2.2)
%opt_with sctp		- Include SCTP transport

%define have_doxygen %{?_with_doxygen:1}%{!?_with_doxygen:0}
%define have_openssl %(%{?!_with_openssl:pkg-config 'openssl >= 0.9.7'&&}echo 1||echo 0)
%define have_glib %(%{?!_with_glib:pkg-config 'glib-2.0 >= 2.2'&&}echo 1||echo 0)

%if %{have_doxygen}
BuildRequires: doxygen >= 1.3, graphviz
%endif
%if %{have_openssl}
BuildRequires: openssl-devel >= 0.9.7
%endif
%if %{have_glib}
BuildRequires: glib2-devel >= 2.2
%endif

%description
Sofia SIP is a RFC-3261-compliant library for SIP user agents and other
network elements.

%prep
%setup -q

%build
options="--disable-dependency-tracking"
options="$options --with-pic --enable-shared --disable-static"
%if !%{have_glib}
options="$options --without-glib"
%endif
%if %{with_sctp}
options="$options --enable-sctp"
%endif

%configure $options

make %{_smp_mflags}
%if %{have_doxygen}
make doxygen
%endif

# XXX comment next line to build with non-check aware rpmbuild.
%check
%if %{with_check}
make check
%endif

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

# Remove extra files
find $RPM_BUILD_ROOT -type f -name *.la -print0 | xargs -0 rm

%if %{have_doxygen}
# Manually install development docs into manual
cp -p -r libsofia-sip-ua/docs/html manual
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libsofia-sip-ua.so.*
%doc AUTHORS COPYING COPYRIGHTS README

%if %{have_glib}
# note: soname in pkgname allows install of multiple library versions
# The glib interface is still a bit unstable
%package	glib3
Summary:	GLIB bindings for Sofia-SIP
Group:		System Environment/Libraries
Requires:	sofia-sip
Obsoletes:	sofia-sip-glib < %{version}-%{release}
Provides:	sofia-sip-glib = %{version}-%{release}

%description	glib3
GLib interface to Sofia SIP User Agent library.

%files 		glib3
%defattr(-,root,root,-)
%{_libdir}/libsofia-sip-ua-glib.so.*
%doc AUTHORS COPYING COPYRIGHTS README libsofia-sip-ua-glib/ChangeLog

%post glib3 -p /sbin/ldconfig
%postun glib3 -p /sbin/ldconfig

%endif

%package	devel
Summary:	Sofia-SIP Development Package
Group:		Development/Libraries
Requires:	sofia-sip = %{version}-%{release}
Obsoletes:	sofia-devel < %{version}-%{release}
Provides:	sofia-devel = %{version}-%{release}

Requires:	pkgconfig

%description	devel
Development package for Sofia SIP UA library. This package includes
static libraries and include files.

%if !%{with_doxygen}
The reference documentation for Sofia SIP UA library is available at
<http://sofia-sip.sourceforge.net/development.html>
%endif

%files 		devel
%defattr(-,root,root,-)
%dir %{_includedir}/sofia-sip*
%dir %{_includedir}/sofia-sip*/sofia-sip
%{_includedir}/sofia-sip*/sofia-sip/*.h
%{_includedir}/sofia-sip*/sofia-sip/*.h.in
%dir %{_includedir}/sofia-sip*/sofia-resolv
%{_includedir}/sofia-sip*/sofia-resolv/*.h
%dir %{_datadir}/sofia-sip
%{_datadir}/sofia-sip/tag_dll.awk
%{_datadir}/sofia-sip/msg_parser.awk
%{_libdir}/libsofia-sip-ua.so
%{_libdir}/pkgconfig/sofia-sip-ua.pc
%doc TODO README.developers

%if %{have_glib}
# note: no soname here as no multiple glib-devel packages can co-exist in peace
%package	glib-devel
Summary:	GLIB bindings for Sofia SIP development files
Group:			Development/Libraries
Requires:	sofia-sip-glib3 = %{version}-%{release}
Requires:	sofia-sip-devel >= 1.13
BuildRequires:	glib2-devel >= 2.2

%description	glib-devel
Development package for Sofia SIP UA Glib library. This package includes
static libraries and include files for developing glib programs using Sofia
SIP.

%files 		glib-devel
%defattr(-,root,root,-)
%{_includedir}/sofia-sip*/sofia-sip/su_source.h
%{_libdir}/libsofia-sip-ua-glib.so
%{_libdir}/pkgconfig/sofia-sip-ua-glib.pc
%endif

%package	docs
Summary:	Sofia-SIP Development Manual Package
Group:		Documentation
%description	docs
HTML reference documentation for Sofia SIP UA library.

%if %{have_doxygen}
%files docs
%defattr(-,root,root,-)
%doc manual
%endif

%package	utils
Summary:	Sofia-SIP Command Line Utilities
Group:		Applications/Internet
Requires:	sofia-sip = %{version}-%{release}
Obsoletes:	sofia-utils < %{version}-%{release}
Provides:	sofia-utils = %{version}-%{release}
%description	utils
Command line utilities for Sofia SIP UA library.

%files utils
%defattr(-,root,root,-)
%{_bindir}/localinfo
%{_bindir}/addrinfo
%{_bindir}/sip-options
%{_bindir}/sip-date
%{_bindir}/sip-dig
%{_bindir}/stunc
%{_mandir}/man?/*

%changelog
* Thu Dec  7 2006 Pekka Pessi <ppessi at gmail.com> - 1.12.4-1
- Silenced all rpmlint warnings on FC6.

* Wed Dec  6 2006 Pekka Pessi <ppessi at gmail.com> - 1.12.4-0
- Fixing optional values on Fedora. rpmlinted. No doxygen docs.

* Tue Dec  5 2006 Pekka Pessi <ppessi at gmail.com> - 1.12.4
- Bumped version. rpmlinted.

* Tue Dec  5 2006 Kai Vehmanen <first.lastname at nokia.com>
- The 'nua-glib' module, and the related dependency to gobject, has been
  removed from the sofia-sip package

* Fri Oct  6 2006 Pekka Pessi <ppessi at gmail.com> - 1.12.3
- Autodetecting openssl, glib and gobject support with pkg-config
  (use --with openssl --with glib and --with gobject to force them)

* Mon Sep 18 2006 Kai Vehmanen <first.lastname at nokia.com>
- Removed *.m4 files from the distribution package.

* Fri Aug 11 2006 Kai Vehmanen <first.lastname at nokia.com>
- Modified the install location of the awk scripts.

* Thu Jun 15 2006 Kai Vehmanen <first.lastname at nokia.com>
- Added library soname to sofia-sip-glib package name.
- Modified dependencies - the glib subpackages do not depend
  on a specific version of sofia-sip anymore.

* Wed Mar 08 2006 Kai Vehmanen <first.lastname at nokia.com>
- Added libsofia-sip-ua-glib to the package.

* Tue Nov 15 2005 Kai Vehmanen <first.lastname at nokia.com>
- Removed the --includedir parameter. The public headers are
  now installed under includedir/sofia-sip-MAJOR.MINOR/

* Thu Oct 20 2005 Pekka Pessi <first.lastname at nokia.com>
- Using %%{_lib} instead of lib

* Thu Oct  6 2005 Pekka Pessi <first.lastname at nokia.com>
- Added sub-package utils

* Thu Oct  6 2005 Pekka Pessi <first.lastname at nokia.com> - 1.11.0
- Added %%{?dist} to release

* Sat Jul 23 2005 Pekka Pessi <first.lastname at nokia.com> - 1.10.1
- Initial build.
