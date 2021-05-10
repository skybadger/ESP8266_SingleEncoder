/*
File to be included into relevant device REST setup 
*/
//Assumes Use of ARDUINO ESP8266WebServer for entry handlers
#if !defined _ESP8266_SNGLENC_H_
#define _ESP8266_SNGLENC_H_

#include "JSONHelperFunctions.h"

//Handlers used for setup webpage.
void handleSetup             ( void );
void handleSetupHostnamePut  ( void );
void handleSetupResolutionPut( void );
void handleSetupRolloverPut  ( void );
void handleSetupPosition     ( void );

//Local functions
String& setupFormBuilder( String& htmlForm, String& errMsg );

//Non ASCOM functions
/*
 * String& setupFormBuilder( String& htmlForm )
 */
 void handleSetup(void)
 {
  String output = "";
  String err = "";
  output = setupFormBuilder( output, err );
  server.send(200, "text/html", output );
  return;
 }

//Don't forget MQTT ID aligns with hostname too 
void handleSetupHostnamePut( void ) 
{
  String form;
  String errMsg;
  String newName;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleSetupHostnamePut" );
  
  //throw error message
  if( server.hasArg("hostname") )
  {
    newName = server.arg("hostname");
    DEBUGS1( "new hostname:" );DEBUGSL1( newName );
  }
  if( newName != nullptr && newName.length() > 0 &&  newName.length() < MAX_NAME_LENGTH )
  {
    //save new hostname and cause reboot - requires eeprom read at setup to be in place.  
    strcpy( myHostname, newName.c_str() );
    strcpy( thisID,     newName.c_str() );  
    server.send( 200, "text/html", "rebooting!" ); 

    //Write to EEprom
    saveToEeprom();
    device.restart();
  }
  else
  {
    errMsg = "handleSetupHostnamePut: Error handling new hostname";
    DEBUGSL1( errMsg );
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form ); 
  }
}

void handleEncoderSetupPut( void ) 
{
  int i=0;
  int returnCode = 200;
  String form;
  String errMsg;
  int newPosition = 0;
  int newResolution;
  float newDiameter;
  int newRollover;
 
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleEncoderSetupPut" );
  String argsToSearchFor[] = {"position", "resolution", "rollover", "wheelDiameter" };
  
  //throw error message
  for ( i = 0; i < (int) sizeof(argsToSearchFor); i++ )
  {
    if ( !hasArgIC( argsToSearchFor[i] , server, false) )
    {
      returnCode = 403;
      errMsg = "Not all args found"; //Should'nt happen from our own form! 
      form = setupFormBuilder( form, errMsg );
      server.send( returnCode, "text/html", form ); 
    }
  }
  
  newPosition = server.arg( argsToSearchFor[0] ).toInt();
  if ( ! (newPosition >= 0 && newPosition < __INT_MAX__ -1 && newPosition < ppRollover ) ) 
  {
      returnCode = 403;
      errMsg = "Not all args found"; //Should'nt happen from our own form! 
      form = setupFormBuilder( form, errMsg );
      server.send( returnCode, "text/html", form ); 
  }
  newResolution = server.arg(argsToSearchFor[1]).toInt();
  if( !( newResolution > 0 && newResolution <= __INT_MAX__ ) )
  {
      returnCode = 403;
      errMsg = "Resolution out of range "; 
      form = setupFormBuilder( form, errMsg );
      server.send( returnCode, "text/html", form ); 
  }
  newRollover = server.arg(argsToSearchFor[2]).toInt();
  if( !( newRollover > 0 && newRollover <= __INT_MAX__ ) && newRollover >= ppr )
  {
      returnCode = 403;
      errMsg = "Rollover out of range , 0< rollover <= ppr <= __INT_MAX__"; 
      form = setupFormBuilder( form, errMsg );
      server.send( returnCode, "text/html", form ); 
  } 
  newDiameter = (float) server.arg(argsToSearchFor[3]).toDouble();
  if( !( newDiameter > 0.0 && newDiameter <= (float)__INT_MAX__ ) )
  {
      returnCode = 403;
      errMsg = "Wheel diameter out of range; 0.0F < wheel diameter < __INT_MAX__"; 
      form = setupFormBuilder( form, errMsg );
      server.send( returnCode, "text/html", form ); 
  }

  //Update from valid values
  position = newPosition;
  enc.write(position);
  ppr = newResolution;
  wheelDiameter = newDiameter;
  ppRollover = newRollover;

  saveToEeprom();

  returnCode = 200;
  errMsg = ""; 
  form = setupFormBuilder( form, errMsg );
  server.send( returnCode, "text/html", form ); 
 
  DEBUGSL1( "Exiting handleSetupPositionPut" );
}

/*
 * Handler for setup dialog - issue call to <hostname>/setup and receive a webpage
 * Fill in form and submit and handler for each form button will store the variables and return the same page.
 optimise to something like this:
 https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/
 Bear in mind HTML standard doesn't support use of PUT in forms and changes it to GET so arguments get sent in plain sight as 
 part of the URL.
 */
String& setupFormBuilder( String& htmlForm, String& errMsg )
{
  htmlForm = "<!DocType html><html><head></head><meta></meta><body>\n";
  if( errMsg != NULL && errMsg.length() > 0 ) 
  {
    htmlForm +="<div bgcolor='A02222'><b>";
    htmlForm.concat( errMsg );
    htmlForm += "</b></div>";
  }
  
  //Hostname
  htmlForm += "<h1> Enter new hostname for Encoder interface</h1>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/encoder/hostname\" method=\"PUT\" id=\"hostname\" >\n";
  htmlForm += "Changing the hostname will cause the device to reboot and may change the address!\n<br>";
  htmlForm += "<input type=\"text\" name=\"hostname\" value=\"";
  htmlForm.concat( myHostname );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form>\n";
  
  //Position
  htmlForm += "<form action=\"/encoder/setup\" method=\"PUT\" >";
  htmlForm += "<formset><legend>Encoder Settings</legend>";
  htmlForm += "<label for=\"position\"> <span>Enter new position for encoder</span></label>";
  htmlForm += "<input type=\"number\" min=\"0\" name=\"position\" value=\"";
  htmlForm.concat( position );
  htmlForm += "\">";

  //Resolution - ppr
  htmlForm += "<label for=\"resolution\"> <span>Enter new resolution (ppr)</span></label>";
  htmlForm += "<input type=\"number\" min=\"1\" name=\"resolution\" value=\"";
  htmlForm.concat( ppr );
  htmlForm += "\">";
 
  //Rollover count
  htmlForm += "<label for=\"rollover\"> <span>Enter new rollover count (ppr)</span></label>";
  htmlForm += "<input type=\"number\" min=\"";
  htmlForm += ppr;
  htmlForm += "\" max=\"";
  htmlForm += __INT_MAX__;
  htmlForm += "\" name=\"rollover\" value=\"";
  htmlForm.concat( ppRollover );
  htmlForm += "\">";

  //Wheel Diameter
  htmlForm += "<label for=\"diameter\"> <span>Enter new wheel diameter</span></label>";
  htmlForm += "<input type=\"number\" min=\"1\" name=\"diameter\" value=\"";
  htmlForm.concat( wheelDiameter );
  htmlForm += "\">";  
  htmlForm += "<input type=\"submit\" value=\"update\">\n</form>\n";
  htmlForm += "</fieldset>";
  
  htmlForm += "</body>\n</html>\n";

 return htmlForm;
}
#endif
