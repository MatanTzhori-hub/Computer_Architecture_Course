// 046267 Computer Architecture - Winter 20/21 - HW #1                  
// This file should hold your implementation of the predictor simulator 

#include "bp_api.h"
#include <cmath>

struct BTBEntry
{
	uint32_t tag;
	uint32_t target;
	uint8_t  history;
	uint8_t  state;
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

	BTBEntry* BTB;
	uint8_t* smArray;	// for lshare / gshare, only relevant for global state-machine
	uint8_t  globalHistory;
	uint8_t  globalState;

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
				this->BTB = new BTBEntry[btbSize];
				this->smArray = new uint8_t[(int)pow(2,historySize)];
				this->globalHistory = 0;
				this->globalState = fsmState;
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
	return 0;
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
	int index = parsePCtoIndex(pc, predictor.btbSize);
	int tag = parsePCtoTag(pc, predictor.btbSize, predictor.tagSize);

	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

