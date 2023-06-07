/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#define MAX_REGS 32
#define ENTRY_ID -2
#define EXIT_ID -1

#include "dflow_calc.h"
#include <vector>

struct InstNode{
    int instNum;
    InstInfo instInfo;
    InstNode* p1;
    InstNode* p2;

    InstNode(int instNum, InstInfo instInfo, InstNode* p1, InstNode* p2):instNum(instNum), instInfo(instInfo), p1(p1), p2(p2){}
};

struct ExitNode{
    int instNum;
    std::vector<InstNode*> parents;

    ExitNode(int instNum):instNum(instNum), parents(){}
};


class DepSim{
    const InstInfo* theProg;
    unsigned int progLen;
    unsigned int opsLatency[MAX_OPS];

    std::vector<InstNode*> instGraphRef;       // Vector of pointers to instruction nodes in the dependencies graph.
    int lastInstDep[MAX_REGS];                 // Array of the latest instruction to change the index's register

    InstNode* entry;
    ExitNode* exit;

    void InsertInst(InstInfo info, int indx);  // Inserts an instruction into the dependencies graph.
    InstNode* FindInstNode(int instNum);
    void RemoveParentFromExit(int instNum);
    int getInstDepthRecur(InstNode* curNode);


    public:
        DepSim(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts);
        ~DepSim();
        int getInstDepth(int theInst);
        int getInstDeps(unsigned int theInst, int *src1DepInst, int *src2DepInst);
        int getProgDepth();

};

DepSim::DepSim(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts): theProg(progTrace), progLen(numOfInsts), opsLatency(), 
                                                    instGraphRef(), lastInstDep(){
    for(int i = 0; i < MAX_REGS; i++){
        this->opsLatency[i] = opsLatency[i];
    }
    for(int i = 0; i < MAX_OPS; i++){
        lastInstDep[i] = -1;
    }

    entry = new InstNode(ENTRY_ID, {0,0,0,0}, nullptr, nullptr);
    exit = new ExitNode(EXIT_ID);

    for(unsigned int i = 0; i < progLen; i++){
        InsertInst(theProg[i], i);
    }
}

DepSim::~DepSim(){
    for(long unsigned int i = 0; i < instGraphRef.size(); i++){
        delete instGraphRef[i];
    }
    delete entry;
    delete exit;
}

void DepSim::InsertInst(InstInfo info, int indx){
    InstNode* newNode = new InstNode(indx, info, nullptr, nullptr);
    exit->parents.push_back(newNode);

    if(lastInstDep[info.src1Idx] != -1){
        newNode->p1 = FindInstNode(lastInstDep[info.src1Idx]);
        RemoveParentFromExit(lastInstDep[info.src1Idx]);
    }
    if(lastInstDep[info.src2Idx] != -1){
        newNode->p2 = FindInstNode(lastInstDep[info.src2Idx]);
        RemoveParentFromExit(lastInstDep[info.src2Idx]);
    }
    if(newNode->p1 == nullptr && newNode->p2 == nullptr){
        newNode->p1 = entry;
    }

    instGraphRef.push_back(newNode);
    lastInstDep[info.dstIdx] = newNode->instNum;
}

InstNode* DepSim::FindInstNode(int instNum){
    for(long unsigned int i = 0; i < instGraphRef.size(); i++){
        if(instGraphRef[i]->instNum == instNum){
            return instGraphRef[i];
        }
    }
    return nullptr;
}

void DepSim::RemoveParentFromExit(int instNum){
    for(auto itr = exit->parents.begin(); itr != exit->parents.end(); itr++){
        if((*itr)->instNum == instNum){
            exit->parents.erase(itr);
            return;
        }
    }
}

int DepSim::getInstDepth(int theInst){
    if(theInst >= (int)progLen){
        return -1;
    }

    InstNode* startNode = FindInstNode(theInst);
    int res = getInstDepthRecur(startNode);
    return res - opsLatency[startNode->instInfo.opcode];
}

int DepSim::getInstDepthRecur(InstNode* curNode){
    if(curNode == nullptr || curNode->instNum == ENTRY_ID){
        return 0;
    }

    int left = getInstDepthRecur(curNode->p1);
    int right = getInstDepthRecur(curNode->p2);

    int max = (left >= right) ? left : right;
    return max += opsLatency[curNode->instInfo.opcode];
}

int DepSim::getInstDeps(unsigned int theInst, int *src1DepInst, int *src2DepInst){
    if(theInst >= progLen){
        return -1;
    }

    InstNode* curNode = FindInstNode(theInst);
    if(curNode == nullptr){
        return -1;
    }

    if(curNode->p1 != nullptr && curNode->p1->instNum == ENTRY_ID){
        *src1DepInst = -1;
        *src2DepInst = -1;
        return 0;
    }

    if(curNode->p1 != nullptr){
        *src1DepInst = curNode->p1->instNum;
    }
    else{
        *src1DepInst = -1;
    }
    if(curNode->p2 != nullptr){
        *src2DepInst = curNode->p2->instNum;
    }
    else{
        *src2DepInst = -1;
    }

    return 0;
}

int DepSim::getProgDepth(){
    int max = 0;

    for(long unsigned int i = 0; i < exit->parents.size(); i++){
        int curDepth = getInstDepth(exit->parents[i]->instNum);
        curDepth += opsLatency[FindInstNode(exit->parents[i]->instNum)->instInfo.opcode];
        if(curDepth > max)
            max = curDepth;
    }

    return max;
}

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    DepSim* simulator = new DepSim(opsLatency, progTrace, numOfInsts);
    if (simulator == NULL){
        return PROG_CTX_NULL;
    }
    return (ProgCtx)simulator;
}

void freeProgCtx(ProgCtx ctx) {
    delete (DepSim*)ctx;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    if(ctx == NULL){
        return -1;
    }
    
    DepSim* simulator = (DepSim*)ctx;
    return simulator->getInstDepth(theInst);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    if(ctx == NULL){
        return -1;
    }

    DepSim* simulator = (DepSim*)ctx;
    return simulator->getInstDeps(theInst, src1DepInst, src2DepInst);
}

int getProgDepth(ProgCtx ctx) {
    if(ctx == NULL){
        return -1;
    }

    DepSim* simulator = (DepSim*)ctx;
    return simulator->getProgDepth();
}


