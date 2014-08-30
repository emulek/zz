#include <assert.h>
#include <openssl/md5.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>


#include "zfile.hpp"
#include "getopt.hpp"
#include "encoder.hpp"
#include "gettext.h"

bool SFile::open(Mode m)
{
	assert(mode == closed && m != closed);
	mode = m;
	read_buf_count = 0;
	if(options->verbose >= 8)
		printf("SFile::open() open file '%s' (%s)...",
				filename.get(),
				mode == read_dir? "read dir":
					(mode == read_only? "read only": "write only"));

	if(mode == read_dir)
		dir = opendir(filename.get());
	else
		fs = fopen(filename.get(),
				mode == read_only? "r": "w");
	if(mode == read_dir? !dir: !fs)
	{
		options->set_file_error(1, filename);
		mode = closed;
		if(options->verbose >= 9)
			printf(" fail\n");
		return false;
	}
	if(options->verbose >= 8)
		printf(" OK\n");
	return true;
}

const ZString SFile::basename(const ZString &fn)
{
	assert(ZString::regcomp(".*/([^/]+)/?$"));
	ZString r(fn);
	r.regsub("\\1");
	return r;
}

const ZString SFile::remove_tail_slash(const ZString &fn)
{
	assert(ZString::regcomp("/$"));
	ZString r(fn);
	r.regsub("");
	return r;
}


/*
 * Read len bytes to buf
 * if success and no EOF, return true
 * if error read or EOF, return false
*/
bool SFile::read(uint8_t *buf, size_t len)
{
	assert(mode==read_only);
	bool success = false;
	size_t len1 = len;
	for(;;){
		if(read_buf_count!=0)
		{
			if(len<read_buf_count)
			{// all bytes loading
				memcpy(buf, read_buf, len);
				memmove(read_buf, read_buf+len, read_buf_count-len);
				read_buf_count -= len;
				success = true;
				break;
			}
			memcpy(buf, read_buf, read_buf_count);
			buf += read_buf_count;
			len -= read_buf_count;
			read_buf_count = 0;
		}
		if(feof(fs))
		{// EOF or unexpected end of file
			if(len!=0)
				options->set_error(1, (const ZString)gettext("Unexpected end of file '")+filename+"'");
			break;
		}
		read_buf_count = fread(read_buf, 1, ZFILE_RB_SIZE, fs);
		if(ferror(fs))
		{
			options->set_file_error(1, filename);
			break;
		}
	}
	if(options->verbose >= 8)
	{
		printf("SFile::read(uint8_t*) file '%s' read %u bytes. buffer: %u %s\n",
				filename.get(), (uint32_t)len1, (uint32_t)read_buf_count,
				success? "OK": (options->get_errno()? "error": "EOF"));
	}
	return success;
}

bool SFile::read(ZString &buf, size_t &len)
{
	assert(mode==read_only);
	size_t len1 = len;
	bool success = false;
	for(;;){
		if(read_buf_count!=0)
		{
			if(len<read_buf_count)
			{// all bytes loading
				buf.append(read_buf, len);
				memmove(read_buf, read_buf+len, read_buf_count-len);
				read_buf_count -= len;
				len = 0;
				success = true;
				break;
			}
			buf.append(read_buf, read_buf_count);
			len -= read_buf_count;
			read_buf_count = 0;
		}
		if(feof(fs))
		{// EOF
			break;
		}
		read_buf_count = fread(read_buf, 1, ZFILE_RB_SIZE, fs);
		if(ferror(fs))
		{
			options->set_file_error(1, filename);
			break;
		}
	}
	if(options->verbose >= 8)
	{
		printf("SFile::read(ZString&) file '%s' read %u/%u bytes. buffer: %u %s\n",
				filename.get(), (uint32_t)(len1-len), (uint32_t)len1, (uint32_t)read_buf_count,
				success? "OK": (options->get_errno()? "error": "EOF"));
	}
	len = len1-len;
	return success;
}

bool SFile::write(const uint8_t *buf, size_t len)
{
	assert(mode==write_only);
	size_t count = fwrite(buf, 1, len, fs);
	if(count!=len)
	{
		options->set_file_error(1, filename);
		return false;
	}
	if(options->verbose >= 8)
	{
		printf("SFile::write(uint8_t*) OK. filename='%s', writing %u bytes [",
				filename.get(), (uint32_t)len);
		int j;
		int l = len<8? len: 8;
		for(j=0; j<l; j++)
			printf("%02X%s", buf[j], j==l-1? "]\n": " ");
	}
	return true;
}

const dirent *SFile::read()
{
	assert(mode == read_dir);
	const dirent *de = readdir(dir);
	if(!de && errno)
	{// error
		options->set_file_error(1, filename);
		return NULL;
	}
	return de;
}

const ZString SFile::cctime(uint32_t t)
{
	time_t tt(t);
	ZString s(t == ~0u? gettext("undefined time\n"): ctime(&tt));
	return s;
}

bool SFile::stat(ZNode &znode)
{
	if(options->verbose >= 9)
		printf("SFile::stat() stat file '%s'\n", filename.get());
#	if 0
struct stat {
		dev_t     st_dev;     /* ID of device containing file */
		ino_t     st_ino;     /* inode number */
		mode_t    st_mode;    /* protection */
		nlink_t   st_nlink;   /* number of hard links */
		uid_t     st_uid;     /* user ID of owner */
		gid_t     st_gid;     /* group ID of owner */
		dev_t     st_rdev;    /* device ID (if special file) */
		off_t     st_size;    /* total size, in bytes */
		blksize_t st_blksize; /* blocksize for file system I/O */
		blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
		time_t    st_atime;   /* time of last access */
		time_t    st_mtime;   /* time of last modification */
		time_t    st_ctime;   /* time of last status change */
	};
#	endif
	struct stat sb;
	if(::stat(filename.get(), &sb) == -1)
	{
		options->set_file_error(1, filename, "Can't stat this file");
		return false;
	}
	if((sb.st_mode & S_IFMT) != S_IFREG)
	{
		options->set_file_error(1, filename, "Not regular file");
		return false;
	}
	znode.mode = sb.st_mode;
	memset(znode.owner, 0, MD5_DIGEST_LENGTH);
	memset(znode.group, 0, MD5_DIGEST_LENGTH);
	znode.size_in_bytes = sb.st_size;
	znode.atime = sb.st_atime;
	znode.ctime = sb.st_ctime;
	znode.mtime = sb.st_mtime;
	znode.dtime = ~0u;// undefined
	znode.links_count = 1;
	znode.flags = 0;
	znode.n_round = 0;
	znode.redundancy_k = 0;
	znode.redundancy_n = 0;
	znode.reserved1 = 0;
	znode.reserved2 = 0;
	memset(znode.md5, 0, MD5_DIGEST_LENGTH);
	if(options->verbose >= 9)
	{
		printf("file:\t'%s'\n", filename.get());
		printf("size:\t%llud\n", (long long unsigned)znode.size_in_bytes);
		printf("atime:\t%s", SFile::cctime(znode.atime).get());
		printf("ctime:\t%s", SFile::cctime(znode.ctime).get());
		printf("mtime:\t%s", SFile::cctime(znode.mtime).get());
		printf("dtime:\t%s", SFile::cctime(znode.dtime).get());
	}
	return true;
}

SFile::SFile(const SFile &f) : filename(f.filename), mode(f.mode)
{}

const SFile &SFile::operator=(const SFile &y)
{
	if(this==&y)	return *this;
	close();
	mode = y.mode;
	y.mode = closed;
	fs = y.fs;
	dir = y.dir;
	return *this;
}

SFile::SFile(const ZString &fn) : filename(fn), mode(closed), read_buf_count(0)
{}

SFile::SFile() : mode(closed), read_buf_count(0)
{}

void SFile::set_name(const ZString &fn)
{
	if(mode!=closed)
		close();
	filename = fn;
	read_buf_count = 0;
}

bool SFile::close()
{
	if(mode != closed)
	{
		if(options->verbose >= 8)
			printf("SFile::close() close file '%s' (%s)\n",
					filename.get(),
					mode == read_dir? "dir":
						(mode == read_only? "read only": "write only"));
		if(mode==read_dir? closedir(dir): fclose(fs))
		{
			options->set_file_error(1, filename);
			mode = closed;
			return false;
		}
		mode = closed;
	}
	return true;
}

SFile::~SFile()
{
	close();
}

void ZFile::print_buf(const uint32_t *buf, size_t len) const
{
	char s1[200];
	char s2[200];
	char *p1 = s1;
	char *p2 = s2;
	size_t j, k;
	for(j = 0; j < len; j++)
	{
		uint32_t x = buf[j];
		p1 += sprintf(p1, "%08X ", x);
		for(k = 0; k < 4; k++, x>>=8)
			*p2++ = (x&0xFF)<32? '.' : x&0xFF;
	}
	*p2 = '\0';
	printf("%s\t%s\n", s1, s2);
}

ZFile::ZFile(const ZString &fn) : orig_file(fn), code_n(0), code_k(0), blocks_md5(NULL), dirent(NULL)
{
}


bool ZFile::open_dir()
{
	return false;
}

ZFile::~ZFile()
{
	if(blocks_md5)
	{
		uint32_t j;
		for(j = 0; j < znode.n_round; j++)
			delete[] blocks_md5[j];
		delete[] blocks_md5;
		blocks_md5 = NULL;
	}
}
