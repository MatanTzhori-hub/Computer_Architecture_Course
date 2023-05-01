/* 046267 Computer Architecture - Spring 2020 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <cmath>
#include <math.h>
#include <iostream>
using namespace std;


#define SNT 0
#define WNT 1
#define WT  2
#define ST  3

#define NOT_USING_SHARE 0
#define USING_SHARE_LSB 1 
#define USING_SHARE_MID 2 

#define TARGET_LENGTH 30
#define INITIAL_TAG 4294967295 //max unsigned int size to mark an uninitialized table liness

//////////////////////
//	PUBLIC
//////////////////////
unsigned int num_of_flushes;
unsigned int num_of_branches;

//--------------------------------------------- Class declarations -------------------------------------------------------
/* BTB_table Class */

	class BTB_table {
	public:
		BTB_table(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
		bool isGlobalHist, bool isGlobalTable, int Shared);
		unsigned int get_btb_size() const;

		virtual void BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) = 0;
		virtual bool BTB_predict(uint32_t pc, uint32_t* dst) = 0;
		virtual ~BTB_table();
		
		//variables
		unsigned btbSize;
		unsigned historySize;
		unsigned tagSize;
		unsigned fsmState;
		bool isGlobalHist; 
		bool isGlobalTable; 
		int Shared;
		bool last_taken;

	};

	class BTB_LocalHistory_LocalFsm : public BTB_table {
	public:
		BTB_LocalHistory_LocalFsm(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
		virtual void BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
		virtual bool BTB_predict(uint32_t pc, uint32_t* dst);
		virtual ~BTB_LocalHistory_LocalFsm();

		//variables
		unsigned int** tableLines;
		unsigned int** fsmTable;
    
	};

	class BTB_LocalHistory_GlobalFsm : public BTB_table {
	public:
		BTB_LocalHistory_GlobalFsm(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
		virtual void BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
		virtual bool BTB_predict(uint32_t pc, uint32_t* dst);
		virtual ~BTB_LocalHistory_GlobalFsm();
		
		//variables
		unsigned int** tableLines;
		unsigned int* fsmTable;

	};

	class BTB_GlobalHistory_GlobalFsm: public BTB_table {
	public:
		BTB_GlobalHistory_GlobalFsm(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
		virtual void BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
		virtual bool BTB_predict(uint32_t pc, uint32_t* dst);
		virtual ~BTB_GlobalHistory_GlobalFsm();
	
		//variables
		unsigned int** tableLines;
		unsigned int GlobalHistory; 
		unsigned int* fsmTable;

	};

	class BTB_GlobalHistory_LocalFsm : public BTB_table {
	public:
		BTB_GlobalHistory_LocalFsm(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
				bool isGlobalHist, bool isGlobalTable, int Shared);
		virtual void BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
		virtual bool BTB_predict(uint32_t pc, uint32_t* dst);
		virtual ~BTB_GlobalHistory_LocalFsm();

		//variables
		unsigned int** tableLines;
		unsigned int** fsmTable;
		unsigned int GlobalHistory;

		};

//------------------------------------------ inner functions --------------------------------------------------------------

//unsigned int Extract_Bits_From_Integer(unsigned int num, unsigned int startPosition, unsigned int endPosition)
//{
uint32_t get_bits_in_range(uint32_t num, uint32_t low, uint32_t high) {

	for (unsigned int i = 1; i <= low; i++) {
		num = num >> 1;
	}

	return num % int(pow(2,  high - low + 1));
}

//update local fsm up
unsigned int fsm_up_local(unsigned int** fsmTable, unsigned int index, unsigned int history_index){

	if(fsmTable[index][history_index] < ST)
		return fsmTable[index][history_index] + 1;
	else
		return ST;
}

//update local fsm down
unsigned int fsm_down_local(unsigned int** fsmTable, unsigned int index, unsigned int history_index){

	if(fsmTable[index][history_index] > SNT)
		return fsmTable[index][history_index] - 1;
	else
		return SNT;
}

//update global fsm up
unsigned int fsm_up_global(unsigned int* fsmTable, unsigned int history_index){

	if(fsmTable[history_index] < ST)
		return fsmTable[history_index] + 1;
	else
		return ST;
}

//update global fsm down
unsigned int fsm_down_global(unsigned int* fsmTable, unsigned int history_index){

	if(fsmTable[history_index] > SNT)
		return fsmTable[history_index] - 1;
	else
		return SNT;
}

//get xor-ed pc
unsigned int get_pc_xor(unsigned int pc, int Share, unsigned historySize){
		if (Share == NOT_USING_SHARE) 
			 return 0;
		else if (Share == USING_SHARE_LSB)
			return get_bits_in_range(pc, 2, historySize + 2 - 1);
		else //(USING_SHARE_MID)
			return get_bits_in_range(pc, 16, historySize + 16 - 1);
}

// Declare a global variable (BP pointer)
BTB_table* BTB_table;
// ------------------------------------------ BTB_table class functions ---------------------------------------------------


BTB_table::BTB_table(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
	bool isGlobalHist, bool isGlobalTable, int Shared) : btbSize(btbSize), historySize(historySize), tagSize(tagSize),
	fsmState(fsmState), isGlobalHist(isGlobalHist), isGlobalTable(isGlobalTable), Shared(Shared), last_taken(false) {
	//update globals-	
	num_of_flushes = 0;
	num_of_branches = 0;		
	}

//calculate size of table-
unsigned int BTB_table::get_btb_size() const {
	return this->btbSize + //valid bits
	this->tagSize * this->btbSize + //tag
	//log2(num_of_entries) * num_of_entries + //set
	30 * this->btbSize + //target
	this->btbSize * this->historySize * (bool)(!this->isGlobalHist) +  //local history
	this->btbSize * pow(2, this->historySize) * 2 * (bool)(!this->isGlobalTable) + //local fsm table
	this->historySize * (bool)this->isGlobalHist + //global history
	pow(2, this->historySize) * 2 * (bool)this->isGlobalTable; //global fsms
}

BTB_table::~BTB_table() {}

// ----------------------------------------- NOTICE ---------------------------------------
// IS THE INIT OF THE ARRAY LEGAL?
// ----------------------------------------------------------------------------------------


// ------------------------------------------ Local History Local FSM -----------------------------------------------------

BTB_LocalHistory_LocalFsm::BTB_LocalHistory_LocalFsm(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
	bool isGlobalHist, bool isGlobalTable, int Shared) : BTB_table(btbSize, historySize, tagSize, fsmState,
		isGlobalHist, isGlobalTable, Shared) {

	tableLines = new unsigned int* [btbSize];
	fsmTable = new unsigned int* [btbSize];

	for (unsigned int it=0; it<btbSize; it++) {
		//initiate line
		tableLines[it] = new unsigned int[3];
		for (unsigned int j = 0; j < 3; j++) {
			tableLines[it][j] = 0; //0-tag 1-target 2-history
		}
		tableLines[it][0] = INITIAL_TAG;
		//initiate fsm
		fsmTable[it] = new unsigned int[int(pow(2, historySize))];
		for (int j = 0; j < int(pow(2, historySize)); j++) {
			fsmTable[it][j] = fsmState;
		}
	}
	
}

bool BTB_LocalHistory_LocalFsm::BTB_predict(uint32_t pc, uint32_t* dst) {
	unsigned int index = get_bits_in_range(pc, 2, unsigned(log2(btbSize) + 1));  
	unsigned int tag = get_bits_in_range(pc, unsigned(log2(btbSize)) + 2, unsigned(log2(btbSize)) + 2 + tagSize - 1);
	if (tableLines[index][0] != tag) {//pc not in table
		*dst = pc + 4;
		last_taken = false;
		return false;
	}
	
	unsigned int history = tableLines[index][2];//2-history
	unsigned int state = fsmTable[index][history];
	if (state <= 1) {
		*dst = pc + 4;
		last_taken = false;
		return false;
	}
	else {
		*dst = tableLines[index][1];
		last_taken = true;
		return true;
	}
	return false;
}

void BTB_LocalHistory_LocalFsm::BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
	//first, update num of branches
	num_of_branches++;

	//update flushes
	if (taken && (targetPc != pred_dst))
		num_of_flushes++;
	else if (taken != last_taken)//wrong prediction
		num_of_flushes++;


	unsigned int index = get_bits_in_range(pc, 2, unsigned(log2(btbSize) + 1));  
	unsigned int pc_tag = get_bits_in_range(pc, unsigned(log2(btbSize)) + 2, unsigned(log2(btbSize)) + 2 + tagSize - 1);
	
	//detect new branch	
	if (tableLines[index][0] != pc_tag) {
		tableLines[index][0] = pc_tag;
		tableLines[index][1] = targetPc;
		
		if(taken)	
			tableLines[index][2] = 1;
		else
			tableLines[index][2] = 0;
		
		//reset fsms
		for (unsigned int i = 0; i < pow(2, historySize); i++) {
			fsmTable[index][i] = fsmState;
		}

		//update fsms
		if (taken == true) {
			fsmTable[index][0] = fsm_up_local(fsmTable, index, 0);
		}
		else { // taken == false 
			fsmTable[index][0] = fsm_down_local(fsmTable, index, 0);

		}
	}

	else {
		//first update target
		tableLines[index][1] = targetPc;
		if (taken) {
			fsmTable[index][tableLines[index][2]] = fsm_up_local(fsmTable, index, tableLines[index][2]);
		}
		else {
			fsmTable[index][tableLines[index][2]] = fsm_down_local(fsmTable, index, tableLines[index][2]);
		} 

		//update history
		tableLines[index][2] = (tableLines[index][2] << 1) % (int(pow(2, historySize)));
		tableLines[index][2] += ((taken == 1) ? 1 : 0);
	}

}

BTB_LocalHistory_LocalFsm::~BTB_LocalHistory_LocalFsm(){
	for (unsigned int i = 0; i < btbSize; i++) {
		delete[] tableLines[i];
		}
   delete tableLines;
	for (unsigned int i = 0; i < btbSize; i++) {
		delete[] fsmTable[i];
		}
	delete fsmTable;
}

////////////////////////////////////////

BTB_LocalHistory_GlobalFsm::BTB_LocalHistory_GlobalFsm(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared) : BTB_table(btbSize, historySize, tagSize, fsmState,
		isGlobalHist, isGlobalTable, Shared) {

	tableLines = new unsigned int* [btbSize];

	for (unsigned int i = 0; i < btbSize; i++) {
		tableLines[i] = new unsigned int[3];
		for (unsigned int j = 0; j < 3; j++) {
			tableLines[i][j] = 0;
		}
		tableLines[i][0] = INITIAL_TAG;
	}
	//one global fsm-
	fsmTable = new unsigned [(int)pow(2, historySize)];
	for (int i = 0; i < (int)pow(2, historySize); i++) {
		fsmTable[i] = fsmState;
	}
}
bool BTB_LocalHistory_GlobalFsm::BTB_predict(uint32_t pc, uint32_t* dst) {
	unsigned int index = get_bits_in_range(pc, 2, unsigned(log2(btbSize) + 1));  
	unsigned int pc_tag = get_bits_in_range(pc, unsigned(log2(btbSize)) + 2, unsigned(log2(btbSize)) + 2 + tagSize - 1);

	if (tableLines[index][0] != pc_tag) {
		*dst = pc + 4;
		last_taken = false;
		return false;
	}
	
	unsigned int history = tableLines[index][2];
	unsigned int state = fsmTable[history^get_pc_xor(pc, Shared, historySize)];
	if (state == 0 || state == 1) {
		*dst = pc + 4;
		last_taken = false;
		return false;
	}
	else {
		*dst = tableLines[index][1];//1-target
		last_taken = true;
		return true;
	}
	return false;
}

void BTB_LocalHistory_GlobalFsm::BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {

	//first update num of branches
	num_of_branches++;

	//update flushes
	if (taken && targetPc != pred_dst)
		num_of_flushes++;
	else if (taken != last_taken)
		num_of_flushes++;

	unsigned int index = get_bits_in_range(pc, 2, unsigned(log2(btbSize) + 1));  
	unsigned int pc_tag = get_bits_in_range(pc, unsigned(log2(btbSize)) + 2, unsigned(log2(btbSize)) + 2 + tagSize - 1);
	unsigned int pc_xor;
	
	pc_xor = get_pc_xor(pc, Shared, historySize);

	if (tableLines[index][0] != pc_tag) { //set new line
		tableLines[index][0] = pc_tag;
		tableLines[index][1] = targetPc;
		tableLines[index][2] = (taken == true) ? 1 : 0;
		if (taken) {
			fsmTable[0^pc_xor] = fsm_up_global(fsmTable, (0^pc_xor));
		}
		else { 
			fsmTable[0^pc_xor] = fsm_down_global(fsmTable, (0^pc_xor));
		}
	}

	else { //line exists
		//always update target
		tableLines[index][1] = targetPc;
		if (taken) {
			fsmTable[tableLines[index][2]^pc_xor] = fsm_up_global(fsmTable, (tableLines[index][2]^pc_xor));
		}
		else { 
			fsmTable[tableLines[index][2]^pc_xor] = fsm_down_global(fsmTable, (tableLines[index][2]^pc_xor));
		} 
		//update history
		tableLines[index][2] = (tableLines[index][2] * 2) % (int(pow(2, historySize)));
		tableLines[index][2] += ((taken == 1) ? 1 : 0);
	}

}

BTB_LocalHistory_GlobalFsm::~BTB_LocalHistory_GlobalFsm() {
	for (unsigned int i = 0; i < btbSize; i++) {
		delete tableLines[i];
	}
	delete tableLines;
	delete fsmTable;
}

///////////////////////////////////////////
BTB_GlobalHistory_LocalFsm::BTB_GlobalHistory_LocalFsm(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState, bool isGlobalHist, bool isGlobalTable, int Shared) : BTB_table(btbSize, historySize, tagSize, fsmState,
		isGlobalHist, isGlobalTable, Shared), GlobalHistory(0) {

	tableLines = new unsigned int* [btbSize];
	fsmTable = new unsigned* [btbSize];

	for (unsigned int it = 0; it < btbSize; it++) {
		//initialize table
		tableLines[it] = new unsigned int[2];
		for (unsigned int j = 0; j < 2; j++) {
			tableLines[it][j] = 0;
		}
		tableLines[it][0] = INITIAL_TAG;
		
		//initialize fsms
		fsmTable[it] = new unsigned[(int)pow(2, historySize)];
		for (int j = 0; j < (int)pow(2, historySize); j++) {
			fsmTable[it][j] = fsmState;
		}
	}

}

bool BTB_GlobalHistory_LocalFsm::BTB_predict(uint32_t pc, uint32_t* dst) {
	unsigned int index = get_bits_in_range(pc, 2, unsigned(log2(btbSize) + 1));  
	unsigned int pc_tag = get_bits_in_range(pc, unsigned(log2(btbSize)) + 2, unsigned(log2(btbSize)) + 2 + tagSize - 1);
	
	if (tableLines[index][0] != pc_tag) {
		*dst = pc + 4;
		last_taken = false;
		return false;
	}

	unsigned int state = fsmTable[index][GlobalHistory^get_pc_xor(pc, Shared, historySize)];
	if (state == 0 || state == 1) {
		*dst = pc + 4;
		last_taken = false;
		return false;
	}
	else {
		*dst = tableLines[index][1];
		last_taken = true;
		return true;
	}
	return false;
}

void BTB_GlobalHistory_LocalFsm::BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {

	//update num of branches
	num_of_branches++;
	//update flushes
	if (taken && targetPc != pred_dst)
		num_of_flushes++;
	else if (taken != last_taken)
		num_of_flushes++;

	unsigned int pc_lsb = get_bits_in_range(pc, 2, unsigned(log2(btbSize) + 1));  
	unsigned int pc_tag = get_bits_in_range(pc, unsigned(log2(btbSize)) + 2, unsigned(log2(btbSize)) + 2 + tagSize - 1);
	
	unsigned int pc_xor = get_pc_xor(pc, Shared, historySize);
	
	if (tableLines[pc_lsb][0] != pc_tag) { // new line
		tableLines[pc_lsb][0] = pc_tag;
		tableLines[pc_lsb][1] = targetPc;
		for (unsigned int i = 0; i < pow(2, historySize); i++) {
			fsmTable[pc_lsb][i] = fsmState;
		}
		if (taken) {
			fsmTable[pc_lsb][GlobalHistory^pc_xor] =  fsm_up_local(fsmTable,pc_lsb, (GlobalHistory^pc_xor));
		}
		else { 
			fsmTable[pc_lsb][GlobalHistory ^ pc_xor] = fsm_down_local(fsmTable,pc_lsb, (GlobalHistory^pc_xor));
		}
	}

	else { // tableLines[pc_lsb][0] == pc_tag
		tableLines[pc_lsb][1] = targetPc;
		if (taken == true) {
			fsmTable[pc_lsb][GlobalHistory ^ pc_xor] = (fsmTable[pc_lsb][GlobalHistory ^ pc_xor] == ST) ? ST : fsmTable[pc_lsb][GlobalHistory ^ pc_xor] + 1;
		}
		else { // taken == false 
			fsmTable[pc_lsb][GlobalHistory ^ pc_xor] = (fsmTable[pc_lsb][GlobalHistory ^ pc_xor] == SNT) ? SNT : fsmTable[pc_lsb][GlobalHistory ^ pc_xor] - 1;
		}
		
	}
	GlobalHistory = (GlobalHistory * 2) % (int(pow(2, historySize))) + ((taken == 1) ? 1 : 0);

}

BTB_GlobalHistory_LocalFsm::~BTB_GlobalHistory_LocalFsm() {
	for (unsigned int i = 0; i < btbSize; i++) {
		delete tableLines[i];
	}
	delete tableLines;
	for (unsigned int i = 0; i < btbSize; i++) {
		delete fsmTable[i];
	}
	delete fsmTable;
}
// ------------------------------------------ Global History Global FSM ---------------------------------------------------
BTB_GlobalHistory_GlobalFsm::BTB_GlobalHistory_GlobalFsm(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
	bool isGlobalHist, bool isGlobalTable, int Shared) : BTB_table(btbSize, historySize, tagSize, fsmState,
		isGlobalHist, isGlobalTable, Shared) , GlobalHistory(0) {
	//size_of_memory = (int)btbSize + (int)btbSize * (int)(tagSize + TARGET_LENGTH)+ (int)historySize + 2 * int(pow(2, historySize));
	tableLines = new unsigned int* [btbSize];
	for (unsigned int i = 0; i < btbSize; i++) {
		tableLines[i] = new unsigned int[2];
		for (unsigned int j = 0; j < 2; j++) {
			tableLines[i][j]= 0;
		}
		tableLines[i][0] = INITIAL_TAG;
	}
	int power = (int)pow(2, historySize);
	fsmTable = new unsigned[power];
	for (int i = 0; i < power; i++) {
		fsmTable[i] = fsmState;
	}
}
bool BTB_GlobalHistory_GlobalFsm::BTB_predict(uint32_t pc, uint32_t* dst) {
	unsigned int pc_lsb = get_bits_in_range(pc, 2, unsigned(log2(btbSize) + 1));  
	unsigned int pc_tag = get_bits_in_range(pc, unsigned(log2(btbSize)) + 2, unsigned(log2(btbSize)) + 2 + tagSize - 1);
	unsigned int pc_xor;
	if (Shared == NOT_USING_SHARE)
		pc_xor = 0;
	else if (Shared == USING_SHARE_LSB) {
		pc_xor = get_bits_in_range(pc, 2, historySize + 2 - 1);
	}
	else { // Shared == USING_SHARE_MID
		pc_xor = get_bits_in_range(pc, 16, historySize + 16 - 1);
	}
	if (tableLines[pc_lsb][0] != pc_tag) {
		*dst = pc + 4;
		last_taken = false;
		return false;
		
	}

	unsigned int state = fsmTable[GlobalHistory ^ pc_xor];
	
	if (state == 0 || state == 1) {
		*dst = pc + 4;
		last_taken = false;	
		return false;
	}
	else {
		*dst = tableLines[pc_lsb][1];
		last_taken = true;
		return true;
	}
	return false; //shouldn't get here
}

void BTB_GlobalHistory_GlobalFsm::BTB_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
	num_of_branches++;
	if (taken && targetPc != pred_dst)
		num_of_flushes++;
	else if (taken != last_taken)
		num_of_flushes++;
	unsigned int pc_lsb = get_bits_in_range(pc, 2, unsigned(log2(btbSize) + 1));
	unsigned int pc_tag = get_bits_in_range(pc, unsigned(log2(btbSize)) + 2, unsigned(log2(btbSize)) + 2 + tagSize - 1);
	unsigned int pc_xor;
	if (Shared == NOT_USING_SHARE)
		pc_xor = 0;
	else if (Shared == USING_SHARE_LSB)
		pc_xor = get_bits_in_range(pc, 2, historySize + 2 - 1);
	else { // Shared == USING_SHARE_MID
		pc_xor = get_bits_in_range(pc, 16, historySize + 16 - 1);
	}
	if (tableLines[pc_lsb][0] != pc_tag) {
		tableLines[pc_lsb][0] = pc_tag;
		tableLines[pc_lsb][1] = targetPc;
		if (taken == true) {
			fsmTable[GlobalHistory ^ pc_xor] = (fsmTable[GlobalHistory ^ pc_xor] == ST) ? ST : fsmTable[GlobalHistory ^ pc_xor] + 1;
		}
		else { // taken == false 
			fsmTable[GlobalHistory ^ pc_xor] = (fsmTable[GlobalHistory ^ pc_xor] == SNT) ? SNT : fsmTable[GlobalHistory ^ pc_xor] - 1;
		}
	}

	else { // tableLines[pc_lsb][0] == pc_tag
		tableLines[pc_lsb][1] = targetPc;
		if (taken == true) {
			fsmTable[GlobalHistory ^ pc_xor] = (fsmTable[GlobalHistory ^ pc_xor] == ST) ? ST : fsmTable[GlobalHistory ^ pc_xor] + 1;
		}
		else { // taken == false 
			fsmTable[GlobalHistory ^ pc_xor] = (fsmTable[GlobalHistory ^ pc_xor] == SNT) ? SNT : fsmTable[GlobalHistory ^ pc_xor] - 1;
		}
		
	}
	GlobalHistory = ((GlobalHistory * 2) % (int(pow(2, historySize)))) + ((taken == 1) ? 1 : 0);
} 

BTB_GlobalHistory_GlobalFsm::~BTB_GlobalHistory_GlobalFsm() {
	for (unsigned int i = 0; i < btbSize; i++) {
		delete tableLines[i];
	}
	delete tableLines;
	delete fsmTable;
}



//-------------------------------------------------------------------------------------------------------------------------


/*
	 * BP_init - initialize the predictor
	 * all input parameters are set (by the main) as declared in the trace file
	 * return 0 on success, otherwise (init failure) return <0
	 */
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
	bool isGlobalHist, bool isGlobalTable, int Shared) {
	/*if (btbSize != 1 && btbSize != 2 && btbSize != 4 && btbSize != 8 && btbSize != 16 && btbSize != 32)
		return -1;
	if(historySize < 1 || historySize > 8) 
		return -1;
	unsigned int maxTagSize = unsigned(30 - log2(btbSize));
	if (tagSize < 0 || tagSize > maxTagSize)
		return -1;
	if (fsmState < 0 || fsmState > 3)
		return -1;
	if (Shared < 0 || Shared > 2)
		return -1;*/
	//BTB_table(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	if (!isGlobalHist){
		 if (!isGlobalTable) {//local hist local table
			BTB_table = new BTB_LocalHistory_LocalFsm(btbSize, historySize, tagSize, fsmState,
				isGlobalHist, isGlobalTable, Shared);
			return 1;
		}
		else{//local hist global table
			BTB_table = new BTB_LocalHistory_GlobalFsm(btbSize, historySize, tagSize, fsmState,
			isGlobalHist, isGlobalTable, Shared);
		return 1;
		}
	}
	else{
		if(!isGlobalTable) {//global hist local table
		BTB_table = new BTB_GlobalHistory_LocalFsm(btbSize, historySize, tagSize, fsmState,
			isGlobalHist, isGlobalTable, Shared);
		return 1;
		}
		else if(isGlobalHist && isGlobalTable) {//global hist global table
			BTB_table = new BTB_GlobalHistory_GlobalFsm(btbSize, historySize, tagSize, fsmState,
				isGlobalHist, isGlobalTable, Shared);
			return 1;
		}
	}
	return -1;
}

bool BP_predict(uint32_t pc, uint32_t* dst) {

	return BTB_table->BTB_predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
	return BTB_table->BTB_update(pc,targetPc,taken,pred_dst);
}

void BP_GetStats(SIM_stats* curStats) {
	curStats->br_num = num_of_branches;
	curStats->flush_num = num_of_flushes;
	curStats->size = BTB_table->get_btb_size();
	
	delete BTB_table;
	return;
}

