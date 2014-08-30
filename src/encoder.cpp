#include <zlib.h>
#include <openssl/md5.h>

#include "getopt.hpp"
#include "encoder.hpp"
#include "huffman.hpp"
#include "zfile.hpp"
#include "gettext.h"


int Encoder::enc_count = 0;
const Matrix *Encoder::EM = NULL;


void Encoder::decode_start_m(const Matrix &E)
{
	int j, k, l;
	Matrix B(code_k);
	for(j = 0; j < code_k; j++)
	{
		for(k = l = 0; k < code_n; k++)
		{
			if(E[k]!=0)	continue;// error block
			B.set_elem(j, l++, EM->get_elem(j, k));
		}
		assert(l == code_k);
	}
	A = ~B;
	if(options->verbose >=8)
	{
		printf("Inverting matrix A:\n");
		A.print();
	}
	// printf("B:\n"); B.print();
	// B *= A;
	// printf("B*A:\n"); B.print();
}

bool Encoder::encode_part(const ZString &is, uint32_t part_num, uint32_t is_end)
{
	const Huffman input(is);
	Matrix D(1, code_k);
	Matrix C(1, code_n);
	int j;
	// clear output data and headers
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, input.get_raw_data(), input.get_len());
	for(j = 0; j < code_n; j++)
	{
		ovs[j].reset();
		ovs_headers[j].prg_tag = 0x7A7AU;
		ovs_headers[j].version = options->version_hi<<8;
		ovs_headers[j].flags.end_part = is_end;
		ovs_headers[j].flags.flags_reserved = 0;
		memset(ovs_headers[j].sha1, 0, SHA_DIGEST_LENGTH);
		ovs_headers[j].reserved7 = 0;
		ovs_headers[j].part_num = part_num;
		ovs_headers[j].rsd_size = 0;
		ovs_headers[j].crc32 = crc;
		ovs_headers[j].iv_size = input.get_len();
		ovs_headers[j].ov_size = 0;
		ovs_headers[j].blk_sub_num = j;
		ovs_headers[j].code_n = code_n;
		ovs_headers[j].code_k = code_k;
		ovs_headers[j].reserved13 = 0;
	}
	RSD rsd(0);
	do{// encode data
		for(j=0; j<code_k; j++)
		{
			// decode input stream
			if(rsd!=-1)	rsd = input.decode();
			D[j] = rsd!=-1? rsd: 0;
		}
		// matix encoding
		if(options->verbose>=9)
		{
			printf(gettext("Encoder::encode_part() source RSD: "));
			D.print();
		}
		C = D * *EM;
		if(options->verbose>=9)
		{
			printf(gettext("Encoder::encode_part() encode RSD: "));
			C.print();
		}
		// save output vector-row
		for(j=0; j<code_n; j++)
		{
			ovs[j].encode(C[j]);
			ovs_headers[j].rsd_size++;
		}
	}while(rsd!=-1);
	for(j=0; j<code_n; j++)
	{// end encode output rsd digit
		ovs[j].encode(-1);
		// save output lenght
		ovs_headers[j].ov_size = ovs[j].get_len();
		// calculate SHA1
		SHA_CTX c;
		ZString sha1;
		if(!sha1_init(&c))										return false;
		if(!sha1_update(&c, &ovs_headers[j], sizeof(Header)))	return false;
		if(!sha1_update(&c, ovs[j]))							return false;
		if(!sha1_final(&c, sha1))								return false;
		memcpy(ovs_headers[j].sha1, sha1.get_raw_data(), SHA_DIGEST_LENGTH);
		// save minor version
		ovs_headers[j].version |= options->version_low;
	}
	if(options->verbose>=8)
	{
		printf("Encoder::encode_part() part #%u encode. CRC32 %08lX, iv_size %u, %s\n",
				part_num, crc, ovs_headers[0].iv_size, is_end? "end part": "");
	}
	return true;
}

bool Encoder::encode_file(const ZString &fn)
{
	int j, k;
	if(options->verbose >=4)
		printf("Encoder::encode_file() input file: %s\n", fn.get());
	ZString input;

	//TODO desc_file
	ZString desc_name(options->repository+"/"+SFile::basename(fn)+"."+options->extension);
	SFile desc_file(desc_name);
	if(!desc_file.open(SFile::write_only))	return false;

	// init SHA1 file
	SHA_CTX sha1_file;
	if(!sha1_init(&sha1_file))	return false;

	// open inpfile
	SFile inpfile(fn);
	if(!inpfile.open(SFile::read_only))	return false;

	// encode
	size_t bs_sub = sizeof(Header)+code_n*SHA_DIGEST_LENGTH;
	if(options->block_size < bs_sub+BLK_SIZE_ADD)
	{
		options->set_error(1,
				(const ZString)gettext("Error block_size<")+
				ZString::convert_from_int(bs_sub+BLK_SIZE_ADD));
		return false;
	}
	size_t len = (options->block_size-bs_sub)*code_k;
	bool is_not_end;// true if not end part
	uint32_t part_num = 0;// part count
	do{
		// read input part
		input.reset();
		is_not_end = inpfile.read(input, len);
		if(options->get_errno())															return false;// read() error

		// encode part
		if(!encode_part(input, part_num, !is_not_end))										return false;// encode_block() error

		// save blocks
		for(j=0; j<code_n; j++)
		{
			if(part_num==0)
			{// write desc_file for first part
				if(!desc_file.write(ovs_headers[j].sha1, SHA_DIGEST_LENGTH))				return false;
			}
			else
			{// write sha1 to last part
				//static int ccc = 0;
				for(k=0; k<code_n; k++)
				{
					if(!last_block_files[k].write(ovs_headers[j].sha1, SHA_DIGEST_LENGTH))	return false;
					//printf("%d %d %d\n", j, k, ccc++);
				}
				// and close files if last block
				if(j==code_n-1)
					for(k=0; k<code_n; k++)
						if(!last_block_files[k].close())									return false;
			}

			// open block file
			ZString output_filename(options->repository+"/"+
					ZString::convert_from_uints(ovs_headers[j].sha1, SHA_DIGEST_LENGTH)+"."+
					options->extension);
			current_block_files[j].set_name(output_filename);
			if(!current_block_files[j].open(SFile::write_only))								return false;

			// write header
			if(!current_block_files[j].write((const uint8_t*)&ovs_headers[j], sizeof(Header)))	return false;

			// write data
			if(!current_block_files[j].write(ovs[j].get_raw_data(), ovs[j].get_len()))		return false;

			// update sha1_file
			if(!sha1_update(&sha1_file, ovs_headers[j].sha1, SHA_DIGEST_LENGTH))			return false;
		}

		if(!is_not_end)
		{// end part; write sha1_file to end blocks
			ZString sha1;
			if(!sha1_final(&sha1_file, sha1))												return false;
			for(j=0; j<code_n; j++)
			{
				if(!current_block_files[j].write(sha1.get_raw_data(), SHA_DIGEST_LENGTH))	return false;
				current_block_files[j].close();
			}
			if(options->verbose>=6)
			{
				printf("------------------- file %s %s encoded. All right.\n",
						ZString::convert_from_uints(sha1.get_raw_data(), SHA_DIGEST_LENGTH).get(),
						fn.get());
			}
			return true;// All right, exit
		}
		else
		{// not end part; swap last_block_files and current_block_files
			SFile *tmp = last_block_files;
			last_block_files = current_block_files;
			current_block_files = tmp;
		}
		part_num++;
	}while(true);
	return false;//no way
}

bool Encoder::sha1_init(SHA_CTX *c) const
{
	if(!SHA1_Init(c))
	{
		options->set_error(1, gettext("Can't init SHA1"));
		return false;
	}
	return true;
}

bool Encoder::sha1_update(SHA_CTX *c, const void *buf, size_t len) const
{
	if(!SHA1_Update(c, buf, len))
	{
		options->set_error(1, gettext("Can't update SHA1"));
		return false;
	}
	return true;
}

bool Encoder::sha1_update(SHA_CTX *c, const ZString &buf) const
{
	if(!SHA1_Update(c, buf.get_raw_data(), buf.get_len()))
	{
		options->set_error(1, gettext("Can't update SHA1"));
		return false;
	}
	return true;
}

bool Encoder::sha1_final(SHA_CTX *c, ZString &buf) const
{
	uint8_t bbuf[SHA_DIGEST_LENGTH];
	if(!SHA1_Final(bbuf, c))
	{
		options->set_error(1, gettext("Can't final SHA1"));
		return false;
	}
	buf.reset();
	buf.append(bbuf, SHA_DIGEST_LENGTH);
	return true;
}

bool Encoder::decode_block(const Matrix &E, Huffman &output, uint32_t iv_size)
{
	if(options->verbose>=6)
	{
		printf(gettext("Error vector: "));
		E.print();
	}
	decode_start_m(E);
	int j, k;
	Matrix C(1, code_k);// encode rsd digits
	Matrix D(1, code_k);// decode rsd digits
	// printf("iv_size: %u\n", iv_size);
	while(iv_size>output.get_len())
	{
		for(j=k=0; j<code_n; j++)
		{// load input vector
			if(E[j]!=0)	continue;// error block
			RSD c = ovs[j].decode();
			C[k++] = c==-1? 0: c;
		}
		if(options->verbose>=9)
		{
			printf(gettext("encode data: "));
			C.print();
		}
		D = C * A;
		if(options->verbose>=9)
		{
			printf(gettext("decode data: "));
			D.print();
		}
		for(j=0; j<code_k; j++)
		{// save output vector
			output.encode(D[j]);
		}
	}
	output.encode(-1);// end encode
	return true;
}

bool Encoder::decode_file(const ZString &fn)
{
	int j;
	ZString buf;
	size_t len;

	Matrix E(1, code_n); // errors vector

	if(options->verbose >=4)
		printf("Encoder::decode_file(%s)\n", fn.get());

	// open output file
	ZString output_filename(options->target+"/"+SFile::basename(fn));
	SFile output_file(output_filename);
	if(!output_file.open(SFile::write_only))	return false;

	// open desc_file
	ZString desc_name(options->repository+"/"+SFile::basename(fn)+"."+options->extension);
	SFile desc_file(desc_name);
	if(!desc_file.open(SFile::read_only))	return false;

	// load sha1 for first blocks
	len = code_n*SHA_DIGEST_LENGTH;
	if(desc_file.read(buf, len) || options->get_errno())
	{// error load desc_file
		if(!options->get_errno())
			options->set_error(1, (const ZString)gettext("Unexpected end of file '")+desc_name+"'");
		return false;
	}
	desc_file.close();
	for(j=0; j<code_n; j++)
		memcpy(
				ovs_headers[j].sha1,
				buf.get_raw_data()+j*SHA_DIGEST_LENGTH,
				SHA_DIGEST_LENGTH
				);
	buf.reset();

	// decode file
	uint32_t part_num;
	Huffman output;
	bool is_not_end = true;
	for(part_num=0; is_not_end; part_num++)
	{// main loop

		if(!load_blocks(part_num, E))	return false;

		// find good block
		for(j=0; E[j]==1; j++);

		is_not_end = !ovs_headers[j].flags.end_part;
		uint32_t iv_size = ovs_headers[j].iv_size;// size output part
		uint32_t part_crc32 = ovs_headers[j].crc32; // crc32 output part

		// decode part
		output.reset();
		if(!decode_block(E, output, iv_size))	return false;

		// compare crc32
		uint32_t crc = crc32(0L, Z_NULL, 0);
		crc = crc32(crc, output.get_raw_data(), iv_size);
		if(crc!=part_crc32)
		{// crc32 from loading block != crc32 from block header
			options->set_error(1,
					(const ZString)gettext("Error CRC32 in part #")+
					ZString::convert_from_int(part_num)+" "+
					ZString::convert_from_int(crc, ZString::CF_HEX|ZString::CF_L8)+"/"+
					ZString::convert_from_int(part_crc32, ZString::CF_HEX|ZString::CF_L8));
			return false;
		}
		if(options->verbose>=5)
			printf("Encoder::decode_file() Decode part #%u OK, CRC32: %08X\n",
					part_num, crc);
		// save decode data
		if(!output_file.write(output.get_raw_data(), iv_size))
			return false;
	}
	if(options->verbose>=1)
	{
		printf("Encoder::decode_file() Decode file %s %s OK\n",
				output_filename.get(),
				ZString::convert_from_uints(ovs_headers[j].sha1, SHA_DIGEST_LENGTH).get());
	}
	return true;
}

bool Encoder::load_blocks(uint32_t part_num, Matrix& E)
{
	int j;
	int blk_count=0;// non error blocks

	size_t len;

	for(j=0; j<code_n; j++)	E[j]=1; //all blocks error

	ZString block_name;

	for(j=0; j<code_n; j++)
	{// load block j
		// save sha1
		uint8_t sha1_buf[SHA_DIGEST_LENGTH];
		memcpy(sha1_buf, ovs_headers[j].sha1, SHA_DIGEST_LENGTH);

		// open block
		block_name = options->repository+"/"+
			ZString::convert_from_uints(sha1_buf, SHA_DIGEST_LENGTH)+"."+options->extension;
		if(options->verbose >=6)	printf("Encoder::load_blocks() Loading block %s\n", block_name.get());
		SFile block_file(block_name);
		if(!block_file.open(SFile::read_only))
		{// error block (not open)
			options->print_error();
			options->reset_error();
			continue;
		}

		// load header
		if(!block_file.read((uint8_t*)&ovs_headers[j], sizeof(Header)))	return false;

		// compare sha1 from filename vs sha1 from header
		if(memcmp(sha1_buf, ovs_headers[j].sha1, SHA_DIGEST_LENGTH)!=0)
		{
			options->set_error(1, (const ZString)gettext("Error name SHA1 in block ")+block_name);
			return false;
		}

		// load data
		ovs[j].reset();
		len = ovs_headers[j].ov_size;// load ov_size bytes
		if(!block_file.read(ovs[j], len))
		{// length error or read error
			if(!options->get_errno())
			{// unexpected end of stream
				options->set_error(1, (const ZString)gettext("Unexpected end of file '")+block_name+"'");
			}
			return false;
		}

		// control sha1 for load data
		ZString sha1;
		SHA_CTX c;
		if(!sha1_init(&c))										return false;
		memcpy(sha1_buf, ovs_headers[j].sha1, SHA_DIGEST_LENGTH);
		memset(ovs_headers[j].sha1, 0, SHA_DIGEST_LENGTH);
		uint16_t vl = ovs_headers[j].version&0xFF;
		ovs_headers[j].version &= 0xFF00;
		if(!sha1_update(&c, &ovs_headers[j], sizeof(Header)))	return false;
		if(!sha1_update(&c, ovs[j]))							return false;
		if(!sha1_final(&c, sha1))								return false;
		if(memcmp(sha1_buf, sha1.get_raw_data(), SHA_DIGEST_LENGTH)!=0)
		{
			options->set_error(1, (const ZString)gettext("Error SHA1 in block ")+block_name);
			return false;//FIXME or continue?
		}

		// version control
		if((ovs_headers[j].version>>8)>options->version_hi)
		{
			options->set_error(1, (const ZString)gettext("Expected newer version of ")+
					options->myname+gettext(" in block ")+block_name);
			return false;
		}

		// restory sha1 and version_low
		memcpy(ovs_headers[j].sha1, sha1_buf, SHA_DIGEST_LENGTH);
		ovs_headers[j].version |= vl;

		// block no errors
		E[j] = 0;
		blk_count++;
		ovs[j].set_last_pair_ic(ovs_headers[j].rsd_size%2);
		if(options->verbose >=5)
			printf("Encoder::load_blocks() Block #%d %s loading OK\n", j,
					ZString::convert_from_uints(ovs_headers[j].sha1, SHA_DIGEST_LENGTH).get());

		if(blk_count>=code_k)
		{// all blocks collected, read next sha1 from this block_file
			int k;
			for(k=0; k<code_n; k++)
			{
				block_file.read(ovs_headers[k].sha1, SHA_DIGEST_LENGTH);
				if(options->get_errno())	return false;
				if(ovs_headers[j].flags.end_part)	break;
			}
			// success
			if(options->verbose >=5)
				printf("Encoder::load_blocks() Load %u of %u blocks\n", blk_count, code_n);
			return true;
		}
	}// for(j=0; j<code_n; j++)
	// fail
	// not complit collected blocks
	options->set_error(1,
			ZString(gettext("Load "))+
			ZString::convert_from_int(blk_count)+
			gettext(" block(s) of ")+
			ZString::convert_from_int(code_k));
	return false;
}

Encoder::Encoder() : A(options->code_k), code_n(options->code_n), code_k(options->code_k)
{
	ovs_headers = new Header[code_n];
	ovs = new Huffman[code_n];
	last_block_files = new SFile[code_n];
	current_block_files = new SFile[code_n];
	if(enc_count++ == 0)
	{
		EM = new Matrix(
				code_k, code_n, code_k*code_n,
				1, 1394, 25, 1274, 625, 1072, 236, 219, 304, 1278, 605, 1172, 1135, 1320, 395,
				1, 1392, 49, 1056, 1002, 1380, 133, 468, 921, 548, 361, 271, 901, 688, 780,
				1, 1389, 100, 399, 207, 728, 1114, 52, 879, 1003, 1162, 971, 83, 569, 1305,
				1, 1385, 196, 54, 643, 791, 118, 1146, 744, 776, 328, 1004, 1333, 924, 1054,
				1, 1380, 361, 136, 214, 131, 309, 1124, 1028, 54, 373, 1307, 349, 364, 79,
				1, 1379, 400, 394, 514, 912, 1346, 1060, 1184, 103, 738, 629, 11, 1179, 203,
				1, 1376, 529, 424, 41, 456, 704, 596, 282, 509, 884, 653, 370, 1283, 1269
				);
	}
}

Encoder::~Encoder()
{
	delete[] current_block_files;
	delete[] last_block_files;
	delete[] ovs;
	delete[] ovs_headers;
	if(--enc_count == 0)
	{
		//printf("Encoder::~Encoder() enc_count==0\n");
		delete EM;
	}
}
