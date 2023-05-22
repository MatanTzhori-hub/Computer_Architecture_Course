
#ifndef Class_cache
#define Class_cache

#include <vector>
#include <math.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

class Block
{
public:
	int tag;
	bool dirty;
	unsigned int address;
	Block(int tag = -1, int dirty = -1,unsigned int address = -1) :tag(tag), dirty(dirty),address(address) {};
	bool operator == (const Block& block2)
	{
		if (this->tag == block2.tag)
		{
			return true;
		}
		return false;
	}
};


class LCache
{
public :

	// Members :

	unsigned int Bsize, Lsize, ways, WrAlloc, numCycles, numSets;
	double access, misses;

	vector<Block>** sets;

	// Methods :
	LCache(unsigned int Bsize = 0 , unsigned int Lsize = 0, unsigned int ways = 0,
		unsigned int WrAlloc = 0, unsigned numCycles = 0);

	~LCache()
	{
		for (unsigned int i = 0; i < numSets; i++)
			delete sets[i];
		free (sets);
	}

	
};

class Caches
{
public :

LCache L1, L2;
double tMemory , totalTime , totalAccess;


Caches(int tMemory,unsigned int Bsize1 , unsigned int Lsize1, unsigned int ways1, unsigned int WrAlloc1, unsigned numCycles1,
			unsigned int Bsize2 , unsigned int Lsize2, unsigned int ways2, unsigned int WrAlloc2, unsigned numCycles2):
				L1(LCache(Bsize1,Lsize1,ways1,WrAlloc1,numCycles1)), L2(LCache(Bsize2,Lsize2,ways2,WrAlloc2,numCycles2))
			{
				this->tMemory = tMemory;
				this->totalAccess =0;
				this->totalTime =0;
			}

void updateCaches (bool write, int address);


};

#endif //!Class_cache
