%define ver     1.7.3
%define rel     1

Summary: system35 scenario decoder on X
Name: xsystem35
Version: %{ver}pre5
Release: %{rel}
License: GPL
Group: Amusements/Games
Source: http://www.aist-nara.ac.jp/~masaki-c/private/unitbase/xsys35/down/xsystem35-1.7.3pre5.tar.gz
# Source: http://www.aist-nara.ac.jp/~masaki-c/private/unitbase/xsys35/down/test/xsystem35-%{ver}-pre5.tar.gz
Requires: gtk+ >= 1.0.0
BuildPreReq: gtk+-devel
URL: http://www.aist-nara.ac.jp/~masaki-c/private/unitbase/xsys35/index.html
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
system35 scenario decoder on X 
Xsystem35 enables Alice Soft's games on X.

%description -l ja
アリスソフトのゲームをX上で実行するためのプログラムです。
最初に実行する前に ドキュメントディレクトリの xsys35rc.sample
をホームディレクトリに.xsys35rcとしてコピーしてお使い下さい。

%changelog

%prep

%setup -q
# %setup -q  -n %{name}-%{ver}-pre5

#%patch -p1

rm -rf %{buildroot}


%build

./configure --prefix=/usr --enable-audio=oss,alsa,esd --enable-cdrom=linux,mp3 --enable-midi=extp,raw,seq --disable-debug --enable-qmidi
make

%install

%makeinstall

%clean
rm -rf %{buildroot}

%pre

%post

%preun

%postun

%files 
%defattr(-, root, root)
%doc ABOUT-NLS COPYING INSTALL doc/* src/xsys35rc.sample
%doc contrib
%doc patch
%{_libdir}/xsystem35/*
%{_bindir}/xsystem35
%{_datadir}/locale/*/LC_MESSAGES/xsystem35*

%changelog
* Sun Dec 21 2003 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.7.2]
- version 1.7.2

* Sun Aug 31 2003 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.7.1]
- version 1.7.1

* Thu May 01 2003 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.7.0]
- version 1.7.0 preX

* Sat Feb 01 2003 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.6.0]
- version 1.6.0

* Sun Jan 05 2003 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.6.0]
- 1.6.0 preX

* Tue May 14 2002 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.5.x]
- revised for xsystem35-1.5.x

* Sun May 1 2001 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.4.0]
- update to xsystem35-1.4.0

* Sun Apr 1 2001 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.3.4]
- update to xsystem35-1.3.4

* Sun Nov 26 2000 CHIKAMA Masaki <masaki-c@is.aist-nara.ac.jp>
  [xsystem35-1.3.3]
- import spec file from JRPM to tar ball. (Grate thanks to Toshiya Takagi)

* Sun Dec 4 1999 Toshiya Takagi <jwtoshi@mars.dti.ne.jp>
  [xsystem35-1.2.4-1]
- Updated to xsystem35-1.2.4

* Sun Sep 5 1999 Toshiya Takagi <jwtoshi@mars.dti.ne.jp>
  [xsystem35-1.2.3-2]
- Use TiMidity and mp3 instead of CD-DA

* Thu Aug 26 1999 Toshiya Takagi <jwtoshi@mars.dti.ne.jp>
  [xsystem35-1.2.3-1]
- Updated to xsystem35-1.2.3

* Sat Jul 24 1999 Toshiya Takagi <jwtoshi@mars.dti.ne.jp>
  [xsystem35-1.2.2-1]
- Updated to xsystem35-1.2.2

* Thu Jun 10 1999 Toshiya Takagi <jwtoshi@mars.dti.ne.jp>
  [xsystem35-1.2.1-1]
- Updated to xsystem35-1.2.1

* Mon May 24 1999 Toshiya Takagi <jwtoshi@mars.dti.ne.jp>
  [xsystem35-1.2.0-1]
- Updated to xsystem35-1.2.0

* Thu May 13 1999 Toshiya Takagi <jwtoshi@mars.dti.ne.jp>
- Changed /usr/local/lib/xsystem35 to /usr/X11R6/lib/xsystem35

* Mon May 3 1999 Toshiya Takagi <jwtoshi@mars.dti.ne.jp>
- First release against glibc

