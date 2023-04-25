// 046267 Computer Architecture - Winter 20/21 - HW #1                  
// This file should hold your implementation of the predictor simulator 

#include "bp_api.h"
#include <cmath>

struct BTBEntry
{
	uint32_t tag;
	uint32_t target;
	uint8_t  history;
	uint8_t* smArray;
};

class Predictor
{
	public:
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;

	BTBEntry** BTB;
	uint8_t* smArray;	// for lshare / gshare, only relevant for global state-machine
	uint8_t  globalHistory;

	Predictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	~Predictor();
	Predictor(Predictor const&)      = delete; // disable copy ctor
	void operator=(Predictor const&)  = delete; // disable = operator

    static Predictor& getInstance( unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared ){

	  static Predictor* instance = nullptr;
      if ( instance == nullptr ) {
        instance = new Predictor(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
      }
      return *instance;
    }
};

Predictor::Predictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared) :
				btbSize(btbSize),
				historySize(historySize),
				tagSize(tagSize),
				isGlobalHist(isGlobalHist),
				isGlobalTable(isGlobalTable),
				Shared(Shared)
			{
				this->BTB = new BTBEntry*[btbSize];
				for(unsigned i = 0; i < btbSize; i++){
					this->BTB[i] = nullptr;
				}
				this->smArray = new uint8_t[(int)pow(2,historySize)];
				for(int j =0; j < (int)pow(2,historySize); j++){
					this->smArray[j] = fsmState;
				}
				this->globalHistory = 0;
			}

Predictor::~Predictor(){

}

int parsePCtoIndex(uint32_t pc, unsigned btbSize){
	int nbits = (int)log2(btbSize);
	uint32_t mask = 0xFFFFFFFF;
	mask = mask >> (32-nbits);
	pc = pc >> 2;
	uint32_t index = mask & pc;
	return index;
}

int parsePCtoTag(uint32_t pc, unsigned btbSize, unsigned tagSize){
	int nbits = (int)log2(btbSize);
	uint32_t mask = 0xFFFFFFFF;
	mask = mask >> (32-tagSize);
	pc = pc >> (2+ nbits);
	uint32_t tag = mask & pc;
	return tag;
}

int ParseHistorytoIndexSM(uint8_t history, unsigned historySize, int share, uint32_t pc){
	uint8_t mask = 0xFF >> (8 - historySize);
	if(share == 1){
		uint8_t pc_8b = (uint8_t)(pc >> 2);
		history = history ^ pc_8b;
	}
	else if(share == 2){
		uint8_t pc_8b = (uint8_t)(pc >> 16);
		history = history ^ pc_8b;
	}

	int index = history & mask;
	return index;
}

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
			
			if(btbSize != 1 && btbSize != 2 && btbSize != 4 && btbSize != 8 && btbSize != 16 && btbSize != 32){
				return -1;
			}
			if(historySize < 1 || 8 < historySize){
				return -1;
			}
			if(tagSize < 0 || (30 - (unsigned int)log2(btbSize)) < tagSize){
				return -1;
			}
			if(fsmState < 0 || 3 < fsmState){
				return -1;
			}
			
			Predictor::getInstance(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
			return 1;

	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	Predictor& predictor = Predictor::getInstance(0,0,0,0,0,0,0);
	unsigned int index = parsePCtoIndex(pc, predictor.btbSize);
	unsigned int tag = parsePCtoTag(pc, predictor.btbSize, predictor.tagSize);
	BTBEntry* entry = predictor.BTB[index];
	
	if(entry == nullptr || entry->tag != tag){ // doesn't exist in BTB or same entry with diff tags
		*dst = pc+4;
		return false;	
	}
	
	// Decide if local history or global history
	int history;
	if(predictor.isGlobalHist == false){
		history = entry->history;
	}
	else{
		history = predictor.globalHistory;
	}

	// Claculate index in smArray based on history
	int smIndx = ParseHistorytoIndexSM(history, predictor.historySize, predictor.Shared, pc);
	int curState;
	if(predictor.isGlobalTable == true){
		curState = predictor.smArray[smIndx];
	}
	else{
		curState = entry->smArray[smIndx];
	}

	if(curState == 0 || curState == 1) {// exist in BTB but state is NT
		*dst = pc+4;
		return false;
	}
	else{ // exist in BTB but state is T
		*dst = entry->target; 
		return true;
	}
		return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	Predictor& predictor = Predictor::getInstance(0,0,0,0,0,0,0);
	unsigned int index = parsePCtoIndex(pc, predictor.btbSize);
	unsigned int tag = parsePCtoTag(pc, predictor.btbSize, predictor.tagSize);
	BTBEntry* entry = predictor.BTB[index];

	if(entry == nullptr || entry->tag != tag){ // doesn't exist in BTB or same entry with diff tags
		if(entry == nullptr){
			predictor.BTB[index] = new BTBEntry();
			entry = predictor.BTB[index];
		}
		entry->tag = tag;
		entry->target = targetPc;
		entry->history = 0;
		if(predictor.isGlobalTable == false){
			if(entry->smArray == nullptr){
				entry->smArray = new uint8_t[(int)pow(2,predictor.historySize)];
			}
			for(int j =0; j < (int)pow(2,predictor.historySize); j++){
				entry->smArray[j] = predictor.fsmState;
			}
		}
		// Need to update the state machine according to the history index (depends if global or local history)
	}
	
	// Decide if local history or global history
	int history;
	if(predictor.isGlobalHist == false){
		history = entry->history;
	}
	else{
		history = predictor.globalHistory;
	}

	// Claculate index in smArray based on history
	int smIndx = ParseHistorytoIndexSM(history, predictor.historySize, predictor.Shared, pc);
	// Update both histories
	entry->history = entry->history << 1;
	entry->history = (taken == 1) ? entry->history + 1 : entry->history; 
	predictor.globalHistory = predictor.globalHistory << 1;
	predictor.globalHistory = (taken == 1) ? predictor.globalHistory + 1 : predictor.globalHistory; 

	uint8_t* curState = &entry->smArray[smIndx];
	if(predictor.isGlobalTable == true){
		curState = &predictor.smArray[smIndx];
	}

	if(taken == 1 && *curState != 3) {// exist in BTB but state is NT
		(*curState)++;
	}
	else if(taken == 0 && *curState != 0){ // exist in BTB but state is T
		(*curState)--;
	}

	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

