#**************************************************************************
#			Inject at address 0x80033d48 for Gecko Code
#**************************************************************************
#						DOL Mod Replacements
# 0x30928: Replace 9804008E with 4BFD0988 (Branch 0x80033D48 -> 0x800046D0)
# 0x16D0: Replace 0s with code
# 0x17B8: Replace 0 with 4802F594 (Branch 0x800047B8 -> 0x80033D4C)
#***************************************************************************

b writeOnDeathData
#***************************************************************************
#                  subroutine: startExiTransfer
#  description: prepares port B exi to be written to
#***************************************************************************
startExiTransfer:
lis r11, 0xCC00 #top bytes of address of EXI registers

#disable read/write protection on memory pages
lhz r10, 0x4010(r11)
ori r10, r10, 0xFF
sth r10, 0x4010(r11) # disable MP3 memory protection

#set up EXI
li r10, 0xD0 #bit pattern to set clock to 1 MHz and enable CS for device 0
stw	r10, 0x6814(r11) #start transfer, write to parameter register

blr

#***************************************************************************
#                   	subroutine: sendByteExi
#  description: sends one byte over port B exi
#  inputs: r3: byte to send
#***************************************************************************
sendByteExi:
lis r11, 0xCC00 #top bytes of address of EXI registers
li r10, 0x5 #bit pattern to write to control register to write one byte

#write value in r3 to EXI
slwi r3, r3, 24 #the byte to send has to be left shifted
stw	r3, 0x6824(r11) #store current byte into transfer register
stw	r10, 0x6820(r11) #write to control register to begin transfer

#wait until byte has been transferred
exiCheckReceiveWait:                
lwz	r10, 0x6820(r11)
andi. r10, r10, 1
bne	exiCheckReceiveWait #while((exi_chan1cr)&1);

blr

#***************************************************************************
#                   	subroutine: sendWordExi
#  description: sends one word over port B exi
#  inputs: r3: word to send
#***************************************************************************
sendWordExi:
lis r11, 0xCC00 #top bytes of address of EXI registers
li r10, 0x35 #bit pattern to write to control register to write four bytes

#write value in r3 to EXI
stw	r3, 0x6824(r11) #store current bytes into transfer register
stw	r10, 0x6820(r11) #write to control register to begin transfer

#wait until byte has been transferred
exiCheckReceiveWaitWord:                
lwz	r10, 0x6820(r11)
andi. r10, r10, 1
bne	exiCheckReceiveWaitWord #while((exi_chan1cr)&1);

blr

#***************************************************************************
#                  subroutine: endExiTransfer
#  description: stops port B writes
#***************************************************************************
endExiTransfer:
lis r11, 0xCC00 #top bytes of address of EXI registers

li r10, 0
stw	r10, 0x6814(r11) #write 0 to the parameter register

blr

#***************************************************************************
#                        subroutine: sendBlockExi
#  description: writes block of memory to port B of the console
#  inputs: r3: start address of data to write, r4: number of bytes to write, r5: event code
#***************************************************************************
sendBlockExi:
#create stack frame and store link register
mflr r0
stw r0, 0x4(r1)
stwu r1,-0x20(r1)
stw r31,0x1C(r1)
stw r30,0x18(r1)
stw r29,0x14(r1)

mr r29,r3 #move address to register that we know wont be overwritten
mr r30,r4
mr r31,r5

bl startExiTransfer #inialize transfer

#write out event byte
mr r3,r31
bl sendByteExi

#iterate through memory locations, writing bytes
writeByte:
lbz r3, 0x0000(r29) #look at current address of r29 and write that byte
bl sendByteExi

#check to see if done writing bytes
addi r29, r29, 1 #increment memory address
subi r30, r30, 1 #decrement bytes left to write
cmpwi r30, 0
bne writeByte #if there are bytes left to write, branch to write byte

bl endExiTransfer #close transfer

#restore registers and sp
lwz r0, 0x24(r1)
lwz r31, 0x1C(r1)
lwz r30, 0x18(r1)
lwz r29, 0x14(r1)
addi r1, r1, 0x20
mtlr r0

blr

#***************************************************************************
#                        subroutine: writeOnDeathData
#  description: writes interesting data to exi channel on character death
#***************************************************************************
writeOnDeathData:
#execute replaced instruction (this updates a player's stock count)
stb r0,0x008E(r4)

#create stack frame and store link register
mflr r0
stw r0, 0x4(r1)
stwu r1,-0x20(r1)
stw r31,0x1C(r1)
stw r30,0x18(r1)

bl startExiTransfer #initialize transfer

#write event code
li r3,0x10
bl sendByteExi

#get and write game time played (counted in frames)
lis r11,0x8046
ori r11,r11,0xB6C4
lwz r3,0x0000(r11)
bl sendWordExi

#initialize player id for search, start with player id 0
li r30, 0

loopPlayers:
#execute function to get character data pointer
addi r3,r30,0
lis   r11,0x8003
ori   r11,r11,0x4110
mtctr   r11
bctrl

#once here r3 is equal to character data pointer, if pointer is equal to 0, skip to next player
cmpwi r3,0
beq increment

#here the character data pointer is valid, read data and store in memory somewhere
mr r31,r3

#load and store stock count, this is a global address, must fetch
lis r11,0x8045
ori r11,r11,0x310E
mulli r10,r30,0xE90
add r11,r11,r10
lbz r3,0x0000(r11)
bl sendByteExi

#load and store percentage
lwz r3,0x1890(r31)
bl sendWordExi

#load and store last move connected
lwz r3,0x20EC(r31)
bl sendByteExi

increment:
addi r30,r30,1
cmpwi r30,4
blt loopPlayers

#restore registers and sp
lwz r0, 0x24(r1)
lwz r31, 0x1C(r1)
lwz r30, 0x18(r1)
addi r1, r1, 0x20
mtlr r0