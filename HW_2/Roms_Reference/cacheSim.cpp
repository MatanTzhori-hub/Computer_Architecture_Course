/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Class_cache.h"
#include <math.h>
#include <algorithm>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

int main(int argc, char** argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
		L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		}
		else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		}
		else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		}
		else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		}
		else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		}
		else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		}
		else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		}
		else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		}
		else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		}
		else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	Caches caches = Caches(MemCyc, pow(2,BSize), pow(2,L1Size), pow(2,L1Assoc), WrAlloc, L1Cyc, pow(2,BSize), pow(2,L2Size), pow(2,L2Assoc), WrAlloc, L2Cyc);


	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
		
		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		if (operation == 'w') {
			caches.updateCaches(true,num);
		}
		else
		{
			caches.updateCaches(false,num);
		}

	}


	double L1MissRate = ((caches.L1.misses)/(caches.L1.access));
	double L2MissRate = ((caches.L2.misses) / (caches.L2.access));
	double avgAccTime = caches.totalTime/caches.totalAccess;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}

static int get_set_and_tag(unsigned int address, unsigned int blockSize, unsigned int numSets, int* tag)
{

	address = address / (blockSize);

	int set = address % (numSets);

	address = address / numSets;

	*tag = address;

	return set;
}

Block find_block(int tag, vector<Block> set_vector) {
	for (auto block : set_vector)
	{
		if (block.tag == tag)
		{
			return block;
		}
	}
	return Block();
}



LCache::LCache(unsigned int Bsize, unsigned int Lsize, unsigned int ways, unsigned int WrAlloc, unsigned numCycles) :
	Bsize(Bsize), Lsize(Lsize), ways(ways), WrAlloc(WrAlloc), numCycles(numCycles)
{
	numSets = Lsize / (Bsize * ways);
	sets = (vector<Block>**)malloc(sizeof(vector<Block>*) * numSets);
	for (unsigned int i = 0; i < numSets; i++)
		sets[i] = new vector<Block>;
	access = 0;
	misses = 0;

}

// Updates the specific cache that call the function by LRU

bool updateCache(LCache& cache,bool write, int address, int* removed_address, bool *isDirty)
{

	auto sets = cache.sets;
	auto WrAlloc = cache.WrAlloc;
	int tag;
	int set = get_set_and_tag(address, cache.Bsize, cache.numSets, &tag);
	auto set_vector = sets[set];

	Block req_block = find_block(tag, *set_vector);
        
        // eneter if we found the block in the given cache
	if (req_block.tag != -1)
	{
		set_vector->erase(find(set_vector->begin(), set_vector->end(), req_block));
		if (write)
		{
			req_block.dirty = true;
		}
		set_vector->insert(set_vector->begin(), req_block);
		return false;
	}

	req_block.address = address;
	req_block.tag = tag;
	req_block.dirty = false;
        
        // if WA and W operation so we need to insert new block from the lower level and mark dirty
	if (!write || (write && WrAlloc))
	{
		if (write && WrAlloc)
		{
			req_block.dirty = true;
		}

		set_vector->insert(set_vector->begin(), req_block);
	}

	// remove LRU block
	if (set_vector->size() > cache.ways)
	{
		*removed_address = set_vector->back().address;
		if (set_vector->back().dirty)
		{
			*isDirty = true;
		}
		set_vector->erase(set_vector->end());
	}
	return true;
}


// remove Block
void removeBlock(LCache& cache,int address)
{
	auto sets = cache.sets;
	int tag;
	int set = get_set_and_tag(address, cache.Bsize, cache.numSets, &tag);
	auto set_vector = sets[set];
	Block req_block = find_block(tag, *set_vector);
	if(req_block.tag != -1)
	{
		set_vector->erase(find(set_vector->begin(), set_vector->end(), req_block));
	}
}


// Updates the entire class
void Caches::updateCaches(bool write, int address)
{
	totalAccess += 1;
	totalTime += L1.numCycles;
	L1.access += 1;
	int returnAdress1 = -1;
	int returnAdress2 = -1;
	bool isDirty1;
	bool isDirty2;

        // first updates L1 cache
	if (updateCache(L1,write, address, &returnAdress1,&isDirty1))
	{
		L1.misses += 1;
		totalTime += L2.numCycles;
		L2.access += 1;
                // Updates L2 LRU if removed dirty block from L1 
		if (isDirty1)
		{

			updateCache(L2,write, returnAdress1, &returnAdress2,&isDirty2);
		}
                // if miss updates L2 cache
		if (updateCache(L2,write, address, &returnAdress2,&isDirty2))
		{
			L2.misses += 1;
			totalTime += tMemory;

		}
                // remove from L1 if evicted from L2
		if (returnAdress2 != -1)
		{
			removeBlock(L1,returnAdress2);
		}
	}
}

