//NESI+ BEE I2C Test for switch
//Author: Colton Schimank

//Include NESI+ library for peripheral function usage
#include <nesi.h>
#include <uart2.h>
#include <string.h>
#include <math.h>

// I2C Functions -------------------------------------------------------------------

// frequency of fscl needed for baud rate of slave module
#define Fscl 100000
#define slave_addr 0x1A

//set the I2C bus to an idle state
//function initiates I2C2 to clock rate needed for slave module(Scl)
void i2c_init(void)
{
    int data;
    
    // set bus to idle
    I2C2CONbits.I2CEN = 0;    // disable I2C for reconfig
    I2C2CONbits.I2CSIDL = 0;  // continue module operation in idle mode
    I2C2CONbits.DISSLW = 1;   // disable slew rate
    I2C2CONbits.I2CEN = 1;    // enable I2C2 

    // set baud rate specified in datasheet, FCY is the NESI clock
    I2C2BRG =  (FCY/Fscl)-(FCY/10000000)-1;
    // clear buffer
    data = I2C2RCV;
}

// Initialtes the i2c start command, returns 1 if success, 0 if not
Boolean i2c_start(void)
{
  int x = 10; 

  // Clear flags
  IFS3bits.MI2C2IF = 0;
  I2C2STATbits.IWCOL = 0;

  // Enable Start Command
  I2C2CONbits.SEN = 1;

  // Wait for start command to be complete
  while(I2C2CONbits.SEN);
  while((!IFS3bits.MI2C2IF) * x--);

//  if(!I2C2CONbits.S)  // If no slave detects start, fail
//    return 0;

  return 1; // success
}

// Initialtes the i2c restart command, returns 1 if success, 0 if not
int i2c_restart(void)
{
  int x = 10; 

  // Clear flags
  IFS3bits.MI2C2IF = 0;
  I2C2STATbits.IWCOL = 0;

  // Enable Restart Command
  I2C2CONbits.RSEN = 1;

  // Wait for restart command to be complete
  while(I2C2CONbits.RSEN);
  while((!IFS3bits.MI2C2IF) * x--);

//  if(!I2C2CONbits.S)  // If no slave detects start, fail
//    return 0;

  return 1; // success
}

// Sends one byte of data over the i2c bus, returns acknowledgement from slave
// ACK = 0, NACK = 1, unsuccessful = 2
int send_byte(char byte)
{
  int x = 0;
  int ackStat;
  // Clear flags
  IFS3bits.MI2C2IF = 0;
  while(I2C2STATbits.TBF); // To avoid collisions

  // Send byte
  I2C2TRN = byte; 

  // Collision control
  while(I2C2STATbits.IWCOL)
  {  
    delay(1); //error, collision transmitting try again
    I2C2STATbits.IWCOL = 0;
    I2C2TRN = byte;
    x++;
    if(x == 3)
     return 2;
  }

  // Wait until done sending
  while(I2C2STATbits.TBF);
  delay_us(10);

  // get slave acknowledgement status
  ackStat = I2C2STATbits.ACKSTAT;

  // Wait for ack/nack from slave
  while((!IFS3bits.MI2C2IF) * x--);

  return ackStat; // return ack/nack from slave
}

// master sends an ack/nack, send_ack(1) is a nack, send_ack(0) is ack
void send_ack(Boolean ack)
{
  int x = 30;

  // clear flag
  IFS3bits.MI2C2IF = 0;

  // set ack value to nack or ack
  I2C2CONbits.ACKDT = ack;
  delay_us(1);

  // initiate ack value send
  I2C2CONbits.ACKEN = 1;

  // wait until done sending
  while(I2C2CONbits.ACKEN * x--);
  x = 3;
  while((!IFS3bits.MI2C2IF) * x--);
}

// reads byte of incoming data 
char read_data(void)
{
  int x = 100;
  char dataRead;

  // clear flag
  IFS3bits.MI2C2IF = 0;
  delay_us(1);

  // initiate receive
  I2C2CONbits.RCEN = 1;

  // wait for data transfer to finish
  while((I2C2CONbits.RCEN) * x--);

  // put data from buffer to dataRead
  dataRead = I2C2RCV;
  if(I2C2STATbits.I2COV)  // if fail to read try again
    dataRead = I2C2RCV;

  // ONLY IMPLEMENT if slave packet will never = 0xff
  if(x==0)                // if transfer did not finish
    return 0xff;          // return 0xff

  // clear flag
  I2C2STATbits.I2COV = 0;

  return dataRead;      // return read data
}

// Initialte a stop command
void i2c_stop(void)
{
  int x = 10;

  IFS3bits.MI2C2IF = 0; // clear flag
  I2C2CONbits.PEN = 1;  // initiate stop
  while((!IFS3bits.MI2C2IF) * x--); // wait for completion
}

// EEE read Ethernet Controller
void read_EthCont(void)
{
  char stat[128] = {0};
  char temp = 0x00;
  usb.printf("Start I2C\n\r");

  // Initiate start
  i2c_start();
  delay_us(5);

  // send address of RTC in write mode
  if(send_byte(slave_addr)){
    usb.printf("Cannt find slave!\n\r"); // error could not communicate
  }
         
  delay_us(2);

  // send first value to read
  if(send_byte(0x10)){
    usb.printf("error 1\n\r"); // error could not communicate
  }

  delay_us(5);
  // read seconds
  // temp = read_data();

  
  delay_us(1);
  send_ack(0);
  delay_us(5);

  // end communication
  i2c_stop();

  sprintf(stat, temp);
  usb.printf(stat);
}

// sets the RTC to time and date now, returns 1 if success, 0 if not
Boolean set_time(DateAndTime now)
{
  return 1;
}

// File functions ------------------------------------------------------------

DateAndTime getTimeFromFile(String filename)                  
{                                         
  char fileTime[24] = {0};                          
  FSFILE* timeFile = FSfopen(filename, "r");                  
  if(!timeFile) return dateTime.new(0,0,0,0,0,0);               
  FSfread(fileTime, sizeof(char), 24, timeFile);                
  FSfclose(timeFile);                             
                    
  return dateTime.parseStamp(fileTime);                   
}                                       
                      
//Put time stamp "Time" into file "filename"                  
void putTimeToFile(String filename, DateAndTime Time)
{                                         
  FSFILE* timeFile = FSfopen(filename, "w");
  if(!timeFile) return;                           
  FSfprintf(timeFile, "%s", dateTime.toStamp(Time));
  FSfclose(timeFile);                               
}                                       
                  
//Chech SD card for a file named filename                   
Boolean checkForFile(String filename)                     
{                                       
  FSFILE* file = FSfopen(filename, "r");                                
  if(!file) return 0; // no file could be read/found
  FSfclose(file);
  return 1;     // file found       
}

char dat[70];

void logdata(String str)
{
  FSFILE* timeFile = FSfopen("dataLog.txt", FS_APPEND);
  if(!timeFile) return;                           
  FSfprintf(timeFile, str);              
  FSfclose(timeFile);  
}

/*****************************  MAIN  *****************************/

int main (void)
{
  //initialize NESI+ systems
  nesi.init();
  i2c_init();
  uart2.init();

//  usb.connect();

  char input[64] = {0};
  int bytesRead = 0;

  wait(3000);
//  usb.printf("Start Shit!!\n\r");
  while(1)
  {
    wait(1000);
    if(button.isPressed())//(bytesRead = usb.read(input,64)))
      {
        // usb.printf(input);
        // if(bytesRead > 0)
          read_EthCont();
        // input[bytesRead] = '\0'; // terminate string
      }
    i2c_start();
    delay_us(5);
    send_byte(0x1A);
    delay_us(5);
    send_byte(0x11);
    delay_us(5);
    i2c_stop();
      
  }

  return 0;

}



