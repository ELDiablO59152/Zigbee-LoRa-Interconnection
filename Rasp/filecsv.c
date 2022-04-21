/*
 * File:   filecsv.c
 * Author: Fabien AMELINCK
 *
 * Created on 13 April 2022
 */

#include "filecsv.h"

void DeleteDataFile(void)
{
    int del = remove(DataFile);
    // int del = remove("./data.csv");

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
    df = fopen(DataFile, "a+");
    // df = fopen("./data.csv", "a+");

    if (df == NULL)
    {
        printf("Unable to create data file\n");
        exit(EXIT_FAILURE);
    }

    fprintf(df, "Date;Time;Network_ID;Node_ID;Data;\n");

    // fprintf(df, "Date;Time;Network ID;Node ID;Sensor ID;");
    // fprintf(df, "%c",10);		// add Line Feed in the file
    // note: \n could be used in the 1st fprintf line

    fclose(df);
}

void WriteDataInFile(const uint8_t *NodeID, const uint8_t *NbBytesReceived, const uint8_t *NodeData, const int8_t *RSSI)
{
    FILE *df;

    df = fopen(DataFile, "a+");
    // df = fopen("./data.csv", "a+");

    if (df == NULL)
    {
        printf("Unable to open data file\n");
        exit(EXIT_FAILURE);
    }

    int hour, min, sec, day, month, year;
    time_t now = time(0);
    // char date[255]="";

    // char current_date = time(&now);				// returns the current date (unused here)
    printf("%s", ctime(&now));
    struct tm *local = localtime(&now); // converts to local time

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
    fprintf(df, "%02d; %02d; ", NetID, (int)*NodeID);

    for (uint8_t x = 0; x < (*NbBytesReceived - 4); x++)
    {
        fprintf(df, "0x%X; ", NodeData[x]);
    }
    fprintf(df, "%d; ", *RSSI);
    fprintf(df, "\n");

    fclose(df);
}

void WriteResetInFile(void)
{
    FILE *df;

    df = fopen(DataFile, "a+");
    // df = fopen("./data.csv", "a+");

    if (df == NULL)
    {
        printf("Unable to open data file\n");
        exit(EXIT_FAILURE);
    }

    int hour, min, sec, day, month, year;
    time_t now = time(0);
    // char date[255]="";

    // char current_date = time(&now);				// returns the current date (unused here)
    printf("%s", ctime(&now));
    struct tm *local = localtime(&now); // converts to local time

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
