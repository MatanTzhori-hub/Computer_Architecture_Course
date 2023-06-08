/* 046267 Computer Architecture - Winter 20/21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <stdio.h>
#include <stdlib.h>
#define REGISTERNUM 32


typedef struct d_graph_node {
    int latency;
    int inst;
    struct d_graph_node* dependecy1;
    struct d_graph_node* dependecy2;
} DGraphNode;

typedef struct d_grpah {
    DGraphNode* entery;
    DGraphNode** registerDependecies;
    DGraphNode** instsNodes;
    unsigned int instNum;

} DGraph;

/*init the entery nodes and register dependecy array*/

static DGraph* init_graph (const unsigned int instsNum)
{
    DGraphNode* entery = malloc(sizeof(DGraphNode));
    entery->dependecy1 = NULL;
    entery->dependecy2 = NULL;
    entery->latency = 0;
    entery->inst = -1;
    DGraphNode** registerDependecies = malloc(REGISTERNUM*sizeof(DGraphNode*));
    for (int i =0 ; i<REGISTERNUM; i++)
    {
        registerDependecies[i] = entery;
    }
    DGraphNode** instsNodes = malloc(instsNum*sizeof(DGraphNode*));
    for (int i =0 ; i<instsNum; i++)
    {
        instsNodes[i] = NULL;
    }
    DGraph* graph = malloc(sizeof(DGraph));
    graph->entery = entery;
    graph->registerDependecies = registerDependecies;
    graph->instsNodes = instsNodes;
    graph->instNum = instsNum;
    return graph;
}

/*go through the program' for each instruction create a node and assign their dpendecy by dpendecy array*/
ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) 
{
    DGraph* graph = init_graph(numOfInsts);
    InstInfo curr_info;

    for(unsigned int i = 0 ; i<numOfInsts ; i++)
    {
        curr_info = progTrace[i];
        DGraphNode* curr_node = malloc(sizeof(DGraphNode));
        curr_node->dependecy1 = graph->registerDependecies[curr_info.src1Idx];
        curr_node->dependecy2 = graph->registerDependecies[curr_info.src2Idx];
        curr_node->latency = opsLatency[curr_info.opcode];
	    curr_node->inst = i;
        graph->registerDependecies[curr_info.dstIdx] = curr_node;
        graph->instsNodes[i] = curr_node;  
    }
    return (void*)graph;
}

void freeProgCtx(ProgCtx ctx) 
{
    DGraph* graph = (DGraph*)ctx;
    for (int i =0 ; i<graph->instNum;i++)
    {
        free(graph->instsNodes[i]);
    }
    free(graph->entery);
    free(graph->registerDependecies);
    free(graph->instsNodes);
    free(graph);
}


/*use backtracking to calcuate depth in dpendecy graph*/
int getInstLatencyAux(DGraphNode* node)
{
    if (node->inst == -1)
    {
        return 0;
    }
    int latency1  = getInstLatencyAux(node->dependecy1);
    int latency2  = getInstLatencyAux(node->dependecy2);
    if (latency1 > latency2)
    {
        return latency1 + node->latency;
    }
    return latency2 + node->latency;
}


int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    DGraph* graph = (DGraph*)ctx;
    if (theInst > graph->instNum)
    {
        return -1;
    }
    int depth = getInstLatencyAux(graph->instsNodes[theInst]);
    return depth - graph->instsNodes[theInst]->latency;
}

/*return nodes dependecy*/
int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    DGraph* graph = (DGraph*)ctx;
    if (theInst > graph->instNum)
    {
        return -1;
    }
    DGraphNode* req_node = graph->instsNodes[theInst];
    *src1DepInst = req_node->dependecy1->inst;
    *src2DepInst = req_node->dependecy2->inst;
    return 0;

}

/*return maxumin node depth*/
int getProgDepth(ProgCtx ctx) {
    DGraph* graph = (DGraph*)ctx;
    int progDepth = 0;
    int currDepth =0;
    for (int i =0 ; i<graph->instNum;i++)
    {
        currDepth = getInstLatencyAux(graph->instsNodes[i]);
        if (progDepth < currDepth)
        {
            progDepth = currDepth;
        }
    }
    return progDepth;
}

void printDeps(ProgCtx ctx){
    DGraph* graph = (DGraph*)ctx;
    for(long unsigned int i = 0; i < graph->instNum; i++){
        int d1, d2;
        getInstDeps(ctx, graph->instsNodes[i]->inst, &d1, &d2);
        printf("Inst #%d: {%d, %d}\n", graph->instsNodes[i]->inst, d1, d2);
    }
}
