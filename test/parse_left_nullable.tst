###grammar
S -> A B c
A -> a
A ->
B -> b
B ->
###input
c
###pformat
S(
    A()
    B()
    c
)
###pformat_ext
S(
#1
    A(
    #3
    )
    B(
    #5
    )
    c
)
