
// OSC dispatcher.  We don't want our messages to get in the way of things, so all messages get added to a queue.

#include "osc.h"

// This should probably move to nicksDebugTools.cpp or something
void forcePrintString(char * _string, uint16_t length)
{
    for (uint16_t thisChar = 0; thisChar < length; thisChar++)
        printf("%c", _string[thisChar]);
}


void printStringWithSpecialChars(char * _string, uint16_t _length)
{
    for (uint16_t thisChar = 0; thisChar < _length; thisChar++)
    {
        if (_string[thisChar] < 32 || _string[thisChar] > 127)
            printf("[%d]", _string[thisChar]);
        else
            printf("%c", _string[thisChar]);
    }
}


void printCharValues(char * _string, uint16_t _length)
{
    for (uint16_t thisChar = 0; thisChar < _length; thisChar++)
        printf("[%d]", _string[thisChar]);
}


// Wrappers for various values
bool osc_enqueueFloat(char * _address, float _value, OSCOUTGOINGQUEUE * _oscOutgoingQueue)
{
    OSCVALUE oscValue;
    oscValue.value_float = _value;
    return osc_enqueueMessage(_address, &oscValue, _oscOutgoingQueue, OSC_MESSAGETYPE_FLOAT);
}

bool osc_enqueueInt(char * _address, int _value, OSCOUTGOINGQUEUE * _oscOutgoingQueue)
{
    OSCVALUE oscValue;
    oscValue.value_int = _value;
    return osc_enqueueMessage(_address, &oscValue, _oscOutgoingQueue, OSC_MESSAGETYPE_INT);
}

bool osc_enqueueString(char * _address, char * _value, OSCOUTGOINGQUEUE * _oscOutgoingQueue)
{
    OSCVALUE oscValue;
    strcpy(oscValue.value_string, _value);
    return osc_enqueueMessage(_address, &oscValue, _oscOutgoingQueue, OSC_MESSAGETYPE_STRING);
}


// Add a message to the outgoing queue.
bool osc_enqueueMessage(char * _address, OSCVALUE * _value, OSCOUTGOINGQUEUE * _oscOutgoingQueue, uint8_t _type)
{
    for (uint8_t thisSlot = 0; thisSlot < OSC_MAXOUTGOINGMESSAGES; thisSlot++) // find an open slot
    {
        if (!_oscOutgoingQueue[thisSlot].active)  // found a slot
        {
            //printf("osc_enqueueMessage() found a slot\r\n");
            strncpy(_oscOutgoingQueue[thisSlot].address, _address, OSC_STRINGLENGTH);  // copy the address to the queue
            _oscOutgoingQueue[thisSlot].type = OSC_MESSAGETYPE_FLOAT;
            _oscOutgoingQueue[thisSlot].value = *_value;
            _oscOutgoingQueue[thisSlot].active = true;
            
            //printf("address: %s\r\n", _oscOutgoingQueue[thisSlot].address);
            //printf("type: %d\r\n", _oscOutgoingQueue[thisSlot].type);
            
            /*
            switch (_oscOutgoingQueue[thisSlot].type)
            {
                default:
                case OSC_MESSAGETYPE_INT:    printf("value: %d\r\n", _oscOutgoingQueue[thisSlot].value.value_int);    break;
                case OSC_MESSAGETYPE_FLOAT:  printf("value: %f\r\n", _oscOutgoingQueue[thisSlot].value.value_float);  break;
                case OSC_MESSAGETYPE_STRING: printf("value: %s\r\n", _oscOutgoingQueue[thisSlot].value.value_string); break;
            }
            
            printf("active: %d\r\n", _oscOutgoingQueue[thisSlot].active);
            */
            
            return true;
        }
    }

    return false;
}


union OSCFLOAT
{
    float f;
    char fToChar[4];
};


// send all outgoing messages
uint16_t osc_processOutgoingQueue(OSCOUTGOINGQUEUE * _oscOutgoingQueue)
{
    // TODO: check for exceeded string buffers, or just allocate it dynamically
    for (uint8_t thisMessage = 0; thisMessage < OSC_MAXOUTGOINGMESSAGES; thisMessage++)
    {
        if (_oscOutgoingQueue[thisMessage].active)
        {
            //printf("oh, look, there's a pending outgoing message in the OSC queue.  Let's send it !!!\r\n");
            // look, yo.  Eventually this will go in its own function, but not now, ffs.
            char out_buffer[(OSC_STRINGLENGTH * 2) + 4];  // Where the heck did I come up with this value?
            // Zero out this string (thanks Adam (wooooooo))
            memset(out_buffer, 0, (OSC_STRINGLENGTH * 2) + 4);
            
            // first, add the address
            uint8_t addressLength = strlen(_oscOutgoingQueue[thisMessage].address);
            strcpy(out_buffer, _oscOutgoingQueue[thisMessage].address);
            
            // everything in OSC-land has to happen in groups of four bytes.
            // We need to fill the buffer with no fewer than four bytes, until the total length is a multiple of four.
            // Instead of actually writing zeros to the buffer, we'll just specify the offset for the next part of the packet.
            uint8_t addressBytesUntilWord = 4 - (addressLength - ((addressLength >> 2) << 2));
            uint8_t typeOffset = addressLength + addressBytesUntilWord;
            
            //printf("the osc message so far: %s\r\n", out_buffer);
            //printf("addressBytesUntilWord = %d\r\n", addressBytesUntilWord);
            //printf("typeOffset = %d + %d = %d\r\n", addressLength, addressBytesUntilWord, typeOffset);
            
            // add the type symbol
            char * typeSymbol;
            bool abortSend = false;
            switch (_oscOutgoingQueue[thisMessage].type)
            {
                case OSC_MESSAGETYPE_FLOAT:  typeSymbol = ",f"; break;
                case OSC_MESSAGETYPE_STRING: typeSymbol = ",s"; break;
                case OSC_MESSAGETYPE_INT:    typeSymbol = ",i"; break;
                default:
                {
                    printf("osc_processOutgoingQueue ERROR: no type specified!  Cannot send.");
                    typeSymbol = "";   // To suppress warnings.  I'm a lazy, lazy programmer
                    abortSend = true;
                }
            }
            
            strcpy(&out_buffer[typeOffset], typeSymbol);
            
            //printf("the osc message with type:  ");
            //printStringWithSpecialChars(out_buffer, typeOffset + 2);
            //printf("\r\n");
            
            OSCFLOAT oscFloat;
            oscFloat.f = _oscOutgoingQueue[thisMessage].value.value_float;  // TODO: this only works with float right now
            
            //printf("oscFloat.fToChar = ");
            //printCharValues(oscFloat.fToChar, 4);
            //printf("\r\n");
            
            // we do it this way to fix endian-ness
            for (uint8_t thisByte = 0; thisByte < 4; thisByte++)
                out_buffer[typeOffset + 4 + (3 - thisByte)] = oscFloat.fToChar[thisByte];
            
            //printf("the osc message with value:  ");
            //printStringWithSpecialChars(out_buffer, typeOffset + 8);
            //printf("\r\n");
            
            //sprintf(out_buffer, "/1/victory%c%c%c%c,f%c%c%c%c%c%c", 0,0,0,0,0, 0, 0x3f, 0x80, 0, 0);
            if (!abortSend)
            {
                //printf("this is what we are sending: ");
                //printStringWithSpecialChars(out_buffer, typeOffset + 8);
                //printf("\r\n");
                
                // OSC includes null characters, so everything becomes a little more complicated
                for (uint8_t thisChar = 0; thisChar < typeOffset + 8; thisChar++)
                  Serial.print(out_buffer[thisChar]);
                
                //printf("sent\r\n");
            }
            
            // We're done with this message; free it up so that someone else can use it
            _oscOutgoingQueue[thisMessage].active = false;
        }
    }
    
    return true;
}


#define OSC_BUFFER_LENGTH 128
char oscBuffer[OSC_BUFFER_LENGTH];
uint8_t oscCursor = 0;

void resetOSCBuffer()
{
  for (uint8_t thisChar = 0; thisChar < OSC_BUFFER_LENGTH; thisChar++)
    oscBuffer[thisChar] = 0;
    
  oscCursor = 0;
}


bool addCharToBuffer(char _char)
{
  if (oscCursor < OSC_BUFFER_LENGTH)
  {
    oscBuffer[oscCursor] = _char;
    oscCursor++;
  }
  else
    return false;
    
  return true;
}

bool oscMessageInProgress = false;
bool oscLookingForTypes   = false;

void handleOSCChar(char _char)
{
  switch (_char)
  {
    case '/':  
    {
      if (!oscMessageInProgress)
      {
        Serial.println("found a slash!");
        resetOSCBuffer();
        oscMessageInProgress = true;
      }
      
      break;
    }
    case ',':
    {
      Serial.print("The address is: ");
      Serial.println(oscBuffer);
      
      oscLookingForTypes = true;
      
      break;
    }
    case 'f':
    {
      if (oscLookingForTypes)
      {
        Serial.println("Found a float!");
      }
      
      break;
    }
    case 'i':
    {
      if (oscLookingForTypes)
      {
        Serial.println("Found an int!");
      }
      
      break;
    }
    case 's':
    {
      if (oscLookingForTypes)
      {
        Serial.println("Found a string!");
      }
      
      break;
    }
    case 0:
    case '*':  // TODO: for fuck's sake, delete me!
    {
      if (oscLookingForTypes)
      {
        Serial.println("No more types!");
        oscLookingForTypes = false;
      }
    }
    default: break;
  }
  
  addCharToBuffer(_char);
}
