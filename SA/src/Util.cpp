#include "refactor.h"
#include "resub.h"
#include <vector>
#include <algorithm>

/**
 * Initialize the manager
 */
WHY_Man * WHY_Start ( ) {
    // initilize ABC
    Abc_Start();
    // WHY_Man * pMan = new WHY_Man;
    WHY_Man* pMan = new WHY_Man;
    return pMan;
}

/**
 * Read the network from the given file
 */
void WHY_ReadBlif ( WHY_Man * pMan, string Filename ) {

    Abc_Frame_t * pAbc = Abc_FrameGetGlobalFrame();
    ostringstream command("");
    command << "read_blif " << Filename;
    Cmd_CommandExecute( pAbc, command.str().c_str() );
    Abc_Ntk_t * pNtk = Abc_NtkDup( Abc_FrameReadNtk(pAbc) );
    pMan->pNtk = Abc_NtkStrash( pNtk, 0, 1, 0 );
    assert ( Abc_NtkIsStrash( pMan->pNtk ) );
    Abc_NtkDelete(pNtk);
    // pMan->pAig = Abc_NtkToDar( pNtk, 0, 0 );
}

/**
 * Write the rewrite network
 */
void WHY_WriteBlif ( WHY_Man * pMan, char * Filename ) {
    // if no nerwork in the pNtkNew, we check the AigNew
    Abc_Ntk_t * pNtkNetlist = Abc_NtkToNetlist( pMan->pNtk );
    Io_WriteBlif( pNtkNetlist, Filename, 0, 0, 0 );
    Abc_NtkDelete( pNtkNetlist );
}

void WHY_PrintStats ( WHY_Man * pMan ) {
    // old size / old level
    cout << Abc_NtkNodeNum( pMan->pNtk ) << "," << Abc_NtkLevel( pMan->pNtk ) << "," ;
}

/**
 * Release the memory allocated
 */
void WHY_Stop ( WHY_Man * pMan ) {
    Abc_NtkDelete( pMan->pNtk );
    // recycle memory
    Abc_Stop();
    pMan->Rfr_Gain.clear();
    pMan->Rsb_Gain.clear();
    pMan->Rwr_Gain.clear();
    delete pMan;
}



// refactor the whole network once with SA
void SA_Refactor ( double * temp, double ratio, WHY_Man* pMan, bool random) {

    Abc_Ntk_t * pNtk = pMan->pNtk;
    int nNodeSizeMax = 10;
    int nConeSizeMax = 16;
    int fUpdateLevel = 1;
    int fUseZeros = 1;
    int fUseDcs = 1;
    int fVerbose = 0;


    ProgressBar * pProgress;
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCut;
    Dec_Graph_t * pFForm;
    Vec_Ptr_t * vFanins;
    Abc_Obj_t * pNode;
    abctime clk, clkStart = Abc_Clock();
    int i, nNodes;

    assert( Abc_NtkIsStrash(pNtk) );
    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    // start the managers
    pManCut = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, 1000 );
    pManRef = Abc_NtkManRefStart( nNodeSizeMax, nConeSizeMax, fUseDcs, fVerbose );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCut );
    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );

    // resynthesize each node once
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);
    nNodes = Abc_NtkObjNumMax(pNtk);
    pProgress = Extra_ProgressBarStart( stdout, nNodes );

    // Added by MXY about random
        // random
    vector<Abc_Obj_t *> vec;
    Abc_NtkForEachNode( pNtk, pNode, i ) {
        vec.push_back( pNode );
    }

    srand( time(NULL) );

    if (random)
        random_shuffle( vec.begin(), vec.end() );


    for ( int i=0;i<vec.size();i++ )
    {

        pNode = vec[i];

        if ( pNode == NULL ) {
            continue;
        }

        if ( pNode->Id == 0 ) {
            continue;
        }
    // end of random


    // Abc_NtkForEachNode( pNtk, pNode, i )
    // {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // skip the constant node
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
            continue;
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
            continue;
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        // compute a reconvergence-driven cut
clk = Abc_Clock();
        vFanins = Abc_NodeFindCut( pManCut, pNode, fUseDcs );
pManRef->timeCut += Abc_Clock() - clk;
        // evaluate this cut
clk = Abc_Clock();
        // pFForm = Abc_NodeRefactor( pManRef, pNode, vFanins, fUpdateLevel, fUseZeros, fUseDcs, fVerbose ); changed
        pFForm = EvalNodeRef( pManRef, pNode, vFanins);
pManRef->timeRes += Abc_Clock() - clk;
        if ( pFForm == NULL )
            continue;
//        if (pManRef->nLastGain > 0)
//         cout << pManRef->nLastGain << endl;
        // added by MXY
        if (pManRef->nLastGain < 0) {
            double threshold = exp(pManRef->nLastGain / *temp);
            double rand_num = rand() / ( RAND_MAX + 1.0 );
            if (threshold <= rand_num)
                continue;
        }
        *temp *= ratio;
        // acceptable replacement found, update the graph
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pFForm, fUpdateLevel, pManRef->nLastGain );
pManRef->timeNtk += Abc_Clock() - clk;
        Dec_GraphFree( pFForm );

        // Print result
        WHY_PrintStats ( pMan );
        cout << endl;
    }
    Extra_ProgressBarStop( pProgress );
pManRef->timeTotal = Abc_Clock() - clkStart;
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);

    // delete the managers
    Abc_NtkManCutStop( pManCut );
    Abc_NtkManRefStop( pManRef );
    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );
    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkRefactor: The network check has failed.\n" );
        return;
    }
    return;
}




Dec_Graph_t* EvalNodeRef(Abc_ManRef_t * p, Abc_Obj_t * pNode, Vec_Ptr_t * vFanins) {
    // Notice: (pFForm == NULL) means cannot update, check it first
    int fVeryVerbose = 0;
    int fUpdateLevel = 1;
    int fUseZeros = 1;
    int fUseDcs = 1;
    int fVerbose = 0;
    int nVars = Vec_PtrSize(vFanins);
    int nWordsMax = Abc_Truth6WordNum(p->nNodeSizeMax);
    Dec_Graph_t * pFForm;
    Abc_Obj_t * pFanin;
    word * pTruth;
    abctime clk;
    int i, nNodesSaved, nNodesAdded, Required;

    p->nNodesConsidered++;

    Required = fUpdateLevel? Abc_ObjRequiredLevel(pNode) : ABC_INFINITY;

    // get the function of the cut
clk = Abc_Clock();
    pTruth = Abc_NodeConeTruth( p->vVars, p->vFuncs, nWordsMax, pNode, vFanins, p->vVisited );
p->timeTru += Abc_Clock() - clk;
    if ( pTruth == NULL ) {
        pFForm = NULL;
        return NULL;
    }

    // always accept the case of constant node
    if ( Abc_NodeConeIsConst0(pTruth, nVars) || Abc_NodeConeIsConst1(pTruth, nVars) )
    {
        p->nLastGain = Abc_NodeMffcSize( pNode );
        p->nNodesGained += p->nLastGain;
        p->nNodesRefactored++;
        pFForm = Abc_NodeConeIsConst0(pTruth, nVars) ? Dec_GraphCreateConst0() : Dec_GraphCreateConst1();
        return Abc_NodeConeIsConst0(pTruth, nVars) ? Dec_GraphCreateConst0() : Dec_GraphCreateConst1();
    }

    // get the factored form
clk = Abc_Clock();
    pFForm = (Dec_Graph_t *)Kit_TruthToGraph( (unsigned *)pTruth, nVars, p->vMemory );
p->timeFact += Abc_Clock() - clk;

    // mark the fanin boundary
    // (can mark only essential fanins, belonging to bNodeFunc!)
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pFanin, i )
        pFanin->vFanouts.nSize++;
    // label MFFC with current traversal ID
    Abc_NtkIncrementTravId( pNode->pNtk );
    nNodesSaved = Abc_NodeMffcLabelAig( pNode );
    // unmark the fanin boundary and set the fanins as leaves in the form
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pFanin, i )
    {
        pFanin->vFanouts.nSize--;
        Dec_GraphNode(pFForm, i)->pFunc = pFanin;
    }

    // detect how many new nodes will be added (while taking into account reused nodes)
clk = Abc_Clock();
    nNodesAdded = Dec_GraphToNetworkCount( pNode, pFForm, nNodesSaved, Required );
p->timeEval += Abc_Clock() - clk;
    // quit if there is no improvement

    if ( nNodesAdded == -1 || (nNodesAdded == nNodesSaved && !fUseZeros) )
    {
        Dec_GraphFree( pFForm );
        pFForm = NULL;
        return NULL;
    }

    // compute the total gain in the number of nodes
    p->nLastGain = nNodesSaved - nNodesAdded;
    p->nNodesGained += p->nLastGain;
    p->nNodesRefactored++;

    // report the progress
    if ( fVeryVerbose )
    {
        printf( "Node %6s : ",  Abc_ObjName(pNode) );
        printf( "Cone = %2d. ", vFanins->nSize );
        printf( "FF = %2d. ",   1 + Dec_GraphNodeNum(pFForm) );
        printf( "MFFC = %2d. ", nNodesSaved );
        printf( "Add = %2d. ",  nNodesAdded );
        printf( "GAIN = %2d. ", p->nLastGain );
        printf( "\n" );
    }
    return pFForm;
}





Dec_Graph_t * Abc_NodeRefactor( Abc_ManRef_t * p, Abc_Obj_t * pNode, Vec_Ptr_t * vFanins, int fUpdateLevel, int fUseZeros, int fUseDcs, int fVerbose )
{
    int fVeryVerbose = 0;
    int nVars = Vec_PtrSize(vFanins);
    int nWordsMax = Abc_Truth6WordNum(p->nNodeSizeMax);
    Dec_Graph_t * pFForm;
    Abc_Obj_t * pFanin;
    word * pTruth;
    abctime clk;
    int i, nNodesSaved, nNodesAdded, Required;

    p->nNodesConsidered++;

    Required = fUpdateLevel? Abc_ObjRequiredLevel(pNode) : ABC_INFINITY;

    // get the function of the cut
clk = Abc_Clock();
    pTruth = Abc_NodeConeTruth( p->vVars, p->vFuncs, nWordsMax, pNode, vFanins, p->vVisited );
p->timeTru += Abc_Clock() - clk;
    if ( pTruth == NULL )
        return NULL;

    // always accept the case of constant node
    if ( Abc_NodeConeIsConst0(pTruth, nVars) || Abc_NodeConeIsConst1(pTruth, nVars) )
    {
        p->nLastGain = Abc_NodeMffcSize( pNode );
        p->nNodesGained += p->nLastGain;
        p->nNodesRefactored++;
        return Abc_NodeConeIsConst0(pTruth, nVars) ? Dec_GraphCreateConst0() : Dec_GraphCreateConst1();
    }

    // get the factored form
clk = Abc_Clock();
    pFForm = (Dec_Graph_t *)Kit_TruthToGraph( (unsigned *)pTruth, nVars, p->vMemory );
p->timeFact += Abc_Clock() - clk;

    // mark the fanin boundary
    // (can mark only essential fanins, belonging to bNodeFunc!)
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pFanin, i )
        pFanin->vFanouts.nSize++;
    // label MFFC with current traversal ID
    Abc_NtkIncrementTravId( pNode->pNtk );
    nNodesSaved = Abc_NodeMffcLabelAig( pNode );
    // unmark the fanin boundary and set the fanins as leaves in the form
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanins, pFanin, i )
    {
        pFanin->vFanouts.nSize--;
        Dec_GraphNode(pFForm, i)->pFunc = pFanin;
    }

    // detect how many new nodes will be added (while taking into account reused nodes)
clk = Abc_Clock();
    // nNodesAdded = Dec_GraphToNetworkCount( pNode, pFForm, nNodesSaved, Required ); changed
    nNodesAdded = Dec_GraphToNetworkCount( pNode, pFForm, 100, Required );
p->timeEval += Abc_Clock() - clk;
    // quit if there is no improvement
    // cout << nNodesAdded - nNodesSaved << " "<< endl;
    // Dec_GraphFree( pFForm );
    // return NULL;
    if ( nNodesAdded == -1 || (nNodesAdded == nNodesSaved && !fUseZeros) )
    {

        Dec_GraphFree( pFForm );
        return NULL;
    }

    // compute the total gain in the number of nodes
    p->nLastGain = nNodesSaved - nNodesAdded;
    p->nNodesGained += p->nLastGain;
    p->nNodesRefactored++;

    // report the progress
    if ( fVeryVerbose )
    {
        printf( "Node %6s : ",  Abc_ObjName(pNode) );
        printf( "Cone = %2d. ", vFanins->nSize );
        printf( "FF = %2d. ",   1 + Dec_GraphNodeNum(pFForm) );
        printf( "MFFC = %2d. ", nNodesSaved );
        printf( "Add = %2d. ",  nNodesAdded );
        printf( "GAIN = %2d. ", p->nLastGain );
        printf( "\n" );
    }
    return pFForm;
}


int Dec_GraphToNetworkCount( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax )
{
    Abc_Aig_t * pMan = (Abc_Aig_t *)pRoot->pNtk->pManFunc;
    Dec_Node_t * pNode, * pNode0, * pNode1;
    Abc_Obj_t * pAnd, * pAnd0, * pAnd1;
    int i, Counter, LevelNew, LevelOld;
    // check for constant function or a literal
    if ( Dec_GraphIsConst(pGraph) || Dec_GraphIsVar(pGraph) )
        return 0;
    // set the levels of the leaves
    Dec_GraphForEachLeaf( pGraph, pNode, i )
        pNode->Level = Abc_ObjRegular((Abc_Obj_t *)pNode->pFunc)->Level;
    // compute the AIG size after adding the internal nodes
    Counter = 0;
    Dec_GraphForEachNode( pGraph, pNode, i )
    {
        // get the children of this node
        pNode0 = Dec_GraphNode( pGraph, pNode->eEdge0.Node );
        pNode1 = Dec_GraphNode( pGraph, pNode->eEdge1.Node );
        // get the AIG nodes corresponding to the children
        pAnd0 = (Abc_Obj_t *)pNode0->pFunc;
        pAnd1 = (Abc_Obj_t *)pNode1->pFunc;
        if ( pAnd0 && pAnd1 )
        {
            // if they are both present, find the resulting node
            pAnd0 = Abc_ObjNotCond( pAnd0, pNode->eEdge0.fCompl );
            pAnd1 = Abc_ObjNotCond( pAnd1, pNode->eEdge1.fCompl );
            pAnd  = Abc_AigAndLookup( pMan, pAnd0, pAnd1 );
            // return -1 if the node is the same as the original root
            if ( Abc_ObjRegular(pAnd) == pRoot )
                return -1;
        }
        else
            pAnd = NULL;
        // count the number of added nodes
        if ( pAnd == NULL || Abc_NodeIsTravIdCurrent(Abc_ObjRegular(pAnd)) )
        {
            Counter++; //changed
            //if ( Counter > NodeMax )
            //    return -1;
        }
        // count the number of new levels
        LevelNew = 1 + Abc_MaxInt( pNode0->Level, pNode1->Level );
        if ( pAnd )
        {
            if ( Abc_ObjRegular(pAnd) == Abc_AigConst1(pRoot->pNtk) )
                LevelNew = 0;
            else if ( Abc_ObjRegular(pAnd) == Abc_ObjRegular(pAnd0) )
                LevelNew = (int)Abc_ObjRegular(pAnd0)->Level;
            else if ( Abc_ObjRegular(pAnd) == Abc_ObjRegular(pAnd1) )
                LevelNew = (int)Abc_ObjRegular(pAnd1)->Level;
            LevelOld = (int)Abc_ObjRegular(pAnd)->Level;
//            assert( LevelNew == LevelOld );
        }
        if ( LevelNew > LevelMax )
            return -1;
        pNode->pFunc = pAnd;
        pNode->Level = LevelNew;
    }
    return Counter;
}

void WHY_EvalRefactor ( WHY_Man * pMan ) {
    // clean up
    pMan->Rfr_Gain.clear();
    // start
    Abc_Ntk_t * pNtk = pMan->pNtk;
    Abc_ManRef_t * pManRef;
    Abc_ManCut_t * pManCut;
    Dec_Graph_t * pFForm;
    Vec_Ptr_t * vFanins;
    Abc_Obj_t * pNode;
    int i, nNodes;

    assert( Abc_NtkIsStrash(pNtk) );
    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);
    // start the managers
    pManCut = Abc_NtkManCutStart( 10, 16, 2, 1000 );
    pManRef = Abc_NtkManRefStart( 10, 16, 1, 0 );
    pManRef->vLeaves   = Abc_NtkManCutReadCutLarge( pManCut );
    // compute the reverse levels if level update is requested
    Abc_NtkStartReverseLevels( pNtk, 0 );

    // resynthesize each node once
    pManRef->nNodesBeg = Abc_NtkNodeNum(pNtk);
    nNodes = Abc_NtkObjNumMax(pNtk);
    Abc_NtkForEachNode( pNtk, pNode, i )
    {

        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
            continue;
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
            continue;
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        // compute a reconvergence-driven cut
        vFanins = Abc_NodeFindCut( pManCut, pNode, 1 );
        // evaluate this cut
        pFForm = Abc_NodeRefactor( pManRef, pNode, vFanins, 1, 1, 1, 0 );


        if ( pFForm == NULL )
            continue;

        // record if a valid graph can be found
        pMan->Rfr_Gain[ Abc_ObjId( pNode ) ] = pManRef->nLastGain;
        cout << pManRef->nLastGain << endl;
        Dec_GraphFree( pFForm );
    }
    pManRef->nNodesEnd = Abc_NtkNodeNum(pNtk);
    // delete the managers
    Abc_NtkManCutStop( pManCut );
    Abc_NtkManRefStop( pManRef );
    // put the nodes into the DFS order and reassign their IDs
    Abc_NtkReassignIds( pNtk );
    // fix the levels
    Abc_NtkStopReverseLevels( pNtk );
}









