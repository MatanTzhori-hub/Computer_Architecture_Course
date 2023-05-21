#include <string.h>

struct Cache_stats{
	unsigned num_of_access;
	unsigned num_of_hits;
};

struct Cache_line{
    std::string address;
	unsigned tag;
    unsigned dirty;
    unsigned valid;
};

struct Cache_set{
    Cache_line* cache_lines;
    unsigned* priority;
    unsigned num_lines;
};

class Cache{
    public:
	unsigned cacheSize;
    unsigned cacheWays;
    unsigned cacheCycles;

    Cache_set* cache_sets;
	Cache_stats Stats;

    Cache(unsigned cache_size, unsigned cache_ways, unsigned cache_clocks);
    ~Cache();
    bool isExist(unsigned set, unsigned tag, unsigned* index);
    void UpdateLRU(unsigned set, unsigned index);
    unsigned InsertToCache(uint32_t tag, uint32_t set, std::string address);
    int findVictim(uint32_t set);
    bool removeCacheLine(uint32_t tag, uint32_t set);
};

Cache::Cache(unsigned cache_size, unsigned cache_ways, unsigned cache_clocks) : 
    cacheSize(cache_size), cacheWays(cache_ways), cacheCycles(cache_clocks), Stats()
{
    cache_sets = new Cache_set[cache_size/cache_ways];
    for(unsigned int i = 0; i < cache_size/cache_ways; ++i) {
        cache_sets[i].cache_lines = new Cache_line[cache_ways];
        cache_sets[i].priority = new unsigned[cache_ways];
        cache_sets[i].num_lines = 0;
    }
}

Cache::~Cache(){
    for(unsigned int i = 0; i < cacheSize/cacheWays; ++i) {
        delete cache_sets[i].cache_lines;
        delete cache_sets[i].priority;
    }
    delete cache_sets;
}

bool Cache::isExist(unsigned set, unsigned tag, unsigned* index){
    for(unsigned int i=0; i<cacheWays; i++){
        if(cache_sets[set].cache_lines[i].valid && cache_sets[set].cache_lines[i].tag == tag){
            *index = i;
            return true;
        }
    }
    return false;
}

void Cache::UpdateLRU(unsigned set, unsigned index){
    Cache_set& cur_set = cache_sets[set];

    unsigned last_val = cur_set.priority[index];
    cur_set.priority[index] = cur_set.num_lines - 1;
    for(unsigned i = 0; i<cacheWays; i++){
        if(i != index &&  cur_set.cache_lines[i].valid == 1 && cur_set.priority[i] > last_val){
            cur_set.priority[i]--;
        }
    }
}

unsigned Cache::InsertToCache(uint32_t tag, uint32_t set, std::string address){
    Cache_set& cur_set = cache_sets[set];

    if(cur_set.num_lines < cacheWays){
        for(unsigned i = 0; i < cacheWays; i++){
            if(cur_set.cache_lines[i].valid == 0){
                cur_set.cache_lines[i].address = address;
                cur_set.cache_lines[i].tag = tag;
                cur_set.cache_lines[i].valid = 1;
                cur_set.cache_lines[i].dirty = 0;
                cur_set.priority[i] = cur_set.num_lines;
                cur_set.num_lines++;
                return i;
            }
        }
    }
    else{
        for(unsigned i = 0; i < cacheWays; i++){
            if(cur_set.cache_lines[i].valid == 1 && cur_set.priority[i] == 0){
                if(cur_set.cache_lines[i].dirty){
                    // Update in the lower cache
                    // Think about me
                }
                cur_set.cache_lines[i].address = address;
                cur_set.cache_lines[i].tag = tag;
                cur_set.cache_lines[i].valid = 1;
                cur_set.cache_lines[i].dirty = 0;
                return i;
            }
        }
    }
    return -1;
}

int Cache::findVictim(uint32_t set){
    Cache_set& cur_set = cache_sets[set];
    for(unsigned i = 0; i < cur_set.num_lines; i++){
            if(cur_set.cache_lines[i].valid == 1 && cur_set.priority[i] == 0){
                return i;
            }
    }
    return -1;
}

bool Cache::removeCacheLine(uint32_t tag, uint32_t set){
    Cache_set& cur_set = cache_sets[set];
    for(unsigned i = 0; i < cacheWays; i++){
        if(cur_set.cache_lines[i].valid == 1 && cur_set.cache_lines[i].tag == tag)
        {
            UpdateLRU(set, i);
            cur_set.cache_lines[i].valid = 0;
            cur_set.priority[i] = cacheWays * 2;  // for debug purposes
            cur_set.num_lines--;

            return true;
        }
    }

    return false;
}

class CacheSimulator
{
	public:
    unsigned memCycles;
	unsigned blockSize;
	bool wallocMethod;

    Cache* l1;
    Cache* l2;

	CacheSimulator(unsigned mem_cycles, unsigned block_size, unsigned walloc_method,
            unsigned l1cache_size, unsigned l1cache_ways, unsigned l1cache_cycles,
            unsigned l2cache_size, unsigned l2cache_ways, unsigned l2cache_cycles);
	~CacheSimulator();
	CacheSimulator(CacheSimulator const&)      = delete; // disable copy ctor
	void operator=(CacheSimulator const&)  = delete; // disable = operator

    static CacheSimulator& getInstance(unsigned mem_cycles, unsigned block_size, unsigned walloc_method,
            unsigned l1cache_size, unsigned l1cache_ways, unsigned l1cache_cycles,
            unsigned l2cache_size, unsigned l2cache_ways, unsigned l2cache_cycles){

	  static CacheSimulator* instance = nullptr;
      if ( instance == nullptr ) {
        instance = new CacheSimulator(mem_cycles, block_size, walloc_method, l1cache_size, l1cache_ways, 
            l1cache_cycles, l2cache_size, l2cache_ways, l2cache_cycles);
      }
      return *instance;
    }

    void executeRead(std::string address);
    void executeWrite(std::string address);
    void executeCommand(char op, std::string address);
};

CacheSimulator::CacheSimulator(unsigned mem_cycles, unsigned block_size, unsigned walloc_method,
            unsigned l1cache_size, unsigned l1cache_ways, unsigned l1cache_cycles,
            unsigned l2cache_size, unsigned l2cache_ways, unsigned l2cache_cycles) :
				memCycles(mem_cycles),
				blockSize(block_size),
				wallocMethod(walloc_method)
			{
				l1 = new Cache(l1cache_size, l1cache_ways, l1cache_cycles);
				l2 = new Cache(l2cache_size, l2cache_ways, l2cache_cycles);
			}

CacheSimulator::~CacheSimulator(){
	delete(l1);
	delete(l2);
}

uint32_t parseAddrtoSet(std::string address, unsigned blockSize, unsigned nSets){
    if(nSets == 1){
        return 0;
    }

    uint32_t addr = (uint32_t) strtol(address.c_str(), NULL, 0);
	int blockbits = (int)log2(blockSize);
    int setbits = (int)log2(nSets);
	uint32_t mask = 0xFFFFFFFF;
	mask = mask >> (32-setbits);
	addr = addr >> blockbits;
	uint32_t set = mask & addr;
	return set;
}

uint32_t parseAddrtoTag(std::string address, unsigned blockSize, unsigned nSets){
	
    uint32_t addr = (uint32_t) strtol(address.c_str(), NULL, 0);
	int blockbits = (int)log2(blockSize);
    int setbits = (int)log2(nSets);
    uint32_t tag = addr >> (blockbits + setbits);
	return tag;
}

void CacheSimulator::executeRead(std::string address){
    uint32_t l1_set = parseAddrtoSet(address, blockSize, l1->cacheSize/l1->cacheWays);
    uint32_t l1_tag = parseAddrtoTag(address, blockSize, l1->cacheSize/l1->cacheWays);
    unsigned l1_indx;
    uint32_t l2_set = parseAddrtoSet(address, blockSize, l2->cacheSize/l2->cacheWays);
    uint32_t l2_tag = parseAddrtoTag(address, blockSize, l2->cacheSize/l2->cacheWays);
    unsigned l2_indx;

    if(l1->isExist(l1_set, l1_tag, &l1_indx)){
        l1->UpdateLRU(l1_set, l1_indx);
        l1->Stats.num_of_hits++;
    }
    else{
        if(l2->isExist(l2_set, l2_tag, &l2_indx)){
            l2->UpdateLRU(l2_set, l2_indx);
            l2->Stats.num_of_hits++;

            l1_indx = l1->InsertToCache(l1_tag, l1_set, address);
            l1->UpdateLRU(l1_set, l1_indx);
        }
        else{
            // Incase the block we remove from l2 is in l1, we must remove it from l1 aswell
            int temp_indx = l2->findVictim(l2_set);
            if(temp_indx != -1){
                std::string temp_address = l2->cache_sets[l2_set].cache_lines[temp_indx].address;
                uint32_t temp_l1_set = parseAddrtoSet(temp_address, blockSize, l1->cacheSize/l1->cacheWays);
                uint32_t temp_l1_tag = parseAddrtoTag(temp_address, blockSize, l1->cacheSize/l1->cacheWays);
                bool removed = l1->removeCacheLine(temp_l1_tag, temp_l1_set);
            }

            l2_indx = l2->InsertToCache(l2_tag, l2_set, address);
            l2->UpdateLRU(l2_set, l2_indx);
            l1_indx = l1->InsertToCache(l1_tag, l1_set, address);
            l1->UpdateLRU(l1_set, l1_indx);
        }
        l2->Stats.num_of_access++;
    }

    l1->Stats.num_of_access++;
}

void CacheSimulator::executeWrite(std::string address){
    uint32_t l1_set = parseAddrtoSet(address, blockSize, l1->cacheSize/l1->cacheWays);
    uint32_t l1_tag = parseAddrtoTag(address, blockSize, l1->cacheSize/l1->cacheWays);
    unsigned l1_indx;
    uint32_t l2_set = parseAddrtoSet(address, blockSize, l2->cacheSize/l2->cacheWays);
    uint32_t l2_tag = parseAddrtoTag(address, blockSize, l2->cacheSize/l2->cacheWays);
    unsigned l2_indx;

    if(wallocMethod){
        if(l1->isExist(l1_set, l1_tag, &l1_indx)){
            l1->UpdateLRU(l1_set, l1_indx);
            l1->Stats.num_of_hits++;
        }
        else{
            if(l2->isExist(l2_set, l2_tag, &l2_indx)){
                l2->UpdateLRU(l2_set, l2_indx);
                l2->Stats.num_of_hits++;

                l1_indx = l1->InsertToCache(l1_tag, l1_set, address);
                l1->UpdateLRU(l1_set, l1_indx);
            }
            else{
                // Incase the block we remove from l2 is in l1, we must remove it from l1 aswell
                int temp_indx = l2->findVictim(l2_set);
                if(temp_indx != -1){
                    std::string temp_address = l2->cache_sets[l2_set].cache_lines[temp_indx].address;
                    uint32_t temp_l1_set = parseAddrtoSet(temp_address, blockSize, l1->cacheSize/l1->cacheWays);
                    uint32_t temp_l1_tag = parseAddrtoTag(temp_address, blockSize, l1->cacheSize/l1->cacheWays);
                    l1->removeCacheLine(temp_l1_tag, temp_l1_set);
                }

                l2_indx = l2->InsertToCache(l2_tag, l2_set, address);
                l2->UpdateLRU(l2_set, l2_indx);
                l1_indx = l1->InsertToCache(l1_tag, l1_set, address);
                l1->UpdateLRU(l1_set, l1_indx);
            }
            l2->Stats.num_of_access++;
        }

        l1->Stats.num_of_access++;
    }
    else{
        if(l1->isExist(l1_set, l1_tag, &l1_indx)){
            l1->cache_sets[l1_set].cache_lines[l1_indx].dirty = 1;
            l1->UpdateLRU(l1_set, l1_indx);
            l1->Stats.num_of_hits++;
        }
        else{
            if(l2->isExist(l2_set, l2_tag, &l2_indx)){
                l2->cache_sets[l2_set].cache_lines[l2_indx].dirty = 1;
                l2->UpdateLRU(l2_set, l2_indx);
                l2->Stats.num_of_hits++;
            }
            l2->Stats.num_of_access++;
        }
        l1->Stats.num_of_access++;
    }

    

}

void CacheSimulator::executeCommand(char op, std::string address){
    if(op == 'r'){
        executeRead(address);
        return;
    }
    else if(op == 'w'){
        executeWrite(address);
        return;
    }
    else{
        return;
    }
}