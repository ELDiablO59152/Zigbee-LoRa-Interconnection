/* 
 * File:   filecsv.h
 * Author: Fabien AMELINCK
 *
 * Created on 17 juin 2021
 */

#ifndef _FILECSV_H
#define	_FILECSV_H

#include <stdio.h> //FILE
#include <stdlib.h> //EXIT
#include <stdint.h> //uint
#include <time.h>

#define NetID 0x01
#define DataFile ./data.csv

void DeleteDataFile(void);
void CreateDataFile(void);
void WriteDataInFile(const uint8_t *NodeID, const uint8_t *NbBytesReceived, const uint8_t *NodeData);
void WriteResetInFile(void);

#endif	/* _FILECSV_H */