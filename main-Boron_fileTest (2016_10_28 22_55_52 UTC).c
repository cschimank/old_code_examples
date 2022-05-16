#include <nesi.h>
#include <string.h>

DateAndTime getTimeFromFile(String filename)									
{																					
	char fileTime[24] = {0};													
	FSFILE* timeFile = FSfopen(filename, "r");									
	if(!timeFile) return dateTime.new(0,1,1,0,0,0);
	FSfread(fileTime, sizeof(char), 24, timeFile);								
	FSfclose(timeFile);															
																				
	return dateTime.parseStamp(fileTime);										
}																				
																						
//Put time stamp "Time" into file "filename"									
void putTimeToFile(String filename, DateAndTime Time)							
{
    String time = dateTime.toStamp(Time);
	FSFILE* timeFile = FSfopen(filename, FS_WRITE);
	if(!timeFile) return;														
	FSfprintf(timeFile, "%s", time);
	FSfclose(timeFile);																
}																				
																				
//Chech SD card for a file named filename										
Boolean checkForFile(String filename)											
{																				
	FSFILE* file = FSfopen(filename, "r");																
	if(!file) return 0;	// no file could be read/found
	FSfclose(file);
	return 1;			// file found				
}

int main (void)
{
	//initialize NESI+ systems
	nesi.init();

	DateAndTime StartTime;
	DateAndTime CurrentTime;
	//  DataLog logfile;

	CurrentTime.second = 0;
	CurrentTime.minute = 42;
	CurrentTime.hour = 2;
	CurrentTime.year = 15;
	CurrentTime.day = 28;
	CurrentTime.month = 4;
	CurrentTime.weekday = 2;


//	if(checkForFile("time.txt"))
//		CurrentTime = getTimeFromFile("time.txt");
//        else
//            putTimeToFile("time.txt",CurrentTime);
//	wait(1000);
//
//	if(!checkForFile("StartTime.txt"))
//	{
//		putTimeToFile("StartTime.txt",CurrentTime);
//		StartTime = CurrentTime;
//	}
//	else
//		StartTime = getTimeFromFile("StartTime.txt");
//

	dateTime.set(CurrentTime);
        
	while(1)
	{
//		if(button.isPressed())
		{
                    CurrentTime = dateTime.get();
                    StartTime;
//			putTimeToFile("time.txt",CurrentTime);
//			wait(1000);
//			usb.connect();
//			while(!button.isPressed())
//			{
//				usb.process();
//			}
//			usb.disconnect();
  		}

	}
}