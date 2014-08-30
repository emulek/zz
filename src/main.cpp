#include <stdio.h>
#include <assert.h>
#include <locale.h>

#include "../config.h"
#include "gettext.h"
#include "getopt.hpp"
#include "encoder.hpp"
#include "zfile.hpp"

int main(int argc, char *const argv[])
{
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	Options op(argc, argv);
	if(options->get_errno())
		return options->get_errno()==63? 0: options->print_error();
	if(options->n_args == 0)
	{
		printf(gettext("no files\n"));
		return 0;
	}

	int j;
	Encoder encoder;
	for(j=0; j<options->n_args; j++)
	{// encode/decode files
		ZString fn(options->args[j]);
		if(!options->is_decode)
		{// encode
			if(!encoder.encode_file(fn))
				return options->print_error();
			if(options->verbose >=4)
				printf(gettext("---------- '%s' encode OK\n"), fn.get());
		}
		else
		{// decode
			if(!encoder.decode_file(fn))
				return options->print_error();
			if(options->verbose >=4)
				printf(gettext("---------- '%s' decode OK\n"), fn.get());
		}
	}
	return 0;
}
