lui x8 0x0
addi x8 x8 0x500

lui x9 0x0
addi x9 x9 0x300

lui x10 0x0
add x10 x8 x9

lui x11 0x0
addi x11 x8 0x700 


lui x2 0x0
addi x2 x2 0x4E0

lui x3 0X0
addi x3 x3 0xE0

lui x4 0X0
addi x4 x10 0xE0

lui x5 0X0
addi x5 x11 0xE0

lui x6 0X0
add x6 x10 x4

lui x7 0X0
addi x7 x7 0xAAA

lui x12 0X0
addi x12 x12 0X0

lui x13 0X0
addi x13 x13 35

sw x7 0x0(x2)
sw x7 0x0(x3)
sw x7 0x0(x4)
sw x7 0x0(x5)


sw x7 0x0(x6)
addi x12 x12 0x1 

bgeu x13 x12 -8

lui x15 0X0
addi x15 x15 0x1

lui x14 0X0
addi x14 x14 0x1

bgeu x8 x9 640


# потом скачки по фрагментам оперативной памяти вида 

bgeu x8 x9 120 