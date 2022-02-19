@from https://github.com/LunarLambda/libseven/blob/main/src/hw/timer.s
@
@ This Source Code Form is subject to the terms of the Mozilla Public
@ License, v. 2.0. If a copy of the MPL was not distributed with this
@ file, You can obtain one at https://mozilla.org/MPL/2.0/.
@


.syntax         unified
.cpu            arm7tdmi

.equ    TM0VAL, 0x04000100
.equ        TM0CNT, 0x04000102
.equ        TM1VAL, 0x04000104
.equ        TM1CNT, 0x04000106
.equ        TM2VAL, 0x04000108
.equ        TM2CNT, 0x0400010A
.equ         TM3VAL, 0x0400010C
.equ        TM3CNT, 0x0400010E

.equ      BF_TIMER_FREQ_OFFSET, 0
.equ       BF_TIMER_FREQ_LENGTH, 3

.equ    TIMER_FREQ16MHZ,     0
.equ    TIMER_FREQ262KHZ,    1
.equ    TIMER_FREQ64KHZ,     2
.equ   TIMER_FREQ16KHZ,     3
.equ    TIMER_FREQCASCADE,   4
.equ         TIMER_IRQ_ENABLE,     6
.equ       TIMER_ENABLE,         7

.text 
.globl timerSet
.globl timerEnable
.globl timerGetValue
.globl timerDisable

.thumb_func
 timerSet:
    cmp         r0, #3
    bgt         1f

    lsls        r0, r0, #2
    lsls        r2, r2, #16
    orrs        r2, r2, r1

    ldr         r3, =REG_TM0VAL
    str         r2, [r3, r0]
1:
    bx          lr

.thumb_func
timerEnable:
    cmp         r0, #3
    bgt         1f

    lsls        r0, r0, #2
    ldr        r1, =TIMER_ENABLE
    ldr         r2, =REG_TM0CNT

    ldrh        r3, [r2, r0]
    orrs        r3, r3, r1
    strh        r3, [r2, r0]
1:
    bx          lr

.thumb_func
timerDisable:
    cmp         r0, #3
    bgt         1f

    lsls        r0, r0, #2
    ldr        r1, =TIMER_ENABLE
    ldr         r2, =REG_TM0CNT

    ldrh        r3, [r2, r0]
    bics        r3, r3, r1
    strh        r3, [r2, r0]
1:
    bx          lr

.thumb_func
timerGetValue:
    cmp         r0, #3
    bgt         1f

    lsls        r0, r0, #2
    ldr         r1, =REG_TM0VAL
    ldrh        r0, [r1, r0]
    bx          lr
1:
    movs        r0, #0
    bx          lr

