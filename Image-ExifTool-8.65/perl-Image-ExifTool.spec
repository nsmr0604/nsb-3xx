Summary: perl module for image data extraction
Name: perl-Image-ExifTool
Version: 8.65
Release: 1
License: Artistic/GPL
Group: Development/Libraries/Perl
URL: http://owl.phy.queensu.ca/~phil/exiftool/
Source0: Image-ExifTool-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
ExifTool is a customizable set of Perl modules plus an application script
for reading and writing meta information in image, audio and video files,
including the maker note information of many digital cameras by various
manufacturers such as Canon, Casio, FujiFilm, GE, HP, JVC/Victor, Kodak,
Leaf, Minolta/Konica-Minolta, Nikon, Olympus/Epson, Panasonic/Leica,
Pentax/Asahi, Reconyx, Ricoh, Samsung, Sanyo, Sigma/Foveon and Sony.

Below is a list of file types and meta information formats currently
supported by ExifTool (r = read, w = write, c = create):

  File Types
  ------------+-------------+-------------+-------------+------------
  3FR   r     | DVB   r     | M4A/V r     | PBM   r/w   | RWZ   r
  3G2   r     | DYLIB r     | MEF   r/w   | PDF   r/w   | RM    r
  3GP   r     | EIP   r     | MIE   r/w/c | PEF   r/w   | SO    r
  ACR   r     | EPS   r/w   | MIFF  r     | PFA   r     | SR2   r/w
  AFM   r     | ERF   r/w   | MKA   r     | PFB   r     | SRF   r
  AI    r/w   | EXE   r     | MKS   r     | PFM   r     | SRW   r/w
  AIFF  r     | EXIF  r/w/c | MKV   r     | PGF   r     | SVG   r
  APE   r     | F4A/V r     | MNG   r/w   | PGM   r/w   | SWF   r
  ARW   r/w   | FLA   r     | MOS   r/w   | PICT  r     | THM   r/w
  ASF   r     | FLAC  r     | MOV   r     | PMP   r     | TIFF  r/w
  AVI   r     | FLV   r     | MP3   r     | PNG   r/w   | TTC   r
  BMP   r     | FPX   r     | MP4   r     | PPM   r/w   | TTF   r
  BTF   r     | GIF   r/w   | MPC   r     | PPT   r     | VRD   r/w/c
  CHM   r     | GZ    r     | MPG   r     | PPTX  r     | VSD   r
  COS   r     | HDP   r/w   | MPO   r/w   | PS    r/w   | WAV   r
  CR2   r/w   | HTML  r     | MQV   r     | PSB   r/w   | WDP   r/w
  CRW   r/w   | ICC   r/w/c | MRW   r/w   | PSD   r/w   | WEBP  r
  CS1   r/w   | IIQ   r/w   | MXF   r     | PSP   r     | WEBM  r
  DCM   r     | IND   r/w   | NEF   r/w   | QTIF  r     | WMA   r
  DCP   r/w   | ITC   r     | NRW   r/w   | RA    r     | WMV   r
  DCR   r     | J2C   r     | NUMBERS r   | RAF   r/w   | X3F   r/w
  DFONT r     | JNG   r/w   | ODP   r     | RAM   r     | XCF   r
  DIVX  r     | JP2   r/w   | ODS   r     | RAR   r     | XLS   r
  DJVU  r     | JPEG  r/w   | ODT   r     | RAW   r/w   | XLSX  r
  DLL   r     | K25   r     | OGG   r     | RIFF  r     | XMP   r/w/c
  DNG   r/w   | KDC   r     | OGV   r     | RSRC  r     | ZIP   r
  DOC   r     | KEY   r     | ORF   r/w   | RTF   r     |
  DOCX  r     | LNK   r     | OTF   r     | RW2   r/w   |
  DV    r     | M2TS  r     | PAGES r     | RWL   r/w   |

  Meta Information
  ----------------------+----------------------+---------------------
  EXIF           r/w/c  |  CIFF           r/w  |  Ricoh RMETA    r
  GPS            r/w/c  |  AFCP           r/w  |  Picture Info   r
  IPTC           r/w/c  |  Kodak Meta     r/w  |  Adobe APP14    r
  XMP            r/w/c  |  FotoStation    r/w  |  MPF            r
  MakerNotes     r/w/c  |  PhotoMechanic  r/w  |  Stim           r
  Photoshop IRB  r/w/c  |  JPEG 2000      r    |  APE            r
  ICC Profile    r/w/c  |  DICOM          r    |  Vorbis         r
  MIE            r/w/c  |  Flash          r    |  SPIFF          r
  JFIF           r/w/c  |  FlashPix       r    |  DjVu           r
  Ducky APP12    r/w/c  |  QuickTime      r    |  M2TS           r
  PDF            r/w/c  |  Matroska       r    |  PE/COFF        r
  PNG            r/w/c  |  GeoTIFF        r    |  AVCHD          r
  Canon VRD      r/w/c  |  PrintIM        r    |  ZIP            r
  Nikon Capture  r/w/c  |  ID3            r    |  (and more)

See html/index.html for more details about ExifTool features.

%prep
%setup -n Image-ExifTool-%{version}

%build
perl Makefile.PL INSTALLDIRS=vendor

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall DESTDIR=%{?buildroot:%{buildroot}}
find $RPM_BUILD_ROOT -name perllocal.pod | xargs rm

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%doc Changes html
/usr/lib/perl5/*
%{_mandir}/*/*
%{_bindir}/*

%changelog
* Tue May 09 2006 - Niels Kristian Bech Jensen <nkbj@mail.tele.dk>
- Spec file fixed for Mandriva Linux 2006.
* Mon May 08 2006 - Volker Kuhlmann <VolkerKuhlmann@gmx.de>
- Spec file fixed for SUSE.
- Package available from: http://volker.dnsalias.net/soft/
* Sat Jun 19 2004 Kayvan Sylvan <kayvan@sylvan.com> - Image-ExifTool
- Initial build.
