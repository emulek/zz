#include <assert.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <libintl.h>

#include "../config.h"
#include "gettext.h"
#include "getopt.hpp"
#include "encoder.hpp"
#include "rs.hpp"
#include "zfile.hpp"

#define ERR_STR_LEN 1024

const Options *options = NULL;

#define STACK_SIZE 100
#define BLOCK_SIZE_DEFAULT 4000

RSD clk(const char**sp, char cend)
{
	RSD stack_v[STACK_SIZE];
	char stack_c[STACK_SIZE];
	int stack_p[STACK_SIZE];
	int stack_pointer = 0;
	const char *s = *sp;
	stack_v[stack_pointer] = 0;
	stack_p[stack_pointer] = 0;
	stack_c[stack_pointer] = '\0';
	RSD v = -1;
	int j;
	do{
		if(*s >= '0' && *s <= '9')
		{
			int x = 0;
			while(*s >= '0' && *s <= '9')
			{
				x *= 10;
				x += *s-'0';
				s++;
			}
			stack_v[++stack_pointer] = x;
			stack_c[stack_pointer] = *s++;
			*sp = s;
		}
		else if(*s == '(')
		{
			*sp = s+1;
			v = clk(sp, ')');
			if(v == -1)	return v;
			stack_v[++stack_pointer] = v;
			stack_c[stack_pointer] = *(*sp)++;
		}
		else
		{
			v = -1;
			return v;
		}
		if(stack_c[stack_pointer] == cend)
			stack_p[stack_pointer] = 0;
		else if(stack_c[stack_pointer] == '+' || stack_c[stack_pointer] == '-')
			stack_p[stack_pointer] = 1;
		else if(stack_c[stack_pointer] == '*' || stack_c[stack_pointer] == '/')
			stack_p[stack_pointer] = 2;
		else if(stack_c[stack_pointer] == '^')
			stack_p[stack_pointer] = 3;
		else
		{
			v = -1;
			return v;
		}
		//printf("%s %d %c\n",
		//		stack_v[stack_pointer].print(), stack_p[stack_pointer], stack_c[stack_pointer]);
		while(stack_p[stack_pointer-1] >= stack_p[stack_pointer])
		{
			switch(stack_c[stack_pointer-1])
			{
				case 0:
					v = stack_v[stack_pointer];
					return v;
				case '+':
					if(stack_v[stack_pointer-1]==-1)	break;
					if(stack_v[stack_pointer]==-1)	{ stack_v[stack_pointer-1]=-1; break; }
					stack_v[stack_pointer-1] += stack_v[stack_pointer];
					break;
				case '-':
					if(stack_v[stack_pointer-1] == -1)	break;
					if(stack_v[stack_pointer]==-1)	{ stack_v[stack_pointer-1]=-1; break; }
					stack_v[stack_pointer-1] -= stack_v[stack_pointer];
					break;
				case '*':
					if(stack_v[stack_pointer-1] == -1)	break;
					if(stack_v[stack_pointer]==-1)	{ stack_v[stack_pointer-1]=-1; break; }
					stack_v[stack_pointer-1] *= stack_v[stack_pointer];
					break;
				case '/':
					if(stack_v[stack_pointer-1] == -1)	break;
					if(stack_v[stack_pointer]==0 || stack_v[stack_pointer]==-1)
						stack_v[stack_pointer-1] = -1;// x/0
					else
						stack_v[stack_pointer-1] /= stack_v[stack_pointer];
					break;
				case '^':
					if(stack_v[stack_pointer-1] == -1)	break;
					if(stack_v[stack_pointer]==-1)	{ stack_v[stack_pointer-1]=-1; break; }
					v = 1;
					for(j = 0; j < stack_v[stack_pointer].get(); j++)
						v *= stack_v[stack_pointer-1];
					stack_v[stack_pointer-1] = v;
					break;
				default:
					v = -1;
					return v;
			}
			stack_c[stack_pointer-1] = stack_c[stack_pointer];
			stack_p[stack_pointer-1] = stack_p[stack_pointer];
			stack_pointer--;
			//printf("= %s %d '%c'\n",
			//		stack_v[stack_pointer].print(), stack_p[stack_pointer], stack_c[stack_pointer]);
		}
	}while(true);

	return v;
}

Options::Options(int argc, char *const argv[]) :
	error_string(NULL),
	err_no(0),
	code_n(15),
	code_k(7),
	verbose(0),
	n_args(0),
	args(NULL),
	myname(argv[0]),
	repository("."),
	extension("zz"),
	target("."),
	is_decode(false),
	block_size(BLOCK_SIZE_DEFAULT)
{
	assert(options == NULL);
	options = this;
	error_string = new char[ERR_STR_LEN];
	RSD cv;
	int is_clk = 0;
	int c;
	const char *s;

	// version
	version_low = version_hi = 0;
	if(ZString::regcomp("^([0-9]+)\\.([0-9]+)$"))
	{
		ZString v(VERSION);
		ZString v1(v);
		v1.regsub("\\1");
		version_hi = v1.convert_to_uint64();// major
		v.regsub("\\2");
		version_low = v.convert_to_uint64();// minor
	}

	// basename(myname)
	if(ZString::regcomp(".*/"))	myname.regsub("");

	while(true){// parsing options
		int option_index = 0;
		static struct option long_options[] = {// list options
			{"block-size",       required_argument, 0, 'B'},
			{"calculate",        required_argument, 0, 'c'},
			{"decode",           no_argument,       0, 'd'},
			{"verbose",          optional_argument, 0, 'v'},
			{"help",             no_argument,       0, 'h'},
			{"version",          no_argument,       0, 'V'},
			{"repository",       required_argument, 0, 'r'},
			{"target-dir",       required_argument, 0, 't'},
			{0,                  0,                 0,  0 }
		};

		c = getopt_long(argc, argv, "B:c:dhvVr:t:",
			long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'B':
				{
					ZString a(optarg);
					uint64_t bs = a.convert_to_uint64(ZString::CF_PF);
					if(bs>=(1ULL<<32))
						set_error(1, (const ZString)gettext("Invalid argument for option")+" --block-size");
					else
						block_size = bs;
				}
				break;

			case 'c':
				is_clk = 1;
				s = optarg;
				cv = clk(&s, '\0');
				printf("%s=", optarg);
				if(cv == -1)
					printf(gettext("%sERROR%s\n"), ESC_RED, ESC_NORMAL);
				else
					printf("%s%u%s\n", ESC_GREN, cv.get(), ESC_NORMAL);
				err_no = 63;
				break;

			case 'd':
				is_decode = true;
				break;

			case 'v':
				if(optarg)
				{
					s = optarg;
					if(s[0]<'0' || s[0]>'9' || s[1]!='\0')
					{
						printf("%s: --verbose=ARG: Invalid argument ARG(%s)\n",
								myname.get(), s);
						set_error(1, (const ZString)gettext("Invalid argument for option")+" --verbose");
						break;
					}
					verbose = s[0]-'0';
				}
				else
					verbose = verbose<9? verbose+1: 9;
				break;

			case 'h':
				printf("\n");
				printf(gettext("Usage: %s [OPTION]... FILES...\n"), myname.get());
				printf(gettext("Options:\n"));
				printf(gettext("\t-B,\t--block-size=BLK_SIZE\t\t\tSet block size for encode (default %d bytes).\n"), block_size);
				printf(gettext("\t-c,\t--calculate=EXPRESSIONS\t\t\tCalculate EXPRESSIONS (for GF(%u)).\n"), RS_M);
				printf(gettext("\t-d,\t--decode\t\t\t\tDecode file(s).\n"));
				printf(gettext("\t-h,\t--help\t\t\t\t\tThis help.\n"));
				printf(gettext("\t-r,\t--repository=REPOSITORY\t\t\tSave/Load encode files to/from REPOSITORY.\n"));
				printf(gettext("\t-t,\t--target-dir=TARGET\t\t\tDecode file(s) to TARGET directory.\n"));
				printf(gettext("\t-v,\t--verbose[=VERBOSITY_LEVEL]\t\tVerbose output (VERBOSITY_LEVEL={0..9}).\n"));
				printf(gettext("\t-V,\t--version\t\t\t\tVersion.\n"));
				err_no = 63;
				break;

			case 'r':
				repository = SFile::remove_tail_slash(optarg);
				break;

			case 't':
				target = SFile::remove_tail_slash(optarg);
				break;

			case 'V':
				printf("\n");
				printf(gettext("%s version %s (HG_REVISION: %s)\n\n"), PACKAGE_NAME, VERSION, HG_REVISION);
				printf(gettext("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"));
				printf(gettext("This is free software: you are free to change and redistribute it.\n"));
				printf(gettext("There is NO WARRANTY, to the extent permitted by law.\n\n"));
				printf(gettext("Written by drBatty <%s>\n"), PACKAGE_BUGREPORT);
				err_no = 63;
				break;

			case '?':
				set_error(1, gettext("Invalid argument"));
				break;

			default:
				printf(gettext("?? getopt returned character code 0%o ??\n"), c);
				set_error(1, gettext("Invalid argument"));
				break;
		}
	}//while

	if(!is_clk && optind<argc)
	{// save arguments to Options::args;
		args = new ZString[argc-optind];
		while(optind<argc)
			args[n_args++] = argv[optind++];
	}
	if(verbose>=6)
	{
		printf(gettext("-- Command line parsing OK.\n"));
		printf(gettext("myname: %s (VERSION: '%s' version_hi: %u, version_low: %u)\n"),
				myname.get(), VERSION, version_hi, version_low);
		printf(gettext("HG_REVISION: %s\n"), HG_REVISION);
		printf(gettext("is_decode: %s\n"), is_decode? "true": "false");
		printf(gettext("verbosity level: %d\n"), verbose);
		printf(gettext("repository directory: '%s'\n"), repository.get());
		if(is_decode)
		{
			printf(gettext("target directory: '%s'\n"), target.get());
		}
		else
		{// encode
			printf(gettext("block size: %u\n"), block_size);
		}
		int j;
		for(j=0; j<n_args; j++)
			printf(gettext("file #%d: '%s'\n"), j, args[j].get());
		printf(gettext("--\n"));
	}
}

Options::~Options()
{
	delete[] args;
	delete[] error_string;
	options = NULL;
}

void Options::set_file_error(int en, const ZString &fn) const
{
	err_no = en;
	sprintf(error_string,
			gettext("%s: '%s' not aviable: %s"),
			myname.get(),
			fn.get(),
			strerror(errno));
}

void Options::set_file_error(int en, const ZString &fn, const ZString &es) const
{
	err_no = en;
	sprintf(error_string,
			"%s: '%s' ",
			myname.get(),
			fn.get());
	strcat(error_string, es.get());
}

void Options::set_error(int en, const ZString &es) const
{
	err_no = en;
	sprintf(error_string,
			"%s: %s",
			myname.get(),
			es.get());
}

int Options::print_error() const
{
	fprintf(stderr, "%s\n", error_string);
	return err_no;
}
