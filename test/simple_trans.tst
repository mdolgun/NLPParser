###grammar
S -> NP VP : NP VP
S -> S in NP : NP -de S 
S -> S with NP : NP -la S 
NP -> i : 
NP -> the man : adam
NP -> the telescope : teleskop
NP -> the house : ev
NP -> NP-1 in NP-2 : NP-2 -deki NP-1
NP -> NP-1 with NP-2 : NP-2 -lu NP-1
VP -> saw NP : NP -ı gördüm
###input
i saw the man in the house with the telescope
###pformatr
S(
    NP(teleskop)
    -la
    S(
        NP(ev)
        -de
        S(
            NP()
            VP(
                NP(adam)
                -ı
                gördüm
            )
        )
    |
        NP()
        VP(
            NP(
                NP(ev)
                -deki
                NP(adam)
            )
            -ı
            gördüm
        )
    )
|
    NP(
        NP(teleskop)
        -lu
        NP(ev)
    )
    -de
    S(
        NP()
        VP(
            NP(adam)
            -ı
            gördüm
        )
    )
|
    NP()
    VP(
        NP(
            NP(teleskop)
            -lu
            NP(
                NP(ev)
                -deki
                NP(adam)
            )
        |
            NP(
                NP(teleskop)
                -lu
                NP(ev)
            )
            -deki
            NP(adam)
        )
        -ı
        gördüm
    )
)
###formatr
S(NP(teleskop) -la S(NP(ev) -de S(NP() VP(NP(adam) -ı gördüm))|NP() VP(NP(NP(ev) -deki NP(adam)) -ı gördüm))|NP(NP(teleskop) -lu NP(ev)) -de S(NP() VP(NP(adam) -ı gördüm))|NP() VP(NP(NP(teleskop) -lu NP(NP(ev) -deki NP(adam))|NP(NP(teleskop) -lu NP(ev)) -deki NP(adam)) -ı gördüm))
###enum
teleskopla evde adamı gördüm
teleskopla evdeki adamı gördüm
teleskoplu evde adamı gördüm
teleskoplu evdeki adamı gördüm
teleskoplu evdeki adamı gördüm
