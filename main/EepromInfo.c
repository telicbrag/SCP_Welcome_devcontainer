//
// Created by telic on 1/9/24.
//

#include <stdio.h>
#include <time.h>
#include <memory.h>
#include "EepromInfo.h"

/*
 * Decode IMEI BCD coding to string
 */
static void decodeImei(const uint8_t *src, char *dst)
{
    int8_t readIndex;
    int8_t writeIndex = 0;
    uint8_t bcd = 0;

    for ( readIndex=0; readIndex<8; readIndex++)
    {
        bcd = (src[readIndex] & 0xf0) >> 4;
        if ( bcd < 0x0a)
            dst[writeIndex++] = bcd + '0';

        bcd = (src[readIndex] & 0x0f);
        if ( bcd < 0x0a)
            dst[writeIndex++] = bcd + '0';
    }
    dst[writeIndex] = 0;
}

void convertEpochToHumanReadableTimestamp( const uint32_t epochtime, char *buffer, const uint16_t buflen)
{
    time_t rawtime = epochtime;
    struct tm ts;

    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&rawtime);
    strftime(buffer, buflen, "%a %Y-%m-%d %H:%M:%S %Z", &ts);
}



int getEepromInfo( const uint8_t  *buffer, const uint16_t buflen, EEPROMInfo *info)
{
    memset( info, 0xff, sizeof(EEPROMInfo));

    if ( (buffer[0] != 0) || (buffer[1] != 1) )
        return -1; // invalid eeprom version

    for( int idx=0; idx<buflen;idx++)
    {
        switch( buffer[idx])
        {
        case 0x00:
            info->eepromVersion    = buffer[idx+1];
            idx += 1;
            break;
        case 0x09:
            idx += 2;
            break;
        case 0x14:
            info->productCodeMajor  = (uint32_t)((buffer[idx+1] << 16) + (buffer[idx+2] << 8) + buffer[idx+3]);
            info->productCodeMinor  = (uint16_t)((buffer[idx+4] << 8) + buffer[idx+5]);
            idx += 5;
            break;
        case 0x1B:
            info->timestamp         = (uint32_t)((buffer[idx+4] << 24) + (buffer[idx+3] << 16) + (buffer[idx+2] <<8) + buffer[idx+1]);
            idx += 4;
            break;
        case 0x24:
            info->assemblyCodeMajor = (uint32_t)((buffer[idx+1] << 16) + (buffer[idx+2] << 8) + buffer[idx+3]);
            info->assemblyCodeMinor = (uint16_t)((buffer[idx+4] << 8) + buffer[idx+5]);
            idx += 5;
            break;
        case 0x37:
            decodeImei( &buffer[idx+1],info->imei);
            idx += 8;
            break;
        default:
            // end-of-info, invalid or unknown field
            idx = buflen +1; // exit
        }
    }
    return 0;
}

void printEepromInfo( EEPROMInfo info)
{
    printf( "\nEEProm information:\n");
    if ( info.eepromVersion == 1)
    {
        printf("\t%-25s %d\n", "Information version", info.eepromVersion);

        printf("\t%-25s %05ld", "Product code", info.productCodeMajor);
        if ( info.productCodeMinor != 0xffff )
            printf(".%03d", info.productCodeMinor);
        printf("\n");

        char stringbuffer[32];
        convertEpochToHumanReadableTimestamp(info.timestamp, stringbuffer, sizeof(stringbuffer));
        printf("\t%-25s %s\n", "Production timestamp", stringbuffer);


        printf("\t%-25s %05ld", "Assembly code", info.assemblyCodeMajor);
        if ( info.assemblyCodeMinor != 0xffff )
            printf(".%03d", info.assemblyCodeMinor);
        printf("\n");

        printf("\t%-25s %s\n", "IMEI", info.imei);
    }
    else
        printf( "\n*** not supported EEPROM protocol version %d\n",info.eepromVersion);
    printf("\n");
}
