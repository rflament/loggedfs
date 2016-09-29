Summary: Transparent fuse-filesystem which allows to log every operation that happens in the backend filesystem. 
Name: loggedfs
Version: 0.3
Release: 1.fc6
Source: %{name}-%{version}.tar.bz2
Packager: Remi Flament <rflament at laposte dot net>
Vendor: Remi Flament <rflament at laposte dot net>
License: GPL
Group: Development/Tools
Url: http://loggedfs.sourceforge.net/
Requires: fuse >= 2.6 fuse-libs >= 2.6 rlog >= 1.3 pcre >= 6.6 libxml2 >= 2.6


%description
LoggedFS is a transparent fuse-filesystem which allows you to log every operation that happens in the backend filesystem. Logs can be written to syslog, to a file, or to standard output. It comes with an XML configuration file in which you can choose exactly what you want to log. You can add filters on users, operations (open, read, write, chown, chmod, etc.), filenames, and return codes. Filename's filters are regular expressions. Since it is fuse-based, you don't need to change anything in your kernel or hard disk partition to use it.

%prep
%setup -q

%build
make

%install
make install

%files
%defattr(-,root,root)
/usr/bin/loggedfs
/etc/loggedfs.xml
%doc /usr/share/man/man1/loggedfs.1*
