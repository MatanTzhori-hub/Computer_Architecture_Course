/**
 * @file cache.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version submission file
 * @date 2022-12-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "cache.h"


cache_line::cache_line(unsigned set_size, unsigned tag_size, unsigned cache_size, 
            unsigned cacheAssoc, unsigned block_size, cache_Type* current):
            set_size(set_size), tag_size(tag_size),cache_size(cache_size),
            cacheAssoc(cacheAssoc),block_size(block_size),current_cache(current)
{
    // line.clear();
    for(int i=0; i<pow(2,cacheAssoc); i++)
    {
        data_block* temp = new data_block(set_size,tag_size);
        line.push_back(temp);
        lru_counter.push_back(i);
    }
};

cache_line::~cache_line()
{
    line.clear();
};

/**
 * @brief assume the block inside
 * 
 * @param update_block 
 */
void cache_line::updateLRU(data_block * update_block)
{
	// uint32_t tag = update_block->tag;
	// uint32_t set = update_block->set;
	int index = get_index_in_line(update_block);
	int x = lru_counter.at(index);
	int num_of_ways = (int) pow(2,cacheAssoc);
	lru_counter.at(index) = num_of_ways - 1;
	for (int  i = 0; i < num_of_ways ; i++)
	{
		if (index != i && lru_counter.at(i) > x)
		{
			lru_counter.at(i)--;
		}
	}
	return;
}


/**
 * @brief replaces the 
 * 
 * @param address 
 * @return block* to be replaced 
 */
data_block * cache_line::replace (uint32_t address)
{
	// cout << " replace in  cache L"<<current_cache->level_num<<endl;
	data_block* temp = nullptr;
	unsigned new_tag = get_tag(address);
	// int index_to_replace = -1;
	// if we have an invalid block
	for (size_t i = 0; i < pow(2,cacheAssoc); i++)
	{
		if(line.at(i)->valid_bit==false){
			// index_to_replace = i;
			temp=line.at(i);
			// line.at(i)->valid_bit = true; // update the block to valid
			break;
		}	
	}
	// cout << temp << endl;
	//if all the blocks are valid
	if (temp == nullptr)
	{
		temp = this->least_recently_used();	

	}
	// cout<<"valid bit of temp: "<<temp->valid_bit<<endl;
	/*snoop upper level cache*/
	if(temp->valid_bit==true){
		// cout << "snooping..." << endl;
		if(current_cache->above_lvl!=nullptr)
			current_cache->snoop(temp->full_address);
		//check dirty bit
		if(temp->dirty_bit == true)
			{
				current_cache->below_lvl->write_request(temp->full_address,false);
				temp->dirty_bit = false;
			}
	}
	assert(temp != nullptr);
	temp->valid_bit=true;
	temp->tag = new_tag;
	temp->full_address = address;

	return temp;
}

unsigned cache_line::get_tag(uint32_t address)
{
	uint32_t tag =  address >> block_size >> set_size;
	return tag;
}
unsigned cache_line::get_set(uint32_t address)
{
	uint32_t set =  address >> block_size;
	set = set % (int)pow(2,set_size);
    return set;
}

unsigned cache_line::get_index_in_line(data_block *block_to_find)
{
	for (int i = 0; i < pow(2,cacheAssoc); i++)
	{
		/* code */
		if (line.at(i)->tag == block_to_find->tag)
			return i;
	}
    return -1;// if not found
	/*
	auto iterator = std::find(line.begin(),line.end(),block_to_find);
	int index = iterator - line.begin();
    return index;*/
}

/**
 * @brief finds the least recently used block
 * 
 * @return block* block to be replaced
 */
data_block* cache_line::least_recently_used()
{
    int index = -1;
    for (size_t i = 0; i < pow(2,cacheAssoc); i++)
    {
        /* code */
        if (lru_counter.at(i)==0)
            index = i;
    }
    if(index!=-1)
        return line.at(index);
    return nullptr;//default - not found
    
	// auto iterator = std::find(lru_counter.begin(),lru_counter.end(),0);
	// int index = iterator - lru_counter.begin();	
    // return line.at(index);
}



cache_Type::cache_Type(unsigned cache_size, bool writeAllocPolicy, unsigned BlockSize, unsigned tagSize, unsigned set_size,
			unsigned access_cycles, unsigned assoc, cache_Type * above, cache_Type * below, cache_Type * mainMem , bool is_mainMemory, int level_num):
	cache_size(cache_size), 
	write_alloc_policy(writeAllocPolicy), 
	miss_count(0),
	hit_count(0),
	access_cycles(access_cycles),
	access_count(0),
	cycles_count(0),
	above_lvl(above),
	below_lvl(below),
	mainMemory(mainMem),
	tag_Size(tagSize),
	set_size(set_size),
	block_size(BlockSize),
	assoc(assoc),
	is_main_memory(is_mainMemory),
	level_num(level_num)
	{
	// lines.clear();
		for(int i=0; i<pow(2,cache_size); i++)
		{
			cache_line* temp = new cache_line(set_size, tagSize,cache_size,assoc,BlockSize,this);
			lines.push_back(temp);
		}
	}
	cache_Type::~cache_Type()
	{
		for(int i=0; i<pow(2,cache_size); i++)
			delete lines.at(i);
	};


unsigned cache_Type::getTag(uint32_t address)
{
	uint32_t tag =  address >> block_size >> set_size;
	return tag;
}
unsigned cache_Type::getSet(uint32_t address)
{
	uint32_t set =  address >> block_size;
	set = set % (int)pow(2,set_size);
    return set;
}

/**
 * @brief finds block in the cache
 * 			
 * 
 * @param address address to look for
 * @param to_count if true - counts the access and hit/miss accordingly
 * @return block* nullptr if didn't find
 * 				the block pointer if found
 */
data_block *cache_Type::look_in_cache(uint32_t address, bool to_count)
{
	
	// gets the working set
	unsigned set_to_look = getSet(address);
	cache_line * set = lines.at(set_to_look); 
	unsigned tag_to_look = getTag(address);
	// find the block in set
	data_block* temp = nullptr;
	for (size_t i = 0; i < pow(2,assoc); i++)
	{
		if (set->line.at(i)->tag == tag_to_look && set->line.at(i)->valid_bit==true)
		{
		temp = set->line.at(i);
		break;// found the tag' and the set is valid, stop looking further
		}
			
	}

	if (to_count == true){
		access_count ++;
		cycles_count += access_cycles;
			if (temp == nullptr)
				miss_count++;
			else
				hit_count++;
	}

	return temp;
}

/**
 * @brief 
 * @version started at 19:40 19/12/22
 * @param address 
 * @return block* 
 */
data_block *cache_Type::read_request(uint32_t address, bool read_count )
{
	if (is_main_memory)
	{
		// cout << "in main memory"<< endl;
		if(read_count==true){
			this->access_count ++;
			this->cycles_count += this->access_cycles;
		}
		
		data_block * block_from_main = new data_block(set_size,tag_Size);
		// cout << "made a new block with address:"<<address<< endl;
		block_from_main->full_address = address;
		//cout << "made a new block with address:"<<address<< endl;	
		return block_from_main; 
	}

	assert(is_main_memory == false);
	// creating the new block from address
	// unsigned new_block_tag = getTag(address);
	unsigned new_block_set = getSet(address);
	// look for block in cache
	data_block * temp;
	temp = look_in_cache(address,read_count);	
	if(temp != nullptr)// HIT in current level
	{
		this->hit_count++; // FIXME: already did in look_in_cache!!
	}
	if( temp == nullptr)// MISS in current level
	{
		data_block * blockf_rom_below = this->below_lvl->read_request(address,true); // read in below level
		temp = lines.at(new_block_set)->replace(address);
		
	}
	assert(temp !=nullptr);
	if (temp!=nullptr)
	{
		lines.at(new_block_set)->updateLRU(temp);	
	}
		
    return temp;
}
void cache_Type::write_request(uint32_t address, bool write_count)
{
	// look in cache if adress exists (block exists)
	data_block * temp = look_in_cache(address,write_count);
	int set = getSet(address);
	//if true => look in cache result != nullptr 	= HIT
	if (temp !=nullptr)
	{
		//look in cache already updates.
		// updateLRU
		lines.at(set)->updateLRU(temp);
		temp->dirty_bit=true;
		return; 
	}
			
	else 	// if false => look in cache returned == nullptr 	= MISS
	{
		// if write alloc do a read_request from below
		if( write_alloc_policy == true)
		{
			// temp = this->below_lvl->read_request(address); // doesn't change the info of temp block
			temp = this->read_request(address,false);
			// temp = lines.at(set)->replace(address);
			temp->dirty_bit = true;
			return;
		}
		else if (write_alloc_policy == false)	// else - write no-alloc
		{
			this->below_lvl->write_request(address, write_count);// do write_request below
			return;
		}
		assert(1);
		
	}
			
	assert(1);	// should never reach here	
	return;
}

void cache_Type::snoop(uint32_t address)
{
	// cout << "	in Snoop.." << endl;
	if (above_lvl == nullptr) // this current level  is L1
	{
		return;
	}

	// in L2 - has above lvl
	data_block* temp = this->above_lvl->look_in_cache(address,false);
	// cout << "		looked_in_cahce in snoop "<<endl;
	if (temp!=nullptr)
	{
		
		// cout << "		temp_address is:"<<temp->full_address << endl;
		temp->valid_bit = false;
		if (temp->dirty_bit== true)
		{
			temp->dirty_bit = false;
		}
		
	}
    return ;
}


