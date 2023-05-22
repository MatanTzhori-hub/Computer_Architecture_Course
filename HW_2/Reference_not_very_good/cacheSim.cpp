/* 046267 Computer Architecture - Winter 20/21 - HW #2 */
/**
 * @file cacheSim.cpp
 * @brief 
 * @version Submission File
 * @date 2022-12-18
 * 
 * @copyright Copyright (c) 2022
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>
#include <cassert>
#include "cache.h"

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::vector;

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	//string filename = "example1_trace.cpp";
	char* fileString = argv[1];
	// ifstream file("examples/example1_trace.txt"); //input file stream - for test 1
	// ifstream file("examples/example2_trace.txt"); //input file stream - for test 2
	// ifstream file("examples/example3_trace.txt"); //input file stream - for test 3
	ifstream file(fileString); //input file stream - for test 1
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}
	/* Test example 1 */
	// unsigned MemCyc = 100, BSize = 3, L1Size = 4, L2Size = 6, L1Assoc = 1,
	// 		L2Assoc = 0, L1Cyc = 1, L2Cyc = 5, WrAlloc = 1;
	/* Test example 2 */
	// unsigned MemCyc = 50, BSize = 4, L1Size = 6, L2Size = 8, L1Assoc = 1,
	// 		L2Assoc = 2, L1Cyc = 2, L2Cyc = 4, WrAlloc = 1;
	/* Test example 3 */
	// unsigned MemCyc = 10, BSize = 2, L1Size = 4, L2Size = 4, L1Assoc = 1,
	// 		L2Assoc = 2, L1Cyc = 1, L2Cyc = 5, WrAlloc = 1;
	// for running several tests
	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;
	
	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	
	/* Creation of Cache Levels */
	// L1
	unsigned L1_setSize = L1Size - BSize - L1Assoc; // number of bits that saves the set
	unsigned L1_tagSize = ADDRESS_BITS  - L1_setSize - BSize ; 
	cache_Type L1 = cache_Type(L1Size,WrAlloc,BSize,L1_tagSize,L1_setSize,L1Cyc,L1Assoc,nullptr,nullptr,nullptr,false,1);
	// L2
	unsigned L2_setSize = L2Size - BSize - L2Assoc;
	unsigned L2_tagSize = ADDRESS_BITS  - L2_setSize - BSize ;
	cache_Type L2 = cache_Type(L2Size,WrAlloc,BSize,L2_tagSize,L2_setSize,L2Cyc,L2Assoc,nullptr,nullptr,nullptr,false,2);
	// Main Memory
	cache_Type main_memory = cache_Type(MAIN_SIZE,WrAlloc,BSize,0,0,MemCyc,0,nullptr,nullptr,nullptr,true,3);

	// Setting Connections between Levels
	L1.below_lvl =&L2;
	L1.mainMemory = &main_memory;
	L2.above_lvl = &L1;
	L2.below_lvl = &main_memory;
	L2.mainMemory=&main_memory;
	main_memory.above_lvl = &L2;

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		// cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		// cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		// cout << " (dec) " << num << endl;

		
		if (operation =='r')
		{
			L1.read_request(num,true);
		}
		else if (operation == 'w')
		{
			L1.write_request(num,true);
		}
	}
		
	double L1MissRate;
	double L2MissRate;
	double avgAccTime;
	//  calculations for MISS rates
	L1MissRate = (double) L1.miss_count / (double) L1.access_count;
	L2MissRate = (double) L2.miss_count / (double) L2.access_count;
	avgAccTime = (double) (L1.cycles_count / (double)(L1.access_count))
				+ (double)(L2.cycles_count / (double)L1.access_count)
				+ (double) (main_memory.cycles_count / (double) L1.access_count);// avgcycle time = each level count devided by total access(=L1.access_count) 
	
	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
