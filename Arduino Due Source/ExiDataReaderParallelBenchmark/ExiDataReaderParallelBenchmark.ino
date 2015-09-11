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

//********************************************************************************************
//**                                    Notes                                               **
//********************************************************************************************
//Transferring 96 bytes (actually 192 because of doubling) takes about 1500 us
//This leaves us with about 15167 us before the next frame
//At a rate of 250000 baud over COM port we can send 1e-6 * (250000 / 10) = 0.025 bytes/us (10 because of sync and stop bits)
//This means we can transfer about 0.025 * 15167 = 379 bytes over COM per frame

byte num, mosi;
byte values[256];
int count, bytesRead = 0;

long lastEnteredLoop;
bool logTime = false;

void loop() {
  //Read port C
  word portC = PIOC->PIO_PDSR;

  if((portC & FULL_BIT_MASK) != 0) {
    Serial.println("Overflow");  
  }
  
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
      values[num] = mosi;

      count = 0;
    }
    bytesRead++;
    logTime = true;
  }
  else
  {
    long loopTime = micros();

    if (logTime) {
      Serial.print(bytesRead); Serial.print("\t");
      Serial.println(loopTime - lastEnteredLoop);
      logTime = false;
    }

    bytesRead = 0;
    lastEnteredLoop = loopTime;
  }
}
