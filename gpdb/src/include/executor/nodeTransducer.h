/*
 * nodeTransducer.h
 *
 * Copyright (c) 2016, VitesseData Inc.
 */

#ifndef NODE_TRANSDUCER_H
#define NODE_TRANSDUCER_H

#include "nodes/execnodes.h" 

extern int ExecCountSlotsTransducer(Transducer *tr);
extern TransducerState* ExecInitTransducer(Transducer *tr, EState *estate, int eflags);
extern TupleTableSlot* ExecTransducer(TransducerState *trst);
extern void ExecEndTransducer(TransducerState *trst);
extern void ExecReScanTransducer(TransducerState *node, ExprContext *exprCtxt);
#endif
