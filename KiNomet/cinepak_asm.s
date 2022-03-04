.syntax         unified
.cpu            arm7tdmi
 .globl   MAKECOLOUR16
.p2align 2
.type    MAKECOLOUR16,%function
.fnstart

MAKECOLOUR16:

   //r0 = r
   //r1 = g
   //r2 = b 
   //shift the colors by 3
   lsr r0, #3 
   lsr r1, #3
   lsr r2, #3 

   //and the colors 
   and r0, r0, #31 
   and r1, r1,  #31 
   and r2, r2, #31 

   //perform the special operations
   lsl r1, #5
   lsl r2, #10
   orr r0, r1//Combine it 
   orr r0, r2//Combine it  2 
1:
    bx          lr
.fnend


 .globl   GBA_RLEDECOMP
.p2align 2
.type    GBA_RLEDECOMP,%function
.fnstart

GBA_RLEDECOMP:

swi 0x14
1:
    bx          lr
.fnend



 .globl   GBA_LZDECOMP
.p2align 2
.type    GBA_LZDECOMP,%function
.fnstart

GBA_LZDECOMP:

  swi 0x11
1:
    bx          lr
.fnend