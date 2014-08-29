#ifndef GETOPT_HPP
#define GETOPT_HPP
#include <unistd.h>
#include <stdint.h>

#include "../config.h"
#include "zstring.hpp"

class Options
{
	private:
		mutable char *error_string;
		mutable int err_no;
	public:
		int get_errno() const { return err_no; }
		void reset_error()	const { err_no=0; }
		void set_file_error(int en, const ZString &fn) const;
		void set_file_error(int en, const ZString &fn, const ZString &es) const;
		void set_error(int en, const ZString &es) const;
		uint8_t version_low;
		uint8_t version_hi;
		int print_error() const;

		uint8_t code_n;
		uint8_t code_k;
		int verbose;
		int n_args;
		ZString *args;
		ZString myname;
		ZString repository;
		ZString extension;
		ZString target;
		bool is_decode;
		uint32_t block_size;
		Options(int argc, char *const argv[]);
		~Options();
};

extern const Options *options;

#endif
