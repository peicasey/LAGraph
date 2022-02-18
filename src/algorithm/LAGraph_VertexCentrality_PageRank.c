//------------------------------------------------------------------------------
// LAGraph_VertexCentrality_PageRank: pagerank
//------------------------------------------------------------------------------

// LAGraph, (c) 2021 by The LAGraph Contributors, All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause
// Contributed by Tim Davis and Mohsen Aznaveh, Texas A&M University.

//------------------------------------------------------------------------------

// This is an Advanced algorithm (G->AT and G->rowdegree are required).

// PageRank (not for the GAP benchmark, but for production use).  Do not use
// this method for the GAP benchmark.  Use LAGraph_VertexCentrality_PageRankGAP
// instead.

// Unlike LAGraph_VertexCentrality_PageRankGAP, this algorithm handles sinks
// correctly (nodes with no outgoing edges).  The GAP method ignores sinks, and
// thus sum(centrality) is not maintained as 1.  This method handles sinks
// properly, and thus keeps sum(centrality) equal to 1.

// The G->AT and G->rowdegree properties must be defined for this method.  If G
// is undirected or G->A is known to have a symmetric structure, then G->A is
// used instead of G->AT, however.  G->rowdegree must be computed so that it
// contains no explicit zeros; as done by LAGraph_Property_RowDegree.

#define LG_FREE_WORK                \
{                                   \
    GrB_free (&d1) ;                \
    GrB_free (&d) ;                 \
    GrB_free (&t) ;                 \
    GrB_free (&w) ;                 \
    GrB_free (&sink) ;              \
    GrB_free (&rsink) ;             \
}

#define LG_FREE_ALL                 \
{                                   \
    LG_FREE_WORK ;                  \
    GrB_free (&r) ;                 \
}

#include "LG_internal.h"

int LAGraph_VertexCentrality_PageRank
(
    // outputs:
    GrB_Vector *centrality, // centrality(i): pagerank of node i
    // inputs:
    const LAGraph_Graph G,  // input graph
    float damping,          // damping factor (typically 0.85)
    float tol,              // stopping tolerance (typically 1e-4) ;
    int itermax,            // maximum number of iterations (typically 100)
    int *iters,             // output: number of iterations taken
    char *msg
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    LG_CLEAR_MSG ;
    GrB_Vector r = NULL, d = NULL, t = NULL, w = NULL, d1 = NULL ;
    GrB_Vector sink = NULL, rsink = NULL ;
    LG_ASSERT (centrality != NULL, GrB_NULL_POINTER) ;
    LG_TRY (LAGraph_CheckGraph (G, msg)) ;
    GrB_Matrix AT ;
    if (G->kind == LAGRAPH_ADJACENCY_UNDIRECTED ||
        G->structure_is_symmetric == LAGRAPH_TRUE)
    {
        // A and A' have the same structure
        AT = G->A ;
    }
    else
    {
        // A and A' differ
        AT = G->AT ;
        LG_ASSERT_MSG (AT != NULL,
            LAGRAPH_PROPERTY_MISSING, "G->AT is required") ;
    }
    GrB_Vector d_out = G->rowdegree ;
    LG_ASSERT_MSG (d_out != NULL,
        LAGRAPH_PROPERTY_MISSING, "G->rowdegree is required") ;

    //--------------------------------------------------------------------------
    // initializations
    //--------------------------------------------------------------------------

    GrB_Index n ;
    (*centrality) = NULL ;
    GrB_TRY (GrB_Matrix_nrows (&n, AT)) ;

    const float damping_over_n = damping / n ;
    const float scaled_damping = (1 - damping) / n ;
    float rdiff = 1 ;       // first iteration is always done

    // r = 1 / n
    GrB_TRY (GrB_Vector_new (&t, GrB_FP32, n)) ;
    GrB_TRY (GrB_Vector_new (&r, GrB_FP32, n)) ;
    GrB_TRY (GrB_Vector_new (&w, GrB_FP32, n)) ;
    GrB_TRY (GrB_assign (r, NULL, NULL, (float) (1.0 / n), GrB_ALL, n, NULL)) ;

    // find all sinks, where sink(i) = true if node i has d_out(i)=0, or with
    // d_out(i) not present.  LAGraph_Property_RowDegree computes d_out =
    // G->rowdegree so that it has no explicit zeros, so a structural mask can
    // be used here.
    GrB_Index nsinks, nvals ;
    GrB_TRY (GrB_Vector_nvals (&nvals, d_out)) ;
    nsinks = n - nvals ;
    if (nsinks > 0)
    {
        // sink<!struct(d_out)> = true
        GrB_TRY (GrB_Vector_new (&sink, GrB_BOOL, n)) ;
        GrB_TRY (GrB_assign (sink, d_out, NULL, (bool) true, GrB_ALL, n,
            GrB_DESC_SC)) ;
        GrB_TRY (GrB_Vector_new (&rsink, GrB_FP32, n)) ;
    }

    // prescale with damping factor, so it isn't done each iteration
    // d = d_out / damping ;
    GrB_TRY (GrB_Vector_new (&d, GrB_FP32, n)) ;
    GrB_TRY (GrB_apply (d, NULL, NULL, GrB_DIV_FP32, d_out, damping, NULL)) ;

    // d1 = 1 / damping
    float dmin = 1.0 / damping ;
    GrB_TRY (GrB_Vector_new (&d1, GrB_FP32, n)) ;
    GrB_TRY (GrB_assign (d1, NULL, NULL, dmin, GrB_ALL, n, NULL)) ;
    // d = max (d1, d)
    GrB_TRY (GrB_eWiseAdd (d, NULL, NULL, GrB_MAX_FP32, d1, d, NULL)) ;
    GrB_free (&d1) ;

    //--------------------------------------------------------------------------
    // pagerank iterations
    //--------------------------------------------------------------------------

    for ((*iters) = 0 ; (*iters) < itermax && rdiff > tol ; (*iters)++)
    {
        float teleport = scaled_damping ; // teleport = (1 - damping) / n
        if (nsinks > 0)
        {
            // handle the sinks: teleport += (damping/n) * sum (r (sink))
            // rsink<struct(sink)> = r
            GrB_TRY (GrB_Vector_clear (rsink)) ;
            GrB_TRY (GrB_assign (rsink, sink, NULL, r, GrB_ALL, n, GrB_DESC_S));
            float sum_rsink = 0 ;
            GrB_TRY (GrB_reduce (&sum_rsink, NULL, GrB_PLUS_MONOID_FP32,
                rsink, NULL)) ;
            teleport += damping_over_n * sum_rsink ;
        }
        // swap t and r ; now t is the old score
        GrB_Vector temp = t ; t = r ; r = temp ;
        // w = t ./ d
        GrB_TRY (GrB_eWiseMult (w, NULL, NULL, GrB_DIV_FP32, t, d, NULL)) ;
        // r = teleport
        GrB_TRY (GrB_assign (r, NULL, NULL, teleport, GrB_ALL, n, NULL)) ;
        // r += A'*w
        GrB_TRY (GrB_mxv (r, NULL, GrB_PLUS_FP32, LAGraph_plus_second_fp32,
            AT, w, NULL)) ;
        // t -= r
        GrB_TRY (GrB_assign (t, NULL, GrB_MINUS_FP32, r, GrB_ALL, n, NULL)) ;
        // t = abs (t)
        GrB_TRY (GrB_apply (t, NULL, NULL, GrB_ABS_FP32, t, NULL)) ;
        // rdiff = sum (t)
        GrB_TRY (GrB_reduce (&rdiff, NULL, GrB_PLUS_MONOID_FP32, t, NULL)) ;
    }

    //--------------------------------------------------------------------------
    // free workspace and return result
    //--------------------------------------------------------------------------

    (*centrality) = r ;
    LG_FREE_WORK ;
    return (GrB_SUCCESS) ;
}
