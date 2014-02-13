package ppresolve;

use strict;

# version number of module
my $VERSION = '1.0.0';

require "function.pm";

#
# this function is no longer used by any of the vhdl files. It's function is
# to do multiple passes on the same piece of data, usefull for resolving array
# values in the preprocessor, for example:
#
# #define ARRAY_0 1
# #define ARRAY_1 2
# #define ARRAY_2 3
#
# #define INDEX 2
#
# "ARRAY_INDEX" would now be preprocessed into ARRAY_2, while 
# "ppresolve(ARRAY_INDEX)" would be preprocessed into "2".
sub ppresolve {
	return Filepp::RunProcessors("@_");
}
Function::AddFunction("ppresolve", "ppresolve::ppresolve");

#
# convert a hexadecimal or octal number to decimal number to use as first
# parameter for the vhdl conv_std_logic_vector function.
sub unhex {
	# taken from http://www.cs.cmu.edu/People/rgs/pl-exp-conv.html
	my $val = "@_";
	$val = oct($val) if $val =~ /^0/;
	return "$val";
}
Function::AddFunction("unhex", "ppresolve::unhex");

#
# convert a numerical value to a vhdl hex number
# (10 => x"A", second parameter contains nr of bits)
sub pp_to_std_logic_vector {
	my ($val, $size) = @_;
	
	$val = unhex($val);
	$size = unhex($size);	

	my $bitstring = "";
	for (my $i=0; $i<$size; $i++) {
		if ($val & (1<<$i)) {
			$bitstring = "1" . $bitstring
		} else {
			$bitstring = "0" . $bitstring
		}
	}

	return "\"$bitstring\"";
}
Function::AddFunction("pp_to_std_logic_vector", "ppresolve::pp_to_std_logic_vector");


#
# the following function is run as preprocessor over each "data block", it
# removes all empty lines making the vhdl files much readable after being
# processed (the original files contained lots of empty lines, since the
# c comment removing processor removes all comments, but keeps the line in
# touch, meaning a block of comment would result in a block of empty lines
# on the output.
sub remove_whiteareas {
	my $line = "@_";

	$line = join("", split(/([\t ]*[\r\n])+/, $line));

	if ($line ne "\n") {
		return "$line";
	} else {
		return "";
	}
}
Filepp::AddProcessor("ppresolve::remove_whiteareas",0,1);

return 1;