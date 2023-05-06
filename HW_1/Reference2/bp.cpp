/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <iostream>
#include <vector>
#include <math.h>

using namespace std;

uint32_t bits_select(uint32_t origin, unsigned bits_num, unsigned offset);
void updateHistory(uint32_t* history, bool taken);
void SM_Update(unsigned* SM, bool taken);

typedef struct BTB_params
{
	uint32_t tag; // tag of the branch
	uint32_t targ_PC;
	uint32_t history; // local history 
	bool valid; // branch already formed in BTB
	std::vector<unsigned> FSM_table; // local FSM table
} BTB_params;

std::vector<BTB_params> Table;

bool is_global_hist;
bool is_global_table;
bool share_use;
unsigned table_size;
unsigned hist_size;
unsigned tag_size;
unsigned fsm_start_state;

unsigned share_offset;
uint32_t global_history;
std::vector<unsigned> global_table;

unsigned flush_counter;
unsigned branch_counter;

//*BP_init - initialize the predictor
//* all input parameters are set(by the main) as declared in the trace file
//* return 0 on success, otherwise(init failure) return < 0

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
	bool isGlobalHist, bool isGlobalTable, int Shared) {
	// Parameters initialization
	table_size = btbSize;
	hist_size = historySize;
	tag_size = tagSize;
	fsm_start_state = fsmState;
	is_global_hist = isGlobalHist;
	is_global_table = isGlobalTable;
	if (Shared == 0) {
		share_use = false;
	}
	else if (Shared == 1) {
		share_use = true;
		share_offset = 2;
	}
	else if (Shared == 2) {
		share_use = true;
		share_offset = 16;
	}
	Table = vector<BTB_params>(table_size);
	// Filling BTB rows
	for (BTB_params row : Table) {
		row.tag = 0;
		row.targ_PC = 0;
		row.history = 0;
		row.valid = false;
		row.FSM_table = vector<unsigned>(pow(2, hist_size), fsm_start_state); // FSM history for each possible local history
	}

	global_history = 0;
	global_table = std::vector<unsigned>(pow(2, hist_size), fsm_start_state);
	flush_counter = 0;
	branch_counter = 0;
	return 0;
}

/*
 * BP_predict - returns the predictor's prediction (taken / not taken) and predicted target address
 * param[in] pc - the branch instruction address
 * param[out] dst - the target address (when prediction is not taken, dst = pc + 4)
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */

bool BP_predict(uint32_t pc, uint32_t* dst) {
	// cutting out wanted parts of pc
	uint32_t btb_idx = bits_select(pc, (int)log2(table_size), 2);
	uint32_t branch_tag = bits_select(pc, tag_size, 2 + (int)log2(table_size));
	// search and prediction logic
	if ((Table[btb_idx].tag == branch_tag) && Table[btb_idx].valid) {
		uint32_t fsm_idx;
		fsm_idx = (!is_global_hist) ? Table[btb_idx].history : global_history;
		if (share_use) {
			fsm_idx = fsm_idx ^ bits_select(pc, hist_size, share_offset);
		}
		bool isTaken;
		if (!is_global_table) {
			isTaken = Table[btb_idx].FSM_table[fsm_idx] > 1;
		}
		else {
			isTaken = global_table[fsm_idx] > 1;
		}
		if (isTaken) {
			*dst = Table[btb_idx].targ_PC;
			return true;
		}
	}
	*dst = pc + 4;
	return false;
}
	/*
 * BP_update - updates the predictor with actual decision (taken / not taken)
 * param[in] pc - the branch instruction address
 * param[in] targetPc - the branch instruction target address
 * param[in] taken - the actual decision, true if taken and false if not taken
 * param[in] pred_dst - the predicted target address
 */
	void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) {
		uint32_t upd_idx = bits_select(pc, (int)log2(table_size), 2);
		uint32_t upd_tag = bits_select(pc, tag_size, 2 + (int)log2(table_size));
		Table[upd_idx].targ_PC = targetPc;

		//Upon the first time a branch is taken, Save the details of the branch in the BTB
		// or replace existing branch, if tags are different
		if (Table[upd_idx].valid==false || Table[upd_idx].tag!=upd_tag) {
			Table[upd_idx].tag = upd_tag;
			Table[upd_idx].history = 0;
			Table[upd_idx].valid = true;
			Table[upd_idx].FSM_table = vector<unsigned>(pow(2, hist_size), fsm_start_state);
		}

		uint32_t hist_permutation_idx;

		if (is_global_hist) {
			if (share_use) {
				hist_permutation_idx = bits_select(pc, hist_size, share_offset ) ^ global_history;

			}else {
				hist_permutation_idx = global_history;

			}
			updateHistory(&(global_history), taken);
		}else {
			if (share_use) {
				hist_permutation_idx = bits_select(pc, hist_size, share_offset) ^ Table[upd_idx].history;

			}
			else {
				hist_permutation_idx = Table[upd_idx].history;

			}
			updateHistory(&(Table[upd_idx].history), taken);
		}

		if (is_global_table) {
			SM_Update(&(global_table[hist_permutation_idx]), taken);
		}
		else {
			SM_Update(&(Table[upd_idx].FSM_table[hist_permutation_idx]), taken);
		}
		// flush count grows if taken but predicted not taken and vice versa
		flush_counter += (taken&&(targetPc!= pred_dst))||(!taken&&(pred_dst!=pc+4));
		branch_counter++;
		return;
	}

	/*
	 * BP_GetStats: Return the simulator stats using a pointer
	 * curStats: The returned current simulator state (only after BP_update)
	 */
		
	void BP_GetStats(SIM_stats * curStats) {
		curStats->flush_num = flush_counter;
		curStats->br_num = branch_counter; 
		
		if (is_global_hist && is_global_table) { 
				curStats->size = table_size * (tag_size + 31) + hist_size + 2 * pow(2, hist_size);
				//#entries * (valid_bit+tag_size + target_size)+(history_size + 2 * 2^history_size)
				// valid_bit=1 ,target_size=32-2=30
		}else if (is_global_hist) {
				curStats->size = table_size * (tag_size + 31 + 2 * pow(2, hist_size)) + hist_size;
				//...+history_size + table_size*2 * 2^history_size
		}else if (is_global_table) {
				curStats->size = table_size * (tag_size + 31 + hist_size) + 2 * pow(2, hist_size);
				//...+table_size*history_size + 2 * 2^history_size
		}else{	
				curStats->size = table_size * (tag_size + 31 + hist_size + 2 * pow(2, hist_size));
				//#entries * (valid_bit+tag_size + target_size+ history_size + 2 * 2^history_size)
		}
		return;
	}

	uint32_t bits_select(uint32_t origin, unsigned bits_num, unsigned offset) {
		uint32_t padded_res = origin << (32 - (bits_num + offset));
		return padded_res >> (32 - bits_num);
	}

	void updateHistory(uint32_t* history, bool taken) {
		*history = ((*history) << 1) + taken;
		*history = bits_select(*history, hist_size, 0);
	}
	void SM_Update(unsigned* SM, bool taken) {
			if (*SM < 3 && taken) {
				(*SM)++;
			}
			if (*SM > 0 && !taken) {
				(*SM)--;
			}
	}