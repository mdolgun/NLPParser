###grammar
S -> a B C
B -> b
B ->
C -> c
C ->
###input
a
###pformat
S(
    a
    B()
    C()
)
###pformat_ext
S(
#1
    a
    B(
    #3
    )
    C(
    #5
    )
)
