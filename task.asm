# pc = 0x0
# x0 = 0
# x1 = 0x2000 (ra, для выхода)
# x2 = 0xF000 (sp)
# x3 = 0x0      # данные для lw
# x4 = 0x400
# x5 = 0x800
# x6..x31 = 0

# данные: 8 обращений, 5 попаданий
# инструкции: всего 19 команд, 1 промах (первый lw), остальные попадания

lw x6, 0(x3)   # miss, адрес 0x0, tag=0, index=0
lw x6, 0(x4)   # miss, адрес 0x400, tag=1, index=0
lw x6, 0(x5)   # miss, адрес 0x800, tag=2, index=0
lw x6, 0(x3)   # hit, tag=0
lw x6, 0(x4)   # hit, tag=1
lw x6, 0(x5)   # hit, tag=2
lw x6, 0(x3)   # hit, tag=0
lw x6, 0(x4)   # hit, tag=1
nop
nop
nop
nop
nop
nop
nop
nop
nop
jalr x0, 0(x1)   # возврат на ra = 0x2000
ebreak           # завершение программы
