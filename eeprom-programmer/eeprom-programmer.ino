//Constants
#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_ENABLE 13


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
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents() {
  Serial.println("Reading EEPROM");
  for (int base = 0; base <= 255; base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset++) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}

/**
 * One Write cycle of a byte into the EEPROM at the specified address.
 */
void writeEEPROM(int address, byte data) {
  //Not only set addres, but also disable EEPROM from driving the BUS (and setting it's I/O pins as I)
    setAddress(address, /*outputEnable*/ false);

    //Setting our D5 to D12 pins (their names offseted to start at D0) as outputs
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
    digitalWrite(WRITE_ENABLE, LOW);
    delayMicroseconds(1); //right at the maximum write pulse width value because arduino cant go lower
    digitalWrite(WRITE_ENABLE, HIGH);

    // at this point a write should be in progress...
    delay(10);
    // I ended up corrupting the most significant bit of almost all addresses so I think I'd rather implement the poorman's version with a non-malicious delay(10) rather than burning the chip with constant data polling for the MSB bit. The purists can find the data polling implementation below.
    // Luckily that bit can be ignored by the 7 segment display...
    /*
    byte pollBusy = readEEPROM(address) & 128;
    while (pollBusy != msb) {
        delay(1);
        pollBusy = readEEPROM(address) & 128;
    }*/
}


/**
 * Erases entire EEPROM with 0xff
 */
void deleteAll(){
  Serial.print("Erasing EEPROM");
  for (int address = 0; address <= 2047; address++) {
    writeEEPROM(address, 0xff);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
  Serial.println(" done");
}

//4-bit hex decoder for a common cathode 7-segment display
byte data[] = { 0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b, 0x77, 0x1f, 0x4e, 0x3d, 0x4f, 0x47 };

/**
 * Program data bytes
 */
void program(){
  Serial.print("Programming EEPROM");
  for (int address = 0; address < sizeof(data); address += 1) {
    writeEEPROM(address, data[address]);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
  Serial.println(" done");
}


//Run only once
void setup(){
  //Set our constants (D2, D3 and D4) as output pins
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);

  //Set \\(\overline{WE}\\) default high, in the good practice arduino order: first the value, then the pinMode
  //We want to avoid the WRITE_ENABLE pin being low by default as it could accidentally write data
  digitalWrite(WRITE_ENABLE, HIGH);
  pinMode(WRITE_ENABLE, OUTPUT);

  Serial.begin(4800) ; //opens USB port and sends that many bits at that rate per seconds

  deleteAll();
  program();
  printContents(); //print at arduino IDE!
 
}

void loop() {

}