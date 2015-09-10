#**************************************************************************
#					 Inject at address 0x8016CDAC
#**************************************************************************
#						DOL Mod Replacements
# 0x30928: Replace 9804008E with 4BFD0988 (Branch 0x80033D48 -> 0x800046D0)
# 0x16D0: Replace 0s with code
# 0x17B8: Replace 0 with 4802F594 (Branch 0x800047B8 -> 0x80033D4C)
#***************************************************************************

lbz	r0,0x5(r3) #execute replaced code line

#***************************************************************************
#                  		subroutine: writeStats
#  description: writes stats to EXI port on each frame
#***************************************************************************
#create stack frame and store link register
mflr r0
stw r0, 0x4(r1)
stwu r1,-0x20(r1)
stw r31,0x1C(r1)
stw r30,0x18(r1)

#check if in single player mode, and ignore code if so
lis	r3,0x801A # load SinglePlayer_Check function
ori	r3,r3,0x4340	
mtlr	r3	
lis	r3,0x8048	
lbz	r3,-0x62D0(r3) #load menu controller major
blrl	
cmpwi r3,1 # is this single player mode?	
beq- CLEANUP # if in single player, ignore everything

#check if there are 3 or more players
lis	r3,0x8016
ori	r3,r3,0xB558 # load CountPlayers function
mtlr r3
blrl
cmpwi r3,3 # 3 or more players in match?
bge- CLEANUP # skip all this if so

#check if match is paused 
lis	r3,0x8048
lbz	r3,-0x6298(r3) # load screen freeze byte
cmpwi r3,0
bne- CLEANUP # if paused/match end, skip

#check frame count
lis	r3,0x8047
lwz	r3,-0x493C(r3)	# load match frame count
cmpwi r3,0
bne- FRAME_UPDATE

#check pre-match frame count
#lis	r3,0x8170
#lhz	r3,0x6(r3)	# load pre-match frame count
#cmpwi r3,0		# is this absolute first frame?
#bne- CLEANUP # during pre-match freeze frames, don't send data

#------------- MATCH_PARAMS -------------
MATCH_PARAMS:
bl startExiTransfer #indicate transfer start

li r3, 0x37
bl sendByteExi #send OnMatchStart event code

lis r31, 0x8045
ori r31, r31, 0xAC4C

lhz r3, 0x1A(r31) #stage ID half word
bl sendHalfExi

li r30, 0 #load player count

MP_WRITE_PLAYER:
#load character pointer for this player
lis r3, 0x8045
ori r3, r3, 0x3130
mulli r4, r30, 0xE90
add r3, r3, r4
lwz r3, 0x0(r3)

#skip this player if not in game
cmpwi r3, 0
beq MP_INCREMENT

#start writing data
mr r3, r30 #send character port ID
bl sendByteExi

#get start address for this player
lis r31, 0x8045
ori r31, r31, 0xAC4C
mulli r4, r30, 0x24
add r31, r31, r4

lbz r3, 0x6C(r31) #character ID
bl sendByteExi
lbz r3, 0x6D(r31) #player type
bl sendByteExi
lbz r3, 0x6F(r31) #costume ID
bl sendByteExi

MP_INCREMENT:
addi r30, r30, 1
cmpwi r30, 4
blt MP_WRITE_PLAYER

bl endExiTransfer #stop transfer
b CLEANUP

#------------- FRAME_UPDATE -------------
FRAME_UPDATE:
bl startExiTransfer #indicate transfer start

li r3, 0x38
bl sendByteExi #send OnFrameUpdate event code

lis	r3,0x8047
lwz	r3,-0x493C(r3) #load match frame count
bl sendWordExi

li r30, 0 #load player count

FU_WRITE_PLAYER:
#load character pointer for this player
lis r3, 0x8045
ori r3, r3, 0x3130
mulli r4, r30, 0xE90
add r3, r3, r4
lwz r3, 0x0(r3)

#skip this player if not in game
cmpwi r3, 0
beq FU_INCREMENT

mr r31, r3 #load player address into r31

lwz	r3,0x64(r31) #load internal char ID
bl sendByteExi
lwz	r3,0x70(r31) #load action state ID
bl sendHalfExi
lwz	r3,0x110(r31) #load Top-N X coord
bl sendWordExi
lwz	r3,0x114(r31) #load Top-N Y coord
bl sendWordExi
lwz	r3,0x680(r31) #load Joystick X axis
bl sendWordExi
lwz	r3,0x684(r31) #load Joystick Y axis
bl sendWordExi
lwz	r3,0x698(r31) #load c-stick X axis
bl sendWordExi
lwz	r3,0x69c(r31) #load c-stick Y axis
bl sendWordExi
lwz	r3,0x6b0(r31) #load analog trigger input
bl sendWordExi
lwz	r3,0x6bc(r31) #load buttons pressed this frame
bl sendWordExi
lwz	r3,0x1890(r31) #load current damage
bl sendWordExi
lwz	r3,0x19f8(r31) #load shield size
bl sendWordExi
lwz	r3,0x20ec(r31) #load last attack landed
bl sendByteExi
lhz	r3,0x20f0(r31) #load combo count
bl sendByteExi
lwz	r3,0x1924(r31) #load player who last hit this player
bl sendByteExi

FU_INCREMENT:
addi r30, r30, 1
cmpwi r30, 4
blt FU_WRITE_PLAYER

bl endExiTransfer #stop transfer

CLEANUP:
#restore registers and sp
lwz r0, 0x24(r1)
lwz r31, 0x1C(r1)
lwz r30, 0x18(r1)
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
li r10, 0xC0 #bit pattern to set clock to 16 MHz and enable CS for device 0
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
lis r11, 0xCC00 #top bytes of address of EXI registers

li r10, 0
stw	r10, 0x6814(r11) #write 0 to the parameter register

blr

GECKO_END: