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
    unsigned numSets;

    Cache_set* cache_sets;
	Cache_stats Stats;

    Cache(unsigned cache_size, unsigned cache_ways, unsigned cache_clocks, unsigned num_sets);
    ~Cache();
    bool isExist(unsigned set, unsigned tag, unsigned* index);
    void UpdateLRU(unsigned set, unsigned index);
    unsigned InsertToCache(uint32_t tag, uint32_t set, std::string address, int* dirty, std::string* dirty_address);
    int findVictim(uint32_t set);
    int findCacheLine(uint32_t set, uint32_t tag);
    bool removeCacheLine(uint32_t tag, uint32_t set);
    bool isSetFull(uint32_t set);
};

Cache::Cache(unsigned cache_size, unsigned cache_ways, unsigned cache_clocks, unsigned num_sets) : 
    cacheSize(cache_size), cacheWays(cache_ways), cacheCycles(cache_clocks), numSets(num_sets), Stats()
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

unsigned Cache::InsertToCache(uint32_t tag, uint32_t set, std::string address, int* dirty, std::string* dirty_address){
    Cache_set& cur_set = cache_sets[set];
    *dirty = 0;
    *dirty_address = "";

    // Case when the set is not full yet, we increase num of lines in this set
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
    // Case when the set is full, we might need to return the dirty block
    else{
        for(unsigned i = 0; i < cacheWays; i++){
            if(cur_set.cache_lines[i].valid == 1 && cur_set.priority[i] == 0){
                if(cur_set.cache_lines[i].dirty){
                    *dirty = 1;
                    *dirty_address = cur_set.cache_lines[i].address;
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

int Cache::findCacheLine(uint32_t set, uint32_t tag){
    Cache_set& cur_set = cache_sets[set];
    for(unsigned i = 0; i < cacheWays; i++){
            if(cur_set.cache_lines[i].valid == 1 && cur_set.cache_lines[i].tag == tag){
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
            cur_set.num_lines--;

            return true;
        }
    }

    return false;
}

bool Cache::isSetFull(uint32_t set){
    return cache_sets[set].num_lines == cacheWays;
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
    double calcMissRate(int i);
    double calcAvgAccTime();
};

CacheSimulator::CacheSimulator(unsigned mem_cycles, unsigned block_size, unsigned walloc_method,
            unsigned l1cache_size, unsigned l1cache_ways, unsigned l1cache_cycles,
            unsigned l2cache_size, unsigned l2cache_ways, unsigned l2cache_cycles) :
				memCycles(mem_cycles),
				blockSize(block_size),
				wallocMethod(walloc_method)
			{
				l1 = new Cache(l1cache_size, l1cache_ways, l1cache_cycles, l1cache_size / (l1cache_ways * block_size));
				l2 = new Cache(l2cache_size, l2cache_ways, l2cache_cycles, l2cache_size / (l2cache_ways * block_size));
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
    uint32_t l1_set = parseAddrtoSet(address, blockSize, l1->numSets);
    uint32_t l1_tag = parseAddrtoTag(address, blockSize, l1->numSets);
    unsigned l1_indx;
    uint32_t l2_set = parseAddrtoSet(address, blockSize, l2->numSets);
    uint32_t l2_tag = parseAddrtoTag(address, blockSize, l2->numSets);
    unsigned l2_indx;

    // Check if block is in L1
    if(l1->isExist(l1_set, l1_tag, &l1_indx)){
        l1->UpdateLRU(l1_set, l1_indx);
        l1->Stats.num_of_hits++;
    }
    else{
        // Block is not in L1, Check if it is in L2
        if(l2->isExist(l2_set, l2_tag, &l2_indx)){
            l2->UpdateLRU(l2_set, l2_indx);
            l2->Stats.num_of_hits++;

            std::string dirty_address;
            int dirty = 0;
            l1_indx = l1->InsertToCache(l1_tag, l1_set, address, &dirty, &dirty_address);
            l1->UpdateLRU(l1_set, l1_indx);

            // In case the removed block from L1 was dirty need to update LRU
            if(dirty){
                uint32_t dirty_l2_set = parseAddrtoSet(dirty_address, blockSize, l2->numSets);
                uint32_t dirty_l2_tag = parseAddrtoTag(dirty_address, blockSize, l2->numSets);
                int dirtyIndx = l2->findCacheLine(dirty_l2_set, dirty_l2_tag);
                l2->UpdateLRU(dirty_l2_set, dirtyIndx);
            }
        }
        // Block is not in L1 or L2
        else{
            // Incase the block we remove from l2 is in l1, we must remove it from l1 aswell
            if(l2->isSetFull(l2_set)){
                int temp_indx = l2->findVictim(l2_set);
                std::string temp_address = l2->cache_sets[l2_set].cache_lines[temp_indx].address;
                uint32_t temp_l1_set = parseAddrtoSet(temp_address, blockSize, l1->numSets);
                uint32_t temp_l1_tag = parseAddrtoTag(temp_address, blockSize, l1->numSets);
                l1->removeCacheLine(temp_l1_tag, temp_l1_set);
            }

            // Insert block to L2 and then to L1
            std::string dirty_address;
            int dirty = 0;
            l2_indx = l2->InsertToCache(l2_tag, l2_set, address, &dirty, &dirty_address);
            l2->UpdateLRU(l2_set, l2_indx);
            dirty = 0;
            l1_indx = l1->InsertToCache(l1_tag, l1_set, address, &dirty, &dirty_address);
            l1->UpdateLRU(l1_set, l1_indx);

            // In case the removed block from L1 was dirty need to update LRU
            if(dirty){
                uint32_t dirty_l2_set = parseAddrtoSet(dirty_address, blockSize, l2->numSets);
                uint32_t dirty_l2_tag = parseAddrtoTag(dirty_address, blockSize, l2->numSets);
                int dirtyIndx = l2->findCacheLine(dirty_l2_set, dirty_l2_tag);
                l2->UpdateLRU(dirty_l2_set, dirtyIndx);
            }
        }
        l2->Stats.num_of_access++;
    }

    l1->Stats.num_of_access++;
}

void CacheSimulator::executeWrite(std::string address){
    uint32_t l1_set = parseAddrtoSet(address, blockSize, l1->numSets);
    uint32_t l1_tag = parseAddrtoTag(address, blockSize, l1->numSets);
    unsigned l1_indx;
    uint32_t l2_set = parseAddrtoSet(address, blockSize, l2->numSets);
    uint32_t l2_tag = parseAddrtoTag(address, blockSize, l2->numSets);
    unsigned l2_indx;

    if(wallocMethod){
        // Check if block is in L1
        if(l1->isExist(l1_set, l1_tag, &l1_indx)){
            l1->UpdateLRU(l1_set, l1_indx);
            l1->Stats.num_of_hits++;
        }
        // Block is not in L1, Check if it is in L2
        else{
            if(l2->isExist(l2_set, l2_tag, &l2_indx)){
                    l2->UpdateLRU(l2_set, l2_indx);
                    l2->Stats.num_of_hits++;

                    std::string dirty_address;
                    int dirty = 0;
                    l1_indx = l1->InsertToCache(l1_tag, l1_set, address, &dirty, &dirty_address);
                    l1->UpdateLRU(l1_set, l1_indx);

                    // In case the removed block from L1 was dirty need to update LRU
                    if(dirty){
                        uint32_t dirty_l2_set = parseAddrtoSet(dirty_address, blockSize, l2->numSets);
                        uint32_t dirty_l2_tag = parseAddrtoTag(dirty_address, blockSize, l2->numSets);
                        int dirtyIndx = l2->findCacheLine(dirty_l2_set, dirty_l2_tag);
                        l2->UpdateLRU(dirty_l2_set, dirtyIndx);
                    }
            }
            // Block is not in L1 or L2
            else{
                // Incase the block we remove from l2 is in l1, we must remove it from l1 aswell
                if(l2->isSetFull(l2_set)){
                    int temp_indx = l2->findVictim(l2_set);
                    std::string temp_address = l2->cache_sets[l2_set].cache_lines[temp_indx].address;
                    uint32_t temp_l1_set = parseAddrtoSet(temp_address, blockSize, l1->numSets);
                    uint32_t temp_l1_tag = parseAddrtoTag(temp_address, blockSize, l1->numSets);
                    l1->removeCacheLine(temp_l1_tag, temp_l1_set);
                }

                // Insert block to L2 and then to L1
                std::string dirty_address;
                int dirty = 0;
                l2_indx = l2->InsertToCache(l2_tag, l2_set, address, &dirty, &dirty_address);
                l2->UpdateLRU(l2_set, l2_indx);
                dirty = 0;
                l1_indx = l1->InsertToCache(l1_tag, l1_set, address, &dirty, &dirty_address);
                l1->UpdateLRU(l1_set, l1_indx);

                // In case the removed block from L1 was dirty need to update LRU
                if(dirty){
                    uint32_t dirty_l2_set = parseAddrtoSet(dirty_address, blockSize, l2->numSets);
                    uint32_t dirty_l2_tag = parseAddrtoTag(dirty_address, blockSize, l2->numSets);
                    int dirtyIndx = l2->findCacheLine(dirty_l2_set, dirty_l2_tag);
                    l2->UpdateLRU(dirty_l2_set, dirtyIndx);
                }
            }
            l2->Stats.num_of_access++;
        }

        l1->cache_sets[l1_set].cache_lines[l1_indx].dirty = 1;
        l1->Stats.num_of_access++;
    }
    // If no walloc, we dont need to bring block to cache after updating
    else{
        if(l1->isExist(l1_set, l1_tag, &l1_indx)){
            l1->cache_sets[l1_set].cache_lines[l1_indx].dirty = 1;
            l1->UpdateLRU(l1_set, l1_indx);
            l1->Stats.num_of_hits++;
        }
        else{
            if(l2->isExist(l2_set, l2_tag, &l2_indx)){
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


double CacheSimulator::calcMissRate(int i){
    if(i == 1)
        return (1 - l1->Stats.num_of_hits/(double)l1->Stats.num_of_access);
    else
        return (1 - l2->Stats.num_of_hits/(double)l2->Stats.num_of_access);
}

double CacheSimulator::calcAvgAccTime(){
    double l1TimeAcc = l1->cacheCycles*l1->Stats.num_of_access;
    double l2TimeAcc = l2->cacheCycles*l2->Stats.num_of_access;
    double memTimeAcc = memCycles*(l2->Stats.num_of_access - l2->Stats.num_of_hits);
    double avgAccTime = (l1TimeAcc + l2TimeAcc + memTimeAcc) / l1->Stats.num_of_access;
    return avgAccTime;
}