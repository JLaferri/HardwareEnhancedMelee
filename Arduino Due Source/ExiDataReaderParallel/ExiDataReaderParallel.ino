#define EMPTY_BIT_MASK 0x2000
#define FULL_BIT_MASK 0x1000
#define ACK_BIT_MASK 0x4000

void setup() {
  // put your setup code here, to run once:
  Serial.begin(250000);

  for (int i = 33; i < 41; i++) pinMode(i, INPUT);

  pinMode(49, OUTPUT);
  pinMode(50, INPUT);
  pinMode(51, INPUT);

  digitalWrite(49, LOW);
  
  //Empty the FPGA buffer on startup
  while((PIOC->PIO_PDSR & EMPTY_BIT_MASK) == 0) {
    digitalWrite(49, HIGH);
    digitalWrite(49, LOW);
  }
}

byte num, mosi, miso;
int count, byteCount = 0;
byte buf[1024];
byte present[1024];

bool printed = false;

void PrintHex8(byte num) // prints 8-bit data in hex with leading zeroes
{
  char tmp[16];
  sprintf(tmp, "%.2X",num); 
  Serial.print(tmp);
}

void loop() {
  //Read port C
  word portC = PIOC->PIO_PDSR;

  //If buffer is not empty
  if((portC & EMPTY_BIT_MASK) == 0)
  {
    //Read current value
    if (count == 0) {
      byte newNum = (byte)(portC >> 1);
      if (newNum < num) {
        for (int i = 0; i <= num; i++) present[i] = 0; //Clear previous buf values
      }
      num = newNum;
    }
    else if (count == 1) mosi = (byte)(portC >> 1);

    //Acknowledge the read
    digitalWrite(49, HIGH);
    digitalWrite(49, LOW);

    present[num] = 1;
    buf[num] = mosi;
    
    count++;
    if (count > 1) count = 0;

    printed = false;
  } else {
    if (!printed)
    {
      Serial.print("("); PrintHex8(num); Serial.print(") ");
      for (int i = 0; i <= num; i++) {
        PrintHex8(i); Serial.print(" ");
      }
      Serial.println();

      Serial.print("("); PrintHex8(num); Serial.print(") ");
      for (int i = 0; i <= num; i++) {
        if (present[i] == 1) {
          PrintHex8(buf[i]); Serial.print(" ");
        } else {
          Serial.print("NN ");
        }
      }
      Serial.println();
      
      printed = true;
    }
  }
}
