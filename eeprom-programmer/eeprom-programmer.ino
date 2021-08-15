/* This code contains the read and write methods for the EEPROM and a 7-segment cathode data array for 1 digit example */

//Constants
#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define EEPROM_WE 13


/**
 * Sets the address and output enable
 */
void setAddress(int address, bool outputEnable) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0 : 128));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);
  
  //Triggers a clock pulse for the shift register output clock signal
  digitalWrite(SHIFT_LATCH, !digitalRead(SHIFT_LATCH));
  digitalWrite(SHIFT_LATCH, !digitalRead(SHIFT_LATCH));
}


/**
 * Read a byte from the EEPROM at the specified address.
 */
byte readEEPROM(int address){
  /* Since we need the EEPROM to output to the "I/O BUS", we need to make sure that at most 1 party is outputting at a time */
  // Set arduino Data pins to be input (thus freeing the BUS)
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin++) {
    pinMode(pin, INPUT);
  }

  //Load address into the EEPROM, and output enable EEPROM (the boolean)
  setAddress(address, true); 

  byte data = 0; //initialize byte

  //Populate byte with the I/O BUS contents
  //start reading MSB first, then shift data to the left each time before summing next pin
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin--) {
    data = (data << 1) + digitalRead(pin);
  }
  
  return data;
}

/**
 * Read the contents of the EEPROM from a to b addresses and print them to the serial monitor.
 */
void printContents(int a, int b) {
  Serial.println("Reading EEPROM");
  char buffer[20];
  for (int i = a; i <= b; i++) {
  sprintf(buffer, "%d: %02x", i, readEEPROM(i)); //1024: 0xFF
  Serial.println(buffer);
  }
}

/**
 * One Write cycle of a byte into the EEPROM at the specified address.
 */
void writeEEPROM(int address, byte data) {
  //Not only set addres, but also disable EEPROM from driving the BUS (and setting it's I/O pins as I)
    setAddress(address, /*outputEnable*/ false);

    //Setting our nano D5 to D12 pins (which are connected to D0 and D7 of the EEPROM) as outputs
    for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin++) {
        pinMode(pin, OUTPUT);
    }

    // keep track of msb for polling write completion
    byte msb = data & 128;

    // write our byte starting from the least significant bit, (then shifting the given data to be written once to the right such that next least significant bits are the ones stored
    for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin++) {
        digitalWrite(pin, data & 1);
        data = data >> 1;
    }

    // toggle write enable pin
    digitalWrite(EEPROM_WE, LOW);
    delayMicroseconds(1); //right at the maximum write pulse width value because arduino cant go lower
    digitalWrite(EEPROM_WE, HIGH);

    // at this point a write should be in progress...
    delay(1); //we're replacing the arbitrary write cycle delay with the MSB poll feature, but you have to give time to the MSB to get updated

    byte pollBusy = readEEPROM(address) & 128;
    while (pollBusy != msb) {
        pollBusy = readEEPROM(address) & 128;
        delay(1);
    }
}


/**
 * Erases entire EEPROM with 0s (then in case I make wrong assembly code I know for sure that all control signals will be low and the computer BUS won't be driven by multiple parties)
 */
void deleteAll(){
  Serial.print("Erasing EEPROM");
  for (int address = 0; address <= 2047; address++) {
    writeEEPROM(address, 0);
    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
  Serial.println(" done");
}

//4-bit hex decoder for a common cathode 7-segment display
byte data[] = {
  0x7e, //0b01111110 = display 0 at EEPROM address 0
  0x30, //0b00110000 = display 1 at EEPROM address 1
  0x6d, //0b01101101 = display 2 at EEPROM address 2
  0x79, //0b01111001 = display 3 at EEPROM address 3
  0x33, //0b00110011 = display 4 at EEPROM address 4
  0x5b, //0b01011011 = display 5 at EEPROM address 5
  0x5f, //0b01011111 = display 6 at EEPROM address 6
  0x70, //0b01110000 = display 7 at EEPROM address 7
  0x7f, //0b01111111 = display 8 at EEPROM address 8
  0x7b, //0b01111011 = display 9 at EEPROM address 9
  0x77, //0b01110111 = display A at EEPROM address 10
  0x1f, //0b00b11111 = display b at EEPROM address 11
  0x4e, //0b01001110 = display C at EEPROM address 12
  0x3d, //0b00111101 = display d at EEPROM address 13
  0x4f, //0b01001111 = display E at EEPROM address 14
  0x47  //0b01000111 = display F at EEPROM address 15
  };

/**
 * Program data bytes
 */
void program(){
  Serial.print("Programming EEPROM");
  for (int address = 0; address < sizeof(data); address++) {
    writeEEPROM(address, data[address]);

    if (address % 64 == 0) {
      Serial.print(".");
    }

  }
  Serial.println(" done");
}


//Run only once
void setup(){
  //Set our shift register pins as outputs
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);

  //Set \\(\overline{WE}\\) default high, in the good practice arduino order: first the value, then the pinMode
  //We want to avoid the EEPROM_WE pin being low by default as it could accidentally write data
  digitalWrite(EEPROM_WE, HIGH);
  pinMode(EEPROM_WE, OUTPUT);

  Serial.begin(4800) ; //opens USB port and sends that many bits at that rate per seconds (to see/send stuff in the Monitor)

  deleteAll();
  //program();
  printContents(0,2047); //print at arduino IDE!
 
}

void loop() {

}