###grammar
%define token1 token2
%ifdef token1
    %ifdef token2
        S -> in : out1
    %else
        S -> in : out2
    %endif
%else
    %ifdef token2
        S -> in : out3
    %else
        S -> in : out4
    %endif    
%endif
S -> in : out
###input
in
###enum
out1
out
###grammar
%define token1
%ifdef token1
    %ifdef token2
        S -> in : out1
    %else
        S -> in : out2
    %endif
%else
    %ifdef token2
        S -> in : out3
    %else
        S -> in : out4
    %endif    
%endif
S -> in : out
###input
in
###enum
out2
out
###grammar
%define token2
%ifdef token1
    %ifdef token2
        S -> in : out1
    %else
        S -> in : out2
    %endif
%else
    %ifdef token2
        S -> in : out3
    %else
        S -> in : out4
    %endif    
%endif
S -> in : out
###input
in
###enum
out3
out
###grammar
%ifdef token1
    %ifdef token2
        S -> in : out1
    %else
        S -> in : out2
    %endif
%else
    %ifdef token2
        S -> in : out3
    %else
        S -> in : out4
    %endif    
%endif
S -> in : out
###input
in
###enum
out4
out
