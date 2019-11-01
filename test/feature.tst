###grammar
%auto_dict none
S -> NP(case=nom,numb,pers) VP NP(case=acc)
NP -> i     [case=nom,numb=sing,pers=1]
NP -> he    [case=nom,numb=sing,pers=3]
NP -> she   [case=nom,numb=sing,pers=3]
NP -> it    [case=nom,numb=sing,pers=3]
NP -> we    [case=nom,numb=plur,pers=1]
NP -> you   [case=nom,numb=plur,pers=2]
NP -> they  [case=nom,numb=plur,pers=3]

NP -> me    [case=acc,numb=sing,pers=1]
NP -> him   [case=acc,numb=sing,pers=3]
NP -> her   [case=acc,numb=sing,pers=3]
NP -> it    [case=acc,numb=sing,pers=3]
NP -> us    [case=acc,numb=plur,pers=1]
NP -> you   [case=acc,numb=plur,pers=2]
NP -> them  [case=acc,numb=plur,pers=3]

NP -> Det Noun  [pers=3]

Det -> this   [numb=sing]
Det -> these  [numb=plur]
Det -> a      [numb=sing]
Det -> two    [numb=plur]
Det -> the
Det ->

Noun -> man   [numb=sing]
Noun -> men   [numb=plur]

VP -> am  Ving  [numb=sing,pers=1]
VP -> is  Ving  [numb=sing,pers=3]
VP -> are Ving  [numb=plur]

VP -> was  Ving [numb=sing]
VP -> were Ving [numb=plur]

VP -> Ved
VP -> V         [numb=sing,pers=1]
VP -> Vs        [numb=sing,pers=3]
VP -> V         [numb=plur]

V    -> watch
Vs   -> watches
Ving -> watching
Ved  -> watched
###input
i am watching her
###pformat_ext
S(
#1[numb=sing,pers=1]
    NP(
    #2[case=nom,numb=sing,pers=1]
        i
    )
    VP(
    #25[numb=sing,pers=1]
        am
        Ving(
        #36
            watching
        )
    )
    NP(
    #11[case=acc,numb=sing,pers=3]
        her
    )
)
###input
she is watching me
###pformat_ext
S(
#1[numb=sing,pers=3]
    NP(
    #4[case=nom,numb=sing,pers=3]
        she
    )
    VP(
    #26[numb=sing,pers=3]
        is
        Ving(
        #36
            watching
        )
    )
    NP(
    #9[case=acc,numb=sing,pers=1]
        me
    )
)
###input
these men are watching us
###pformat_ext
S(
#1[numb=plur,pers=3]
    NP(
    #16[numb=plur,pers=3]
        Det(
        #18[numb=plur]
            these
        )
        Noun(
        #24[numb=plur]
            men
        )
    )
    VP(
    #27[numb=plur]
        are
        Ving(
        #36
            watching
        )
    )
    NP(
    #13[case=acc,numb=plur,pers=1]
        us
    )
)
###input
me am watching you
###pformat_ext
*UnifyError
###input
she is watching i
###pformat_ext
*UnifyError
###input
two man is watching it
###pformat_ext
*UnifyError
###input
a man watch us
###pformat_ext
*UnifyError
###input
they watch us
###pformat_ext
S(
#1[numb=plur,pers=3]
    NP(
    #8[case=nom,numb=plur,pers=3]
        they
    )
    VP(
    #33[numb=plur]
        V(
        #34
            watch
        )
    )
    NP(
    #13[case=acc,numb=plur,pers=1]
        us
    )
)
###input
he watches the men
###pformat_ext
S(
#1[numb=sing,pers=3]
    NP(
    #3[case=nom,numb=sing,pers=3]
        he
    )
    VP(
    #32[numb=sing,pers=3]
        Vs(
        #35
            watches
        )
    )
    NP(
    #16[numb=plur,pers=3]
        Det(
        #21
            the
        )
        Noun(
        #24[numb=plur]
            men
        )
    )
)
###input
he watches a men
###pformat_ext
*UnifyError
