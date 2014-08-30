#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <openssl/sha.h>

#include "rs.hpp"
#include "huffman.hpp"
#include "matrix.hpp"
#include "zfile.hpp"

// input data size in bytes
//#define IV_SIZE 28000

#define BLK_SIZE_ADD 8

class Encoder
{
	private:
		static int enc_count;
		static int encoder_matrix[];
		static const Matrix *EM;
		Matrix A;
		const int code_n;// число выходных данных с избыточностью
		const int code_k;// число входных данных без избыточности
	public:
		struct Header
		{
			uint16_t prg_tag;		// (0)	'zz'
			uint16_t version;		//		version
			struct{
				uint32_t end_part		:1;
				uint32_t flags_reserved	:31;
			} flags;				// (1)
			uint8_t sha1[SHA_DIGEST_LENGTH];// (2,3,4,5,6) SHA1 for this blocks
			uint32_t reserved7;		// (7)
			uint32_t part_num;		// (8) part number
			uint32_t rsd_size;		// (9) number output rsd digits
			uint32_t crc32;			// (10) CRC32 input data
			uint32_t iv_size;		// (11) number bytes in input
			uint32_t ov_size;		// (12) number bytes in output data
			uint8_t blk_sub_num;	// (13) block sub number(0..code_n-1)
			uint8_t code_n;
			uint8_t code_k;
			uint8_t reserved13;
		};// 14*4 = 56 bytes
	private:
		Header *ovs_headers;
		Huffman *ovs;
		SFile *last_block_files;
		SFile *current_block_files;
		// calculate and control SHA1(whith sha1) and write digest to header
		static bool ctrl_sha1(const ZString &sha1, Header &header, const ZString &data, const ZString &block_name);
		bool sha1_init(SHA_CTX *c) const;
		bool sha1_update(SHA_CTX *c, const void *buf, size_t len) const;
		bool sha1_update(SHA_CTX *c, const ZString &buf) const;
		bool sha1_final(SHA_CTX *c, ZString &buf) const;
	protected:
	public:
		int get_code_n() const { return code_n; }
		int get_code_k() const { return code_k; }
		const Matrix &encode_m(const Matrix &D);
		void decode_start_m(const Matrix &E);
		bool encode_part(const ZString &input, uint32_t part_num, uint32_t is_end);
		bool encode_file(const ZString &fn);
		bool decode_file(const ZString &fn);
		bool decode_block(const Matrix &E, Huffman &output, uint32_t iv_size);
		bool load_blocks(uint32_t part_num, Matrix& E);

		Encoder();
		~Encoder();
};

#endif
