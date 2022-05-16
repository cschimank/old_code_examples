
#include <nesi.h>
#include <string.h>

// frequency of fscl needed for baud rate of slave module
#define Fscl 100000

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
  while(!IFS3bits.MI2C2IF * x--); // wait for completion
}

// convert a date/time variable from decimal to Binary Coded Decimal
DateAndTime Dec_to_BCD(DateAndTime temp)
{
  temp.second = (temp.second/10*16) + (temp.second%10);
  temp.minute = (temp.minute/10*16) + (temp.minute%10);
  temp.hour   = (temp.hour  /10*16) + (temp.hour  %10);
  temp.day    = (temp.day   /10*16) + (temp.day   %10);
  temp.month  = (temp.month /10*16) + (temp.month %10);
  temp.year   = (temp.year  /10*16) + (temp.year  %10);

  return temp;
}


// convert a date/time variable from Binary Coded Decimal to decimal
DateAndTime BCD_to_Dec(DateAndTime temp)
{
  temp.second = (temp.second/16*10) + (temp.second%16);
  temp.minute = (temp.minute/16*10) + (temp.minute%16);
  temp.hour   = (temp.hour  /16*10) + (temp.hour  %16);
  temp.day    = (temp.day   /16*10) + (temp.day   %16);
  temp.month  = (temp.month /16*10) + (temp.month %16);
  temp.year   = (temp.year  /16*10) + (temp.year  %16);

  return temp;
}

// RTC read time function, returns the date and time read, sets errors for no communication
DateAndTime read_time(void)
{
  DateAndTime date_time;

  // Initiate start
  i2c_start();
  delay_us(5);

  // send address of RTC in write mode
  if(send_byte(0xD0))
      date_time.year = NULL;    // error could not communicate
  delay_us(2);

  // send first value to read (seconds)
  if(send_byte(0x00))
      date_time.minute = NULL; // error could not communicate
  delay_us(5);

  // restart bus in read mode
  i2c_restart();
  delay_us(5);

  if(send_byte(0xD1))
      date_time.day = NULL;   // error could not communicate
  delay_us(5);

  // read seconds
  date_time.second = read_data();
  delay_us(1);
  send_ack(0);
  delay_us(5);

  // read minutes
  date_time.minute = read_data();
  delay_us(1);
  send_ack(0);
  delay_us(5);

  // read hours
  date_time.hour = read_data();
  delay_us(1);
  send_ack(0);
  delay_us(5);

  // read day of the week 0-7, 0 is Sunday
  date_time.weekday = read_data();
  delay_us(1);
  send_ack(0);
  delay_us(5);

  // read day of the month
  date_time.day = read_data();
  delay_us(1);
  send_ack(0);
  delay_us(5);

  // read month
  date_time.month = read_data();
  delay_us(1);
  send_ack(0);
  delay_us(5);

  // read year
  date_time.year = read_data();
  delay_us(1);
  send_ack(1);
  delay_us(5);

  // end communication
  i2c_stop();

  // convert data
  date_time = BCD_to_Dec(date_time);

  return date_time; // return read time/date
}

// sets the RTC to time and date now, returns 1 if success, 0 if not
Boolean set_time(DateAndTime now)
{
  // convert time/date into BCD for rtc to store 
  now = Dec_to_BCD(now);

  // initiate communication
  i2c_start();
  delay_us(5);

  // send address in write mode
  if(send_byte(0xD0))
  {
      I2C2CONbits.PEN = 1;   // failed to communicate
      return 0;              // return fail
  }
  delay_us(2);

  // send first subaddress to write to (seconds)
  if(send_byte(0x00))
  {
      I2C2CONbits.PEN = 1;  // failed to communicate
      return 0;             // return fail
  }
  delay_us(5);

  // send/save the date and time
  send_byte(now.second);
  delay_us(5);
  send_byte(now.minute);
  delay_us(5);
  send_byte(now.hour);
  delay_us(5);
  send_byte(now.weekday);
  delay_us(5);
  send_byte(now.day);
  delay_us(5);
  send_byte(now.month);
  delay_us(5);
  send_byte(now.year);
  delay_us(5);

  // end communication
  i2c_stop();

  return 1; // success
}

int main()
{
  nesi.init();
  i2c_init();

  DateAndTime timeDate,ok;

  timeDate.second = 0;
  timeDate.minute = 40;
  timeDate.hour   = 8;
  timeDate.day    = 26;
  timeDate.weekday= 2;
  timeDate.month  = 5;
  timeDate.year   = 15;

  delay(100);

  // timeDate = read_time();
  set_time(timeDate);

  while(1)
  {
    ok = timeDate;
    while(button.isPressed())
    {    
      delay(1000);
      if(!button.isPressed())
        timeDate = read_time();
      dataLog.add("\n",0);
      dataLog.add(dateTime.toStamp(timeDate),0);
    }
  }
  return 0;
}