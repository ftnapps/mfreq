Name:		mfreq
Version:	3.15
Release:	1%{?dist}
Summary:	a suite of tools for FTS style file requesting

Group:		Applications/FTN
License:	EUPL
URL:		https://github.com/ftnapps/mfreq
Source0:	%{name}-%{version}.tar.gz

#BuildRequires:	
#Requires:	

%description
mfreq is a suite of tools for FTS style file requesting. It consists of a
file index generator, filelist generator and a SRIF compatible file request
handler.

%prep
%setup -q


%build
make


%install
mkdir -p "%{buildroot}%{_bindir}"
mkdir -p "%{buildroot}%{_docdir}/%{name}-%{version}"
make install PREFIX=%{buildroot}/usr
cp README CHANGES EUPL-v1.1.pdf "%{buildroot}%{_docdir}/%{name}-%{version}"
cp -a sample-cfg "%{buildroot}%{_docdir}/%{name}-%{version}/"

%files
%defattr(-,root,root)
%{_bindir}/mfreq-index
%{_bindir}/mfreq-list
%{_bindir}/mfreq-srif
%dir %{_docdir}/%{name}-%{version}
%doc %{_docdir}/%{name}-%{version}/README
%doc %{_docdir}/%{name}-%{version}/CHANGES
%doc %{_docdir}/%{name}-%{version}/EUPL-v1.1.pdf
%dir %{_docdir}/%{name}-%{version}/sample-cfg
%doc %{_docdir}/%{name}-%{version}/sample-cfg/*

%changelog
* Mon Oct 19 2015 Eric Renfro <psi-jack@deckersheaven.com>
- Initial RPM spec from github tag for version 3.13.

