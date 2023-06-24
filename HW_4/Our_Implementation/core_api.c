/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#define NoAvailableThread -1

typedef enum threadStatus{HALT, MEMORY, READY} ThreadStatus;

typedef struct{
	int tid;
	uint32_t cur_inst_num;
	ThreadStatus t_status;
	int mem_cycle_left;
	tcontext* context;
} ThreadInfo;

typedef struct{
	int total_cycles;
	int total_insts;
	int threads_num;
	ThreadInfo* threads;
} SIM_Manager;

// global pointers for simulators
SIM_Manager* blocked_manager;
SIM_Manager* fg_manager;

SIM_Manager* CreateSimManager(SIM_Manager* manager, int threads_num){
	manager = (SIM_Manager*)malloc(sizeof(SIM_Manager));
	manager->total_cycles = 0;
	manager->total_insts = 0;
	manager->threads_num = threads_num;
	manager->threads = (ThreadInfo*)malloc(sizeof(ThreadInfo)*threads_num);

	for(int i = 0; i<threads_num; i++){
		manager->threads[i].tid = i;
		manager->threads[i].t_status = READY;
		manager->threads[i].mem_cycle_left = 0;
		manager->threads[i].cur_inst_num = 0;
		manager->threads[i].context = (tcontext*)malloc(sizeof(tcontext)*REGS_COUNT);
		for(int j = 0; j<REGS_COUNT; j++){
			manager->threads[i].context->reg[j] = 0;
		}
	}
	return manager;
}

void DestroySimManager(SIM_Manager* manager, int threads_num){
	for(int i = 0; i<threads_num; i++){
		free(manager->threads[i].context);
	}
	free(manager->threads);
	free(manager);
}

// Check if all threads are done - HALTED
bool AllThreadsHALT(SIM_Manager* manager){
	for(int i = 0; i<manager->threads_num; i++){
		if(manager->threads[i].t_status != HALT){
			return false;
		}
	}
	return true;
}

// Update for all threads the amount of cycles left to be ready, and update status
void UpdateMemCycles(SIM_Manager* manager){
	for(int i = 0; i<manager->threads_num; i++){
		if(manager->threads[i].t_status == MEMORY){
			if(manager->threads[i].mem_cycle_left > 1){
				manager->threads[i].mem_cycle_left--;
			}
			else{
				manager->threads[i].mem_cycle_left = 0;
				manager->threads[i].t_status = READY;
			}
		}
	}
}

int GetNextAvailableThread(ThreadInfo* threads,int threads_num,int cur_thread){
	for(int i = cur_thread; i<=threads_num + cur_thread; i++){
		if(threads[i%threads_num].t_status == READY){
			return threads[i%threads_num].tid;
		}
	}
	return NoAvailableThread;
}

void CORE_BlockedMT() {
	int threads_num =  SIM_GetThreadsNum();
	blocked_manager = CreateSimManager(blocked_manager,threads_num);
	int cur_thread = 0;
	int last_thread = 0; // save last thread inorder to know whos the next thread to search for

	// cycles loop
	while(AllThreadsHALT(blocked_manager) == false){
		Instruction instruction;
		if(cur_thread == NoAvailableThread){  // cpu idle for this cycle
			blocked_manager->total_cycles++;
			UpdateMemCycles(blocked_manager);
			cur_thread = GetNextAvailableThread(blocked_manager->threads,threads_num,last_thread);
			if(cur_thread != NoAvailableThread && cur_thread != last_thread){  // add context switch penalty only if switched thread
				int SwitchCycles = SIM_GetSwitchCycles();
				while(SwitchCycles>0){
					UpdateMemCycles(blocked_manager);
					SwitchCycles--;
				}
				blocked_manager->total_cycles += SIM_GetSwitchCycles();
			}
			continue;
		}

		// otherwise cpu fetch command and run it.
		ThreadInfo* thread = &blocked_manager->threads[cur_thread];
		if(thread->t_status == READY){
			SIM_MemInstRead(thread->cur_inst_num, &instruction, cur_thread);
			thread->cur_inst_num++;
			blocked_manager->total_insts++;
			blocked_manager->total_cycles++;
			UpdateMemCycles(blocked_manager);

			int* reg = thread->context->reg;
			switch (instruction.opcode)
			{
			case CMD_NOP:
				break;

			case CMD_HALT:
				thread->t_status = HALT;
				last_thread = cur_thread;
				cur_thread = GetNextAvailableThread(blocked_manager->threads,threads_num,cur_thread);

				if(cur_thread != NoAvailableThread){ // if we find another thread to run, do contex switch
					int SwitchCycles = SIM_GetSwitchCycles();
					while(SwitchCycles>0){	// update memeory once for each cycle of context switch
						UpdateMemCycles(blocked_manager);
						SwitchCycles--;
					}
					blocked_manager->total_cycles += SIM_GetSwitchCycles();
				}
				break;

			case CMD_ADD:     // dst <- src1 + src2
				reg[instruction.dst_index] = reg[instruction.src1_index] + reg[instruction.src2_index_imm];
				break;

			case CMD_ADDI:    // dst <- src1 + imm
				reg[instruction.dst_index] = reg[instruction.src1_index]+ instruction.src2_index_imm;
				break;

			case CMD_SUB:     // dst <- src1 - src2
				reg[instruction.dst_index] = reg[instruction.src1_index]- reg[instruction.src2_index_imm];
				break;

			case CMD_SUBI:    // dst <- src1 - imm
				reg[instruction.dst_index] = reg[instruction.src1_index]- instruction.src2_index_imm;
				break;

			case CMD_LOAD:	  // dst <- Mem[src1 + src2]  (src2 may be an immediate)
				thread->t_status = MEMORY;
				thread->mem_cycle_left = SIM_GetLoadLat();
				last_thread = cur_thread;
				cur_thread = GetNextAvailableThread(blocked_manager->threads,threads_num,cur_thread);
				if(cur_thread != NoAvailableThread){ // if we find another thread to run, do contex switch
					int SwitchCycles = SIM_GetSwitchCycles();
					while(SwitchCycles>0){  // update memeory once for each cycle of context switch
						UpdateMemCycles(blocked_manager);
						SwitchCycles--;
					}
					blocked_manager->total_cycles += SIM_GetSwitchCycles();
				}

				if(instruction.isSrc2Imm){
					SIM_MemDataRead(reg[instruction.src1_index] + instruction.src2_index_imm, &reg[instruction.dst_index]);
				}
				else{
					SIM_MemDataRead(reg[instruction.src1_index] + reg[instruction.src2_index_imm], &reg[instruction.dst_index]);
				}
				break;
			
			case CMD_STORE:	  // Mem[dst + src2] <- src1  (src2 may be an immediate)
				thread->t_status = MEMORY;
				thread->mem_cycle_left = SIM_GetStoreLat();
				last_thread = cur_thread;
				cur_thread = GetNextAvailableThread(blocked_manager->threads,threads_num,cur_thread);
				if(cur_thread != NoAvailableThread){ // if we find another thread to run, do contex switch
					int SwitchCycles = SIM_GetSwitchCycles();
					while(SwitchCycles>0){  // update memeory once for each cycle of context switch
						UpdateMemCycles(blocked_manager);
						SwitchCycles--;
					}
					blocked_manager->total_cycles += SIM_GetSwitchCycles();
				}
				
				if(instruction.isSrc2Imm){
					SIM_MemDataWrite(reg[instruction.dst_index] + instruction.src2_index_imm, reg[instruction.src1_index]);
				}
				else{
					SIM_MemDataWrite(reg[instruction.dst_index] + reg[instruction.src2_index_imm], reg[instruction.src1_index]);
				}
				break;
			
			default:
				break;
			}
		}
	}
}

void CORE_FinegrainedMT() {
	int threads_num =  SIM_GetThreadsNum();
	fg_manager = CreateSimManager(fg_manager,threads_num);
	int cur_thread = 0;
	int last_thread = 0;

	while(AllThreadsHALT(fg_manager) == false){
		Instruction instruction;
		if(cur_thread == NoAvailableThread){  // cpu idle for this cycle
			fg_manager->total_cycles++;
			UpdateMemCycles(fg_manager);
			cur_thread = GetNextAvailableThread(fg_manager->threads,threads_num,(last_thread+1)%threads_num);
			continue;
		}

		// otherwise cpu fetch command and run it.
		ThreadInfo* thread = &fg_manager->threads[cur_thread];
		if(thread->t_status == READY){
			SIM_MemInstRead(thread->cur_inst_num, &instruction, cur_thread);
			thread->cur_inst_num++;
			fg_manager->total_insts++;
			fg_manager->total_cycles++;
			UpdateMemCycles(fg_manager);

			int* reg = thread->context->reg;
			switch (instruction.opcode)
			{
			case CMD_NOP:
				break;

			case CMD_HALT:
				thread->t_status = HALT;
				break;

			case CMD_ADD:     // dst <- src1 + src2
				reg[instruction.dst_index] = reg[instruction.src1_index] + reg[instruction.src2_index_imm];
				break;

			case CMD_ADDI:    // dst <- src1 + imm
				reg[instruction.dst_index] = reg[instruction.src1_index]+ instruction.src2_index_imm;
				break;

			case CMD_SUB:     // dst <- src1 - src2
				reg[instruction.dst_index] = reg[instruction.src1_index]- reg[instruction.src2_index_imm];
				break;

			case CMD_SUBI:    // dst <- src1 - imm
				reg[instruction.dst_index] = reg[instruction.src1_index]- instruction.src2_index_imm;
				break;

			case CMD_LOAD:	  // dst <- Mem[src1 + src2]  (src2 may be an immediate)
				thread->t_status = MEMORY;
				thread->mem_cycle_left = SIM_GetLoadLat();

				if(instruction.isSrc2Imm){
					SIM_MemDataRead(reg[instruction.src1_index] + instruction.src2_index_imm, &reg[instruction.dst_index]);
				}
				else{
					SIM_MemDataRead(reg[instruction.src1_index] + reg[instruction.src2_index_imm], &reg[instruction.dst_index]);
				}
				break;
			
			case CMD_STORE:	  // Mem[dst + src2] <- src1  (src2 may be an immediate)
				thread->t_status = MEMORY;
				thread->mem_cycle_left = SIM_GetStoreLat();
				
				if(instruction.isSrc2Imm){
					SIM_MemDataWrite(reg[instruction.dst_index] + instruction.src2_index_imm, reg[instruction.src1_index]);
				}
				else{
					SIM_MemDataWrite(reg[instruction.dst_index] + reg[instruction.src2_index_imm], reg[instruction.src1_index]);
				}
				break;
			
			default:
				break;
			}

			last_thread = cur_thread;
			cur_thread = GetNextAvailableThread(fg_manager->threads,threads_num,(cur_thread+1)%threads_num);
		}
	}
}

double CORE_BlockedMT_CPI(){
	double ret_cpi = blocked_manager->total_cycles/(double)blocked_manager->total_insts;
	DestroySimManager(blocked_manager, blocked_manager->threads_num);

	return ret_cpi;
}

double CORE_FinegrainedMT_CPI(){
	double ret_cpi = fg_manager->total_cycles/(double)fg_manager->total_insts;
	DestroySimManager(fg_manager, fg_manager->threads_num);

	return ret_cpi;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	if(threadid<0 || threadid>SIM_GetThreadsNum())
		return;
	int* reg = blocked_manager->threads[threadid].context->reg;
	for(int i = 0; i<REGS_COUNT; i++){
		context[threadid].reg[i] = reg[i];
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	if(threadid<0 || threadid>SIM_GetThreadsNum())
		return;
	int* reg = fg_manager->threads[threadid].context->reg;
	for(int i = 0; i<REGS_COUNT; i++){
		context[threadid].reg[i] = reg[i];
	}
}
