###grammar
S -> Subj VP Obj : Obj VP Subj
Subj -> i             : -m
Subj -> you           : -n
Subj -> it | she | he
Subj -> NP : NP
VP -> saw         : gördü       [case=i]
VP -> looked at   : baktı       [case=e]
VP -> insisted on : ısrar etti  [case=de]
VP -> hated       : nefret etti [case=den]
Obj -> NP : NP Case 
Case ->        [case=i,det=0]
Case -> : -yı  [case=i,det=1]
Case -> : -ya  [case=e]
Case -> : -da  [case=de]
Case -> : -dan [case=den]
NP -> a car   : bir araba [det=0]
NP -> the car : araba     [det=1]
###input
i saw a car
###enum
bir araba gördüm
###input
i saw the car
###enum
arabayı gördüm
###input
i hated the car
###enum
arabadan nefret ettim
###input
i looked at the car
###enum
arabaya baktım
###input
i insisted on the car
###enum
arabada ısrar ettim
###input
i insisted on a car
###enum
bir arabada ısrar ettim
