/* 
 * File:   filecsv.c
 * Author: Fabien AMELINCK
 *
 * Created on 17 juin 2021
 */

#include "filecsv.h"

void DeleteDataFile(void)
{
	int del = remove("DataFile");
	//int del = remove("./data.csv");
	
	if (!del)
	{
		printf("Mesurement data file deleted\n");
	}
	else
	{
		printf("Mesurement data file couldn't be deleted\n");
	}
}

void CreateDataFile(void)
{
	FILE *df;
	df = fopen("DataFile", "a+");
	//df = fopen("./data.csv", "a+");
	
	
	if (df == NULL)
	{
		printf("Unable to create data file\n");
		exit(EXIT_FAILURE);
	}
	
	fprintf(df, "Date;Time;Network_ID;Node_ID;Data;\n");
//	fprintf(df, "Hygrometry[0];Hygrometry[1];Hygrometry[2];Hygrometry[3];Hygrometry[4];Hygrometry[5];Hygrometry[6];Hygrometry[7];Hygrometry[8];Hygrometry[9];Hygrometry[10];Hygrometry[11];");
//	fprintf(df, "Temperature[0];Temperature[1];Temperature[2];Temperature[3];Temperature[4];Temperature[5];Temperature[6];Temperature[7];Temperature[8];Temperature[9];Temperature[10];Temperature[11];");
//	fprintf(df, "Battery_Voltage;\n");
	
	//fprintf(df, "Date;Time;Network ID;Node ID;Sensor ID;Hygrometry[0];Hygrometry[1];Hygrometry[2];Hygrometry[3];Hygrometry[4];Hygrometry[5];Hygrometry[6];Hygrometry[7];Hygrometry[8];Hygrometry[9];Hygrometry[10];Hygrometry[11];Temperature[0];Temperature[1];Temperature[2];Temperature[3];Temperature[4];Temperature[5];Temperature[6];Temperature[7];Temperature[8];Temperature[9];Temperature[10];Temperature[11];Battery Voltage;");
	//fprintf(df, "%c",10);		// add Line Feed in the file
	// note: \n could be used in the 1st fprintf line
	
	fclose(df);
}

void WriteDataInFile(const uint8_t *NodeID, const uint8_t *NbBytesReceived, const uint8_t *NodeData)
{
    FILE *df;
	
	df = fopen("DataFile", "a+");
	//df = fopen("./data.csv", "a+");
	
	if (df == NULL)
	{
		printf("Unable to open data file\n");
		exit(EXIT_FAILURE);
	}

	int hour, min, sec, day, month, year;
  	time_t now;
	//char date[255]="";
    
	//char current_date = time(&now);				// returns the current date (unused here)
	printf("%s", ctime(&now));
	struct tm *local = localtime(&now);			// converts to local time

	hour = local->tm_hour;        
	min = local->tm_min;       
	sec = local->tm_sec;       
	day = local->tm_mday;          
	month = local->tm_mon + 1;     
	year = local->tm_year + 1900;  

	/*date[0]=day;
	date[1]=month;
	date[2]=year;
	date[3]=hour;
	date[4]=min;
	date[5]=sec;*/
	

	fprintf(df, "%02d/%02d/%d; ", day, month, year);
	fprintf(df, "%02d:%02d:%02d; ", hour, min, sec);
	fprintf(df, "%02d; %02d; ", NetID, NodeID);
	
	for (uint8_t x = 0; x < (NbBytesReceived - 4); x++)
	{
		fprintf(df, "0x%X; ", NodeData[x]);
	}
	fprintf(df, "\n");

/*	
	for (uint8_t i = 0; i < 12; i++)
	{
		fprintf(df, "%X;", Hygrometry[i]);
	}
	for (uint8_t i = 0; i < 12; i++)
	{
		fprintf(df, "%X;", Temperature[i]);
	}
	fprintf(df, "%X\n", BatteryVoltage);
*/	
	fclose(df);
}

void WriteResetInFile(void)
{
    FILE *df;
	
	df = fopen("DataFile", "a+");
	//df = fopen("./data.csv", "a+");
	
	if (df == NULL)
	{
		printf("Unable to open data file\n");
		exit(EXIT_FAILURE);
	}

	int hour, min, sec, day, month, year;
  	time_t now;
	//char date[255]="";
    
	//char current_date = time(&now);				// returns the current date (unused here)
	printf("%s", ctime(&now));
	struct tm *local = localtime(&now);			// converts to local time

	hour = local->tm_hour;        
	min = local->tm_min;       
	sec = local->tm_sec;       
	day = local->tm_mday;          
	month = local->tm_mon + 1;     
	year = local->tm_year + 1900;  

	/*date[0]=day;
	date[1]=month;
	date[2]=year;
	date[3]=hour;
	date[4]=min;
	date[5]=sec;*/
	

	fprintf(df, "%02d/%02d/%d; ", day, month, year);
	fprintf(df, "%02d:%02d:%02d; ", hour, min, sec);
	fprintf(df, "LORa module reset\n");
	
	fclose(df);
}
