
//This function takes a CAN packet and creates a serial string from it in the modified gridConnect format
void CAN2Serial(CAN_message_t canmsg){
  tempcanid1 = 0;
  tempcanid2 = 0;
  if (canmsg.ext){
    outputString = ":X";
  } else outputString = ":S";

  //put code to turn ID into ASCII
  if (canmsg.ext){
    tempcanid1 = (canmsg.id>>13)&0xFF;
    tempcanid2 = (canmsg.id<<3)&0xFF;
    outputString += bytetoASCII(highByte(tempcanid1));
    outputString += bytetoASCII(lowByte(tempcanid1));
    outputString += bytetoASCII(highByte(tempcanid2));
    outputString += bytetoASCII(lowByte(tempcanid2));
  }
  else{
    tempcanid1 = canmsg.id<<5;
    outputString += bytetoASCII(highByte(tempcanid1));
    outputString += bytetoASCII(lowByte(tempcanid1));
  }
  
  if (canmsg.rtr){
    outputString += "R";
  } else outputString += 'N';
  
  //Turn all the data bytes into ascii and add it to the string
  for (int i=0; i < canmsg.len; i++){
      outputString += bytetoASCII(canmsg.buf[i]);
   }

  //End the string with a ; as per the gridConnect protocol
  outputString += ';';
  Serial.print(outputString);
}

//This function converts a byte (typically read from a CAN packet) and turns it into a 2 character string
String bytetoASCII(byte input){
  byte hiNib, loNib;
  String tmpStr;
  hiNib = input>>4;
  loNib = input&0xF;
  tmpStr = ASCII_table[hiNib];
  tmpStr += ASCII_table[loNib];
  return tmpStr;
}

//This function takes an incoming ascii string and constructs a CAN message from it according to the gridConnect protocol
void Serial2CAN(String inString){

  USBtxmsg.len = 8;
  USBtxmsg.rtr = 0;
  USBtxmsg.ext = 0;
  int posTracker = 2;
  unsigned int tempID;
  byte tempIDhi, tempIDlo;
  uint32_t tempxID1, tempxID2;

  //Check if it is an extended frame
  if(inString.charAt(1) == 'X'){
    USBtxmsg.ext = 1;
  } else USBtxmsg.ext = 0;

  //how to process if its an extended frame
  if(USBtxmsg.ext){
    //turn id string into bytes
    tempIDhi = string2Byte(inString.substring(2,4));
    //Serial.println(inString.substring(2,4));    for debug
    tempIDlo = string2Byte(inString.substring(4,6));
    //Serial.println(inString.substring(4,6));    for debug
    tempxID1 = (tempIDhi<<8) | tempIDlo;
    //turn id string into bytes
    tempIDhi = string2Byte(inString.substring(6,8));
    //Serial.println(inString.substring(2,4));    for debug
    tempIDlo = string2Byte(inString.substring(8,10));
    //Serial.println(inString.substring(4,6));    for debug
    tempxID2 = (tempIDhi<<8) | tempIDlo;
    tempxID1 = (tempxID1<<16) | tempxID2;
  
    //Check if it is a remote frame
    if(inString.charAt(10) == 'R'){
      USBtxmsg.rtr = 1;
    } else USBtxmsg.rtr = 0;
    
    //Turn all the ascii characters into bytes for CAN
    for (int j=11; j < 36; j=j+2){
      
        if(inString.charAt(j) == ';') break;
        //take 2 characters, turn them into bytes
        USBtxmsg.buf[(j-7)/2] = string2Byte(inString.substring(j,j+2));
        //Serial.println(inString.substring(j,j+2));    for debug
        posTracker = j;
     } 
  
    //find length
    USBtxmsg.len = (posTracker-9)/2;
    USBtxmsg.id = tempxID1;
  }
  else{
    //turn id string into bytes
    tempIDhi = string2Byte(inString.substring(2,4));
    //Serial.println(inString.substring(2,4));    for debug
    tempIDlo = string2Byte(inString.substring(4,6));
    //Serial.println(inString.substring(4,6));    for debug
    tempID = (tempIDhi<<8) | tempIDlo;
    tempID = tempID>>5;
  
    //Check if it is a remote frame
    if(inString.charAt(6) == 'R'){
      USBtxmsg.rtr = 1;
    } else USBtxmsg.rtr = 0;
    
    //Turn all the ascii characters into bytes for CAN
    for (int j=7; j < 36; j=j+2){
      
        if(inString.charAt(j) == ';') break;
        //take 2 characters, turn them into bytes
        USBtxmsg.buf[(j-7)/2] = string2Byte(inString.substring(j,j+2));
        //Serial.println(inString.substring(j,j+2));    for debug
        posTracker = j;
     } 
  
    //find length
    USBtxmsg.len = (posTracker-5)/2;
    USBtxmsg.id = tempID;
  }

  //write message
  Can0.write(USBtxmsg);
}

//This function returns a byte value from a 2 character string
byte string2Byte(String chars){

  byte tempHi, tempLo, tempValue;
  switch (chars.charAt(0)) {
    case '0':
      tempHi = 0;
      // statements
      break;
    case '1':
      tempHi = 1;
      // statements
      break;
    case '2':
      tempHi = 2;
      // statements
      break;
    case '3':
      tempHi = 3;
      // statements
      break;
    case '4':
      tempHi = 4;
      // statements
      break;
    case '5':
      tempHi = 5;
      // statements
      break;
    case '6':
      tempHi = 6;
      // statements
      break;
    case '7':
      tempHi = 7;
      // statements
      break;
    case '8':
      tempHi = 8;
      // statements
      break;
    case '9':
      tempHi = 9;
      // statements
      break;
    case 'A':
      tempHi = 0x0A;
      // statements
      break;
    case 'B':
      tempHi = 0x0B;
      // statements
      break;
    case 'C':
      tempHi = 0x0C;
      // statements
      break;
    case 'D':
      tempHi = 0x0D;
      // statements
      break;
    case 'E':
      tempHi = 0x0E;
      // statements
      break;
    case 'F':
      tempHi = 0x0F;
      // statements
      break;
    default:
      tempHi = 0;
    // statements
  }
  //Serial.println(bytetoASCII(tempHi));    for debug

  switch (chars.charAt(1)) {
    case '0':
      tempLo = 0;
      // statements
      break;
    case '1':
      tempLo = 1;
      // statements
      break;
    case '2':
      tempLo = 2;
      // statements
      break;
    case '3':
      tempLo = 3;
      // statements
      break;
    case '4':
      tempLo = 4;
      // statements
      break;
    case '5':
      tempLo = 5;
      // statements
      break;
    case '6':
      tempLo = 6;
      // statements
      break;
    case '7':
      tempLo = 7;
      // statements
      break;
    case '8':
      tempLo = 8;
      // statements
      break;
    case '9':
      tempLo = 9;
      // statements
      break;
    case 'A':
      tempLo = 0xA;
      // statements
      break;
    case 'B':
      tempLo = 0xB;
      // statements
      break;
    case 'C':
      tempLo = 0xC;
      // statements
      break;
    case 'D':
      tempLo = 0xD;
      // statements
      break;
    case 'E':
      tempLo = 0xE;
      // statements
      break;
    case 'F':
      tempLo = 0xF;
      // statements
      break;
    default:
      tempLo = 0;
    // statements
  }
  //Serial.println(bytetoASCII(tempLo));    for debug

  tempValue = (tempHi<<4)+tempLo;
  //Serial.println(bytetoASCII(tempValue));   for debug
  return tempValue;
  
}

//Check if any serial characters are available in the buffer, if there are and
//it is the start of a new packet, decode and send the packet
int CheckSerial(){
  if(Serial.available()){
    inputString = Serial.readStringUntil(';');
    if((inputString.charAt(0) == ':')&&(inputString.charAt(1) == 'S'))
    {
      inputString += ';';
      //Serial.println(inputString);    for debug
      Serial2CAN(inputString);
      //digitalWrite(LED, (!digitalRead(LED)));
      return 1;
    } else return 0;
  } else return 0;
  
}

