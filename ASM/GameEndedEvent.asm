#**************************************************************************
#  Inject at address 0x801A4160 (This is one word after the skip results screen code)
#***************************************************************************

stb r0,0x3(r31) #execute replaced code line

#***************************************************************************
#                  		subroutine: gameEnded
#  description: writes game ended event to EXI bus
#***************************************************************************
#create stack frame and store link register
mflr r0
stw r0, 0x4(r1)
stwu r1,-0x20(r1)

bl startExiTransfer #indicate transfer start

li r3, 0x39
bl sendByteExi #send OnMatchEnd event code

#check byte that will tell us whether the game was won by stock loss or by ragequit
lis r3, 0x8047
lbz r3, -0x4960(r3)
bl sendByteExi #send win condition byte. this byte will be 0 on ragequit, 1 or 3 on win by stock loss

bl endExiTransfer #stop transfer

CLEANUP:
#restore registers and sp
lwz r0, 0x24(r1)
addi r1, r1, 0x20
mtlr r0

b GECKO_END

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
li r10, 0xD0 #bit pattern to set clock to 32 MHz and enable CS for device 0
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
EXI_CHECK_RECEIVE_WAIT:                
lwz	r10, 0x6820(r11)
andi. r10, r10, 1
bne	EXI_CHECK_RECEIVE_WAIT #while((exi_chan1cr)&1);

blr

#***************************************************************************
#                   	subroutine: sendHalfExi
#  description: sends two bytes over port B exi
#  inputs: r3: bytes to send
#***************************************************************************
sendHalfExi:
lis r11, 0xCC00 #top bytes of address of EXI registers
li r10, 0x15 #bit pattern to write to control register to write one byte

#write value in r3 to EXI
slwi r3, r3, 16 #the bytes to send have to be left shifted
stw	r3, 0x6824(r11) #store bytes into transfer register
stw	r10, 0x6820(r11) #write to control register to begin transfer

#wait until byte has been transferred
EXI_CHECK_RECEIVE_WAIT_HALF:                
lwz	r10, 0x6820(r11)
andi. r10, r10, 1
bne	EXI_CHECK_RECEIVE_WAIT_HALF #while((exi_chan1cr)&1);

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
EXI_CHECK_RECEIVE_WAIT_WORD:                
lwz	r10, 0x6820(r11)
andi. r10, r10, 1
bne	EXI_CHECK_RECEIVE_WAIT_WORD #while((exi_chan1cr)&1);

blr

#***************************************************************************
#                  subroutine: endExiTransfer
#  description: stops port B writes
#***************************************************************************
endExiTransfer:
mflr r0
stw r0, 0x4(r1)
stwu r1,-0x20(r1)
#TEMPORARY: Send dummy byte at the end of message so fpga can write out last good byte
li r3, 0xFF
bl sendByteExi

lis r11, 0xCC00 #top bytes of address of EXI registers

li r10, 0
stw	r10, 0x6814(r11) #write 0 to the parameter register

lwz r0, 0x24(r1)
addi r1, r1, 0x20
mtlr r0
blr

GECKO_END: