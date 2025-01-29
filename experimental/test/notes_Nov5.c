

    // traverse all entries in A(v,:)
    for (int64_t p = Ap [v] ; p < Ap [v+1] ; p++)
    {
        int64_t w = Aj [p] ;
        // ...

    }


    // init P:
    Pj = malloc (anvals * sizeof (int64_t)) ;
    Ptail = malloc (n * sizeof (int64_t)) ;
    Phead = Ap ;
    memcpy (Ptail, Ap, n * sizeof (int64_t)) ; 

    // append (P[w],v)
    Pj [Ptail [w]++] = v ;

    // for all v in P[w]
    for (int64_t p = Phead [w] ; p < Ptail [w+1] ; p++)
    {
        int64_t v = Pj [p] ;
        // ...
    }


