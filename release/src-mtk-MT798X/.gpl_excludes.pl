#!/usr/bin/perl

sub append_gpl_excludes
{
	my $fexclude;
	my $uc_model;
	my $uc_modeldir;
	my @tools_array = map m|([^/]+)$|, <../../tools/*.bz2 ../../tools/*.gz ../../tools/*.tgz>;
	my $default_toolchain = "openwrt-gcc840_musl-1.1.24.aarch64.tar.bz2";

	$uc_model=uc($_[0]);
	$uc_modeldir=$_[1];

	system("touch ./.gpl_excludes.sysdeps");
	open($fexclude, ">./.gpl_excludes.sysdeps");

	for (@tools_array) {
		if ( $_ ne $default_toolchain ) { # only keep the toolchain we needed
			print $fexclude "tools/$_\n";
		}
	}

	close($fexclude);
}
	
if ( @ARGV >= 2 ) {
	append_gpl_excludes($ARGV[0], $ARGV[1]);
}
else {
	print "usage: .gpl_excludes.pl [model] [modeldir]\n";
}

