#ifndef __OSC_H
#define __OSC_H

#include "arduino.h"

#define OSC_MESSAGETYPE_FLOAT  1
#define OSC_MESSAGETYPE_INT    2
#define OSC_MESSAGETYPE_STRING 3

#define OSC_MAXOUTGOINGMESSAGES 3
#define OSC_STRINGLENGTH        64

union OSCVALUE
{
    float value_float;
    int   value_int;
    char  value_string[OSC_STRINGLENGTH];
};

typedef struct
{
    char address[OSC_STRINGLENGTH];
    uint8_t type;
    OSCVALUE value;
    bool active;
} OSCOUTGOINGQUEUE;

void forcePrintString(char * _string, uint16_t length);
void printStringWithSpecialChars(char * _string, uint16_t _length);
void printCharValues(char * _string, uint16_t _length);

bool osc_enqueueString (char * _address, char * _value, OSCOUTGOINGQUEUE * _oscOutgoingQueue);
bool osc_enqueueInt    (char * _address, int _value, OSCOUTGOINGQUEUE * _oscOutgoingQueue);
bool osc_enqueueFloat  (char * _address, float _value, OSCOUTGOINGQUEUE * _oscOutgoingQueue);
bool osc_enqueueMessage(char * _address, OSCVALUE * _value, OSCOUTGOINGQUEUE * _oscOutgoingQueue, uint8_t _type);

uint16_t osc_processOutgoingQueue(OSCOUTGOINGQUEUE * _oscOutgoingQueue);

void handleOSCChar(char _char);
bool addCharToBuffer(char _char);
void resetOSCBuffer();

#endif
