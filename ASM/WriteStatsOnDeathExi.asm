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
li r10, 0x80 #bit pattern to set clock to 1 MHz and enable CS for device 0
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
mflr r0
#the 3 other exi functions only use r3, r10, and r11
mr r6,r3 #move address to register that we know wont be overwritten

bl startExiTransfer #inialize transfer

#write out event byte
mr r3,r5
bl sendByteExi

#iterate through memory locations, writing bytes
writeByte:
lbz r3, 0x0000(r6) #look at current address of r6 and write that byte
bl sendByteExi

#check to see if done writing bytes
addi r6, r6, 1 #increment memory address
subi r4, r4, 1 #decrement bytes left to write
cmpwi r4, 0
bne writeByte #if there are bytes left to write, branch to write byte

bl endExiTransfer #close transfer

mtlr r0
blr

#***************************************************************************
#                        subroutine: writeOnDeathData
#  description: writes interesting data to exi channel on character death
#***************************************************************************
writeOnDeathData:
#execute replaced instruction (this updates a player's stock count)
stb r0,0x008E(r4)

#load start address of stored data
lis r12,0x8000
ori r12,r12,0x2A00

#get and write game time played (counted in frames)
lis r11,0x8046
ori r11,r11,0xB6C4
lwz r4,0x0000(r11)
stw r4,0x0000(r12)

#increment write position to player 1 write position
addi r12,r12,4

#initialize player id for search, start with player id 0
li r8, 0

loopPlayers:
#execute function to get character data pointer
addi r3,r8,0
lis   r11,0x8003
ori   r11,r11,0x4110
mtctr   r11
bctrl

#once here r3 is equal to character data pointer, if pointer is equal to 0, skip to next player
cmpwi r3,0
beq increment

#here the character data pointer is valid, read data and store in memory somewhere

#load and store stock count, this is a global address, must fetch
lis r11,0x8045
ori r11,r11,0x310E
mulli r10,r8,0xE90
add r11,r11,r10
lbz r4,0x0000(r11)
stw r4,0x0000(r12)

#load and store percentage
lwz r4,0x1890(r3)
stw r4,0x0004(r12)

#load and store last move connected
lwz r4,0x20EC(r3)
stw r4,0x0008(r12)

increment:
addi r12,r12,0xC
addi r8,r8,1
cmpwi r8,4
blt loopPlayers

#load start address of stored into r3
lis r3,0x8000
ori r3,r3,0x2A03 #offset for right-most byte to start

#load amount of bytes to write into r4
li r4,52

#load event code into r5
li r5,0x36

bl sendBlockExi