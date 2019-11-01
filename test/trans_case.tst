###grammar
S -> i saw Obj()         : Obj(case=i) gördüm
S -> i looked at Obj()   : Obj(case=e) baktım
S -> i insisted on Obj() : Obj(case=de) ısrar ettim
S -> i hated Obj()       : Obj(case=den) nefret ettim
Obj -> NP : NP      [case=i,det=0]
Obj -> NP : NP -yı  [case=i,det=1]
Obj -> NP : NP -ya  [case=e]
Obj -> NP : NP -da  [case=de]
Obj -> NP : NP -dan [case=den]
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
