source commands.gdb
si
si
si
si
si
si
si
si
si
b *0x80000086
c
u 58
info registers mepc
i r mepc
p main
si
n
n
h next
n
n
n
n
n
n
n
n
n
n
n
n
n
n
n
s
n
n
n
n
n
n
n
p p->name
fin
b main.c:42
c
s
n
n
n
n
n
n
p p->name
n
n
si
si
si
si
si 10
si5
si 5
si 5
si 5
si
si
si
i r ra
si
n
n
n
n
n
s
p p->name
n
p p->name
u 112
si
si
si
si
si
si
si
si
si
si
si
si
si
si 10
si 10
si 10
si
si
si
si
si
i r sepc
si
si
si
si
si
si
i r stvec
i r scause
si
i r stvec
i r scause
si 10
si 10
si 10
si 10
n
n
n
n
n
p p->name
n
n
n
n
i r scaus
n
n
n
n
n
s
p p->name
n
p p->name
u 112
si
si
si
si
si
si
si
si
si
si
si
si
si
si 10
si 10
si 10
si
si
si
si
si
si
add-symbol-file user/_init
da
n
n
n
n
n
n
n
n
q
