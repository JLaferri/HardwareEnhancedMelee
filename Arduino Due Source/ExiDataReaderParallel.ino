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
  
//  word portC = PIOC->PIO_PDSR;
//
//  Serial.println(portC, BIN);
}

byte num, mosi, miso;
int count;

void loop() {
  //Read port C
  word portC = PIOC->PIO_PDSR;

  //If buffer is not empty
  if((portC & EMPTY_BIT_MASK) == 0)
  {
    //Read current value
    if (count == 0) num = (byte)(portC >> 1);
    else if (count == 1) mosi = (byte)(portC >> 1);

    //Acknowledge the read
    digitalWrite(49, HIGH);
    digitalWrite(49, LOW);
    
    count++;

    if (count > 1)
    {
      Serial.print(num, HEX);
      Serial.print(" ");
      Serial.println(mosi, HEX);

      count = 0;
    }
    //Read current value
//    byte value = (byte)(portC >> 1);
//
//    //Acknowledge the read
//    digitalWrite(49, HIGH);
//    digitalWrite(49, LOW);
//    
//    Serial.println(value, HEX);
  }
}
