                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /* File to define the eeprom variable save/restore operations for the ASCOM switch web driver */
#ifndef _SnglEncoder_EEPROM_H_
#define _SnglEncoder_EEPROM_H_

#include "DebugSerial.h"
#include "eeprom.h"
#include "EEPROMAnything.h"

const byte magic = '*';

//definitions
void setDefaults(void );
void saveToEeprom(void);
void setupFromEeprom(void);

void setDefaults( void )
{
  DEBUGSL1( "Eeprom setDefaults: entered");
  
  if( myHostname != nullptr )
    free(myHostname);
  myHostname = (char* )calloc( sizeof (char), MAX_NAME_LENGTH );
  strcpy( myHostname, defaultHostname );
  
  if( thisID != nullptr )
    free(thisID);
  thisID = (char* )calloc( sizeof (char), MAX_NAME_LENGTH );
  strcpy( thisID, defaultHostname );
  
  //Encoder-specific startup defaults 
  encPresent = false;
  position = 0;
  lastPosition = 0;
  ppr = 400 * 4;              //List ppr * number of edges = number of pulses in a rev of the encoder wheel
  wheelDiameter = 61.3;         //mm - may need to be a float. 
  ppRollover = (int)(2700/wheelDiameter)*ppr; //Dome wheels size. 
//  ppRollover = __INT_MAX__;     //Natural rollover for  32 bit signed integer (limits.h)
  //(long int)( ppr * (2700/wheelDiameter)) ;//Count at which we complete a revolution of the dome.
  homeFlag = false;
 
  DEBUGSL1( "setDefaults: exiting" );
}

void saveToEeprom( void )
{
  int eepromAddr = 0;

  DEBUGSL1( "savetoEeprom: Entered ");
    
  //hostname
  EEPROMWriteString( eepromAddr=4, myHostname, MAX_NAME_LENGTH );
  DEBUGS1( "Written hostname: ");DEBUGSL1( myHostname );
  eepromAddr += MAX_NAME_LENGTH;
  
  //Position
  EEPROMWriteAnything( eepromAddr, position );
  eepromAddr += sizeof(position);  
  DEBUGS1( "Written position: ");DEBUGSL1( position );
  
  EEPROMWriteAnything( eepromAddr, ppRollover );
  eepromAddr += sizeof( ppRollover );
  DEBUGS1( "Written ppRollover: ");DEBUGSL1( ppRollover );
    
  EEPROMWriteAnything( eepromAddr, ppr );
  eepromAddr += sizeof( ppr );
  DEBUGS1( "Written ppr: ");DEBUGSL1( ppr );

      
  EEPROMWriteAnything( eepromAddr, wheelDiameter );
  eepromAddr += sizeof( wheelDiameter );
  DEBUGS1( "Written wheelDiameter: ");DEBUGSL1( wheelDiameter );
  
  EEPROM.put( 0, (byte) magic );
  
  EEPROM.commit();

 #define DEBUG_EEPROM
 #if defined DEBUG_EEPROM
  DEBUGSL1( "EEPROM content check:" );
  String output = "";
  char inchar;
  for (int i = 0; i< 256; i++)
  {
    inchar = (char) EEPROM.read( i );
    if( inchar == '\0' )
       inchar = '~';
    
    output += inchar;
    if( i%32 == 0 ) 
      output += "\n\r";
  }
  DEBUGSL1( output );
  
 #endif
  DEBUGSL1( "saveToEeprom: exiting ");
}

void setupFromEeprom( void )
{
  int eepromAddr = 0;
  char bTemp = '\0';
  int i=0;
  char newName[MAX_NAME_LENGTH];
    
  DEBUGSL1( "setUpFromEeprom: Entering ");
  
  //Setup internal variables - read from EEPROM.
  EEPROM.get( eepromAddr=0, bTemp );
  DEBUGS1( "Read magic: ");DEBUGSL1( bTemp );
  
  //Sets up ram allocations etc.
  if ( (char) bTemp != magic ) //initialise
  {
    setDefaults();
    saveToEeprom();
    DEBUGSL1( "Failed to find init magic byte - wrote defaults ");
    return;
  }    
    
  DEBUGSL1( "Reading hostname from eeprom");
  //hostname - directly into variable array 
  EEPROMReadString( eepromAddr = 4, newName, MAX_NAME_LENGTH );
  if( myHostname != nullptr ) 
    free( myHostname );
  myHostname = (char*) calloc( sizeof(char), MAX_NAME_LENGTH );
  strcpy( myHostname, newName );
  DEBUGS1( "Read hostname: ");DEBUGSL1( myHostname );
  
  if( thisID != nullptr ) 
    free( thisID );
  thisID = (char*) calloc( sizeof(char), MAX_NAME_LENGTH );
  strcpy( thisID, newName );
  DEBUGS1( "Read thisID: ");DEBUGSL1( thisID );
  eepromAddr  += MAX_NAME_LENGTH;
  
  EEPROMReadAnything( eepromAddr, position );
  eepromAddr += sizeof( position );
  DEBUGS1( "Read position: ");DEBUGSL1( position );

  EEPROMReadAnything( eepromAddr, ppRollover );
  eepromAddr += sizeof( ppRollover );
  DEBUGS1( "Read ppRollover: ");DEBUGSL1( ppRollover );

  EEPROMReadAnything( eepromAddr, ppr );
  eepromAddr += sizeof( ppr );
  DEBUGS1( "Read ppr: ");DEBUGSL1( ppr );

  EEPROMReadAnything( eepromAddr, wheelDiameter );
  eepromAddr += sizeof( wheelDiameter );
  DEBUGS1( "Read wheelDiameter: ");DEBUGSL1( wheelDiameter );
  
  DEBUGSL1( "setupFromEeprom: exiting" );
}
#endif
