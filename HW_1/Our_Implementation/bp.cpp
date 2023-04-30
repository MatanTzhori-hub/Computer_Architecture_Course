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

	SIM_stats predictor_stats;

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
				fsmState(fsmState),
				isGlobalHist(isGlobalHist),
				isGlobalTable(isGlobalTable),
				Shared(Shared),
				predictor_stats()
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
	if(this->isGlobalHist == false){
		for(unsigned int i = 0; i < this->btbSize; i++){
			if(this->BTB[i] != nullptr){
				delete(this->BTB[i]->smArray);
				delete(this->BTB[i]);
			}
		}
	}
	delete(this->BTB);
	delete(this->smArray);
}

int parsePCtoIndex(uint32_t pc, unsigned btbSize){
	// When the BTB size is 1, we get that nbits = 0, and we need to shift 32 bits.
	// This is an undefined behavior, and therefor, for that case we will return 0
	// instead (there is only 1 entry in the BTB and its the cell 0).
	if(btbSize == 1){
		return 0;
	}

	// Otherwise, we calculate the mask.
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
	if(tagSize == 0){
		return 0;
	}

	mask = mask >> (32-tagSize);
	pc = pc >> (2 + nbits);
	uint32_t tag = mask & pc;
	return tag;
}

int ParseHistorytoIndexSM(uint8_t history, unsigned historySize, int share, uint32_t pc, bool isGlobalTables){
	uint8_t mask = 0xFF >> (8 - historySize);
	
	if(isGlobalTables && share == 1){
		uint8_t pc_8b = (uint8_t)(pc >> 2);
		history = (history & mask) ^ (pc_8b & mask);
	}
	else if(isGlobalTables && share == 2){
		uint8_t pc_8b = (uint8_t)(pc >> 16);
		history = (history & mask) ^ (pc_8b & mask);
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

	if(pc == 0xd29ec){
		pc = 0xd29ec;
	}
	if(pc == 0xd4fc){
		pc = 0xd4fc;
	}
	
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
	int smIndx = ParseHistorytoIndexSM(history, predictor.historySize, predictor.Shared, pc, predictor.isGlobalTable);
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
	predictor.predictor_stats.br_num++;

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
	uint8_t history;
	if(predictor.isGlobalHist == false){
		history = entry->history;
	}
	else{
		history = predictor.globalHistory;
	}

	// Claculate index in smArray based on history
	int smIndx = ParseHistorytoIndexSM(history, predictor.historySize, predictor.Shared, pc, predictor.isGlobalTable);

	uint8_t hist_mask = 0xFF >> (8 - predictor.historySize);

	// Update both histories
	entry->history = entry->history << 1;
	entry->history = (taken == 1) ? entry->history + 1 : entry->history; 
	entry->history = entry->history & hist_mask;
	predictor.globalHistory = predictor.globalHistory << 1;
	predictor.globalHistory = (taken == 1) ? predictor.globalHistory + 1 : predictor.globalHistory; 
	predictor.globalHistory = predictor.globalHistory & hist_mask;

	uint8_t* curState;
	if(predictor.isGlobalTable == false)
		curState = &(entry->smArray[smIndx]);
	else{
		curState = &(predictor.smArray[smIndx]);
	}

	if(taken == 1 && *curState != 3) {// exist in BTB but state is NT
		(*curState)++;
	}
	else if(taken == 0 && *curState != 0){ // exist in BTB but state is T
		(*curState)--;
	}

	// Check with a metargel about the valid bit
	if(taken == 0){ // not taken
		if(pred_dst != pc + 4){
			predictor.predictor_stats.flush_num++;
		}
	}
	else{ // taken
		if(pred_dst != targetPc){
			predictor.predictor_stats.flush_num++;
		}
	}

	return;
}

void BP_GetStats(SIM_stats *curStats){
	Predictor& predictor = Predictor::getInstance(0,0,0,0,0,0,0);

	if(predictor.isGlobalHist == false && predictor.isGlobalTable == false){
		predictor.predictor_stats.size = predictor.btbSize * (1 + predictor.historySize + predictor.tagSize + 30 + 2*(int)pow(2, predictor.historySize));
	}
	else if(predictor.isGlobalHist == true && predictor.isGlobalTable == false){
		predictor.predictor_stats.size = predictor.historySize + predictor.btbSize * (1 + predictor.tagSize + 30 + 2*(int)pow(2, predictor.historySize));
	}
	else if(predictor.isGlobalHist == false && predictor.isGlobalTable == true){
		predictor.predictor_stats.size = 2*(int)pow(2, predictor.historySize) + predictor.btbSize * (1 + predictor.historySize + predictor.tagSize + 30);
	}
	else if(predictor.isGlobalHist == true && predictor.isGlobalTable == true){
		predictor.predictor_stats.size = predictor.historySize + 2*(int)pow(2, predictor.historySize) + predictor.btbSize * (1 + predictor.tagSize + 30);
	}

	*curStats = predictor.predictor_stats;

	delete(&predictor);

	return;
}

