#! /usr/bin/perl

binmode IN, ":bytes";
$file = $ARGV[0];

print "#include <wx/bitmap.h>\n";
print "#include <wx/image.h>\n";
print "#include <wx/mstream.h>\n";
print "\n";

foreach $file (@ARGV) {
	open(IN, $file) or die "Can't open $file: $!\n";
	$count = 0;
	$len = -s $file;
	$filelist[$numfiles] = $file;
	$file =~ /^([-_A-Za-z0-9\/]+)/;
	$name = "png_$1";
	$name =~ s/\//_/g;
	$name =~ s/-/_/g;
	$size[$numfiles] = $len;
	$list[$numfiles++] = $name;
	print "static unsigned char ${name}[$len] = \{\n";
	while (read(IN, $byte, 1) == 1) {
		$n = ord($byte);
		printf '0x%02X', $n;
		print ',' if $count < $len - 1;
		print "\n" if $count++ % 12 == 11;
	}
	print "\};\n";
	close(IN);
}
print "\nstatic const unsigned char *png_image_list[] = {\n";
for ($i=0; $i<$numfiles; $i++) {
	print "$list[$i]";
	print ',' if $i < $numfiles - 1;
	print "\n";
}
print "\};\n";

print "\nstatic const char *png_image_name[] = {\n";
for ($i=0; $i<$numfiles; $i++) {
	print "\"$filelist[$i]\"";
	print ',' if $i < $numfiles - 1;
	print "\n";
}
print "\};\n";

print "\nstatic const unsigned int png_image_size[] = {\n";
for ($i=0; $i<$numfiles; $i++) {
	print "$size[$i]";
	print ',' if $i < $numfiles - 1;
	print "\n";
}
print "\};\n";

print "\nstatic wxBitmap * image_list[] = {\n";
for ($i=0; $i<$numfiles; $i++) {
        print "NULL";
        print ',' if $i < $numfiles - 1;
        print "\n";
}

print "\};\n\n";

print <<EOT;
wxBitmap MyBitmap(size_t index)
{
	if (index >= $numfiles || index < 0) return wxNullBitmap;
	wxBitmap *p = image_list[index];
	if (p) return *p;
	wxMemoryInputStream istream(png_image_list[index], png_image_size[index]);
	wxImage img(istream, wxBITMAP_TYPE_PNG);
	p = new wxBitmap(img);
	return *p;
}

wxBitmap MyBitmap(const char *name)
{
	for (size_t i=0; i < $numfiles; i++) {
		if (strcmp(name, png_image_name[i]) == 0) return MyBitmap(i);
	}
	return wxNullBitmap;
}
EOT











