###grammar
S -> A(g=*f) : B(h=*f)
A -> a [g=a]
A -> b [g=b]
B -> : a [h=a]
B -> : b [h=b]
###input
a
###pformat_ext
S(
#1[f=a]
    A(
    #2[g=a]
        a
    )
)
###pformatr_ext
S(
#1[f=a]
    B(
    #4[h=a]
        a
    )
)
###enum
a
