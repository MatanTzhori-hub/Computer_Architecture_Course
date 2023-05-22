#ifndef CACHE_CPP
#define CACHE_CPP


#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#include <cassert>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::vector;

#define BLOCK_OFFSET 2 // bumber of bits for aliignment
#define ADDRESS_BITS 32
#define MAIN_SIZE	0  //for dummy main

// class block;
class cache_Type;
// class cache_line;

class data_block
{
	public:
	/* data */
	unsigned set_size;
	unsigned tag_size;
	unsigned set; 
	unsigned tag;
	unsigned full_address;
	bool valid_bit;
	bool dirty_bit;

	/* methods */
	data_block(){};
	data_block(unsigned set_size, unsigned tag_size ):
	set_size(set_size), tag_size(tag_size), set(0),tag(0),
	full_address(0), valid_bit(false), dirty_bit(false){};
	~data_block(){};
};
class cache_line
{
public:
	/* data */
	unsigned set_size;
	unsigned tag_size;
	unsigned cache_size;
	unsigned cacheAssoc; 
	unsigned block_size;
	vector <data_block*> line;
	cache_Type * current_cache;
	vector <int> lru_counter; // array of counters for LRU algorithm 
	
	/* methods */
	cache_line(unsigned set_size, unsigned tag_size, unsigned cache_size, 
				unsigned cacheAssoc, unsigned block_size, cache_Type* current);
	~cache_line();
	void updateLRU(data_block* update_block);
	data_block* replace (uint32_t address);
	unsigned get_set(uint32_t address);
	unsigned get_tag(uint32_t address);
	unsigned get_index_in_line(data_block * block_to_find);
	data_block * least_recently_used(); // to evict him
};

class cache_Type
{
public:
	/* data */
	unsigned cache_size;
	bool write_alloc_policy;
	uint32_t miss_count;
	uint32_t hit_count;
	uint32_t access_cycles; // cycles to access this level
	uint32_t access_count; 
	uint32_t cycles_count; // total number of cycles done in this level 
	vector<cache_line *> lines;
	cache_Type * above_lvl;
	cache_Type * below_lvl;
	cache_Type * mainMemory; // maybe change pointer to main memory to a different type, if it is main_mem = nullptr
	uint32_t tag_Size;
	uint32_t set_size;
	uint32_t block_size;
	uint32_t assoc;
	bool is_main_memory;
	int  level_num;
    /* methods */
	cache_Type(unsigned cache_size, bool writeAllocPolicy, unsigned blockSize, unsigned tag_size, unsigned set_size,
			unsigned access_cycles, unsigned assoc, cache_Type * above, cache_Type * below, cache_Type * mainMem , bool is_mainMemory, int level_num);
	~cache_Type();
	unsigned getSet(uint32_t address);
	unsigned getTag(uint32_t address);
	data_block * look_in_cache(uint32_t address, bool to_count);
	data_block * read_request(uint32_t address, bool read_count);
	void 	write_request(uint32_t address, bool write_count);
	void 	snoop (uint32_t address);
};


#endif // !CACHE_CPP