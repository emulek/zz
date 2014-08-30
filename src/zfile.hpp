#ifndef ZFILE_HPP
#define ZFILE_HPP
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>


#include "zstring.hpp"

struct ZNode
{
	uint32_t mode;				//(0) режим
	uint8_t owner[16];			//(1) хозяин
	uint8_t group[16];			//(5) группа
	uint64_t size_in_bytes;		//(9) размер в байтах
	uint32_t atime;				//(11) время доступа
	uint32_t ctime;				//(12) время изм. inode
	uint32_t mtime;				//(13) время изм. данных
	uint32_t dtime;				//(14) время удаления
	uint32_t links_count;		//(15) число хардлинков
	uint32_t flags;				//(16) флаги
	uint32_t n_round;			//(17) число раундов кодирования
	// схема кодирования, redundancy_k данных → redundancy_n выходного кода
	uint8_t redundancy_k;		//(18) число RSD цифр данных
	uint8_t redundancy_n;		// число RSD цифр на выходе
	uint16_t reserved1;
	uint32_t reserved2;			//(19)
	uint8_t md5[16];			//(20..23) MD5
	// ... MD5 блоков данных по redundancy_n штук
};

#define ZFILE_RB_SIZE 4096

// системный файл
class SFile
{
	public:
		enum Mode { closed, read_only, write_only, read_dir };
	private:
		ZString filename;
		mutable Mode mode;
		FILE *fs;
		DIR *dir;
		uint8_t read_buf[ZFILE_RB_SIZE];
		size_t read_buf_count;
	public:
		// see basename(3) (glibc)
		static const ZString basename(const ZString &fn);
		static const ZString cctime(uint32_t t);
		static const ZString remove_tail_slash(const ZString &fn);
		bool open(Mode m);
		bool read(uint8_t *buf, size_t len);
		bool read(ZString &buf, size_t &len);
		bool write(const uint8_t *buf, size_t len);
		bool close();
		const dirent *read();
		bool stat(ZNode &znode);
		void set_name(const ZString &fn);
		const ZString &get_name() const
		{	return filename; }
		SFile();
		SFile(const SFile &f);
		const SFile &operator=(const SFile &y);
		SFile(const ZString &fn);
		virtual ~SFile();
};

class ZFile
{
	public:
	private:
		ZNode znode;
		SFile orig_file;
		int code_n;
		int code_k;
		char **blocks_md5;
		class ListNode
		{
			private:
			public:
				ZString *filename;
				uint8_t md5[16];
				ListNode *next;
		};
		ListNode *dirent;
		void print_buf(const uint32_t *buf, size_t len) const;
	public:
		bool encode_file(int cn, int ck);
		ZFile(const ZString &fn);
		bool open_dir();
		~ZFile();
};

#endif
