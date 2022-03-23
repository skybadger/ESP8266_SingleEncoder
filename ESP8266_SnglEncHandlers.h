#ifndef _ESP8266_SnglEncoderHandlers_h_
#define _ESP8266_SnglEncoderHandlers_h_

  extern ESP8266WebServer server;
  void handleEncoder( void );
  void handleEncoderAsBearing( void );
  void handlePpr( void );
  void handleWheelDiameter( void );   
  void handlePpRollover( void );   
  void handleStatusGet( void);
    
  void handleRoot( void );
  void handleNotFound( void );
    
  //Position
  void handleEncoder( void )
  {
    String timeString = "", message = "";
    int returnCode = 200;
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    root["time"] = getTimeAsString( timeString );
    
    if( encPresent )
    {
      if ( server.method() == HTTP_GET ) 
      {
        root["position"]    = (long int) position;
        returnCode = 200;
      }
      else if ( server.method() == HTTP_PUT )
      {
        String toSearchFor[] = {"position",};
        if ( hasArgIC( toSearchFor[0], server, false ) )
        {
          int newPos = server.arg(toSearchFor[0]).toInt();
          if ( newPos >= 0 && newPos < ppRollover )
          {
            position = newPos;
            enc.write( newPos);
            saveToEeprom();
          }
          else
          {
            root["Message"]     = "Encoder new position not in range 0 <= position <= rollover";
            returnCode = 403;
          }
        }
      }
      else
      {
        root["Message"]     = "HTML verb not supported - only GET or PUT";
        returnCode = 403;
      }
    }
    else
    {
      root["Message"]     = "Encoder sensor not present";
      returnCode = 403;
    }

    //return response
    root.printTo(message);
    server.send(returnCode, "application/json", message);  
  }

  //PPR
  void handlePpr( void )
  {
    String timeString = "", message = "";
    int returnCode = 200;
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    root["time"] = getTimeAsString( timeString );
    
    if( encPresent )
    {
      if ( server.method() == HTTP_GET ) 
      {
        root["ppr"]    = (long int) ppr;
        returnCode = 200;
      }
      else if ( server.method() == HTTP_PUT )
      {
        String toSearchFor[] = {"ppr",};
        if ( hasArgIC( toSearchFor[0], server, false ) )
        {
          int newPos = server.arg(toSearchFor[0]).toInt();
          if ( newPos > 0 ) 
          {
            ppr = newPos;
            saveToEeprom();
          }
          else
          {
            root["Message"]     = "Encoder ppr not accepted - 0 found";
            returnCode = 403;
          }
        }
      }
      else
      {
        root["Message"]     = "HTML verb not supported - only GET or PUT";
        returnCode = 403;
      }
    }
    else
    {
      root["Message"]     = "Encoder sensor not present";
      returnCode = 403;
    }

    //return response
    root.printTo(message);
    server.send(returnCode, "application/json", message);  
  }
 
  //PPRollover
  void handlePpRollover( void )
  {
    String timeString = "", message = "";
    int returnCode = 200;
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    root["time"] = getTimeAsString( timeString );
    
    if( encPresent )
    {
      if ( server.method() == HTTP_GET ) 
      {
        root["ppRollover"]    = (long int) ppRollover;
        returnCode = 200;
      }
      else if ( server.method() == HTTP_PUT )
      {
        String toSearchFor[] = {"ppRollover",};
        if ( hasArgIC( toSearchFor[0], server, false ) )
        {
          int newPos = server.arg(toSearchFor[0]).toInt();
          if ( newPos >= 0 && newPos < __INT_MAX__ ) 
          {
            ppRollover = newPos;
            saveToEeprom();
          }
          else
          {
            root["Message"]     = "Rollover value not accepted < 0 found";
            returnCode = 403;
          }
        }
      }
      else
      {
        root["Message"]     = "HTML verb not supported - only GET or PUT";
        returnCode = 403;
      }
    }
    else
    {
      root["Message"]     = "Encoder sensor not present";
      returnCode = 403;
    }

    //return response
    root.printTo(message);
    server.send(returnCode, "application/json", message);  
  }

  //Wheel size
  void handleWheelDiameter( void )
  {
    int returnCode = 200;
    String timeString = "", message = "";
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    root["time"] = getTimeAsString( timeString );
    
    if( encPresent )
    {
      if ( server.method() == HTTP_GET ) 
      {
        root["wheelDiameter"]    = wheelDiameter;
        returnCode = 200;
      }
      else if ( server.method() == HTTP_PUT )
      {
        String toSearchFor[] = {"wheelDiameter",};
        if ( hasArgIC( toSearchFor[0], server, false ) )
        {
          float newSize = (float) server.arg( toSearchFor[0] ).toDouble();
          if ( newSize > 0 && newSize < (float) __INT_MAX__ ) 
          {
            wheelDiameter = newSize;
            saveToEeprom();
          }
          else
          {
            root["Message"]  = "Wheel diameter value not accepted - 0 found";
            returnCode = 403;
          }
        }
      }
      else
      {
        root["Message"]     = "HTML verb not supported - only GET or PUT";
        returnCode = 403;
      }
    }
    else
      root["Message"]     = "Encoder sensor not present";

  //return response
  root.printTo(message);
  server.send(returnCode, "application/json", message);  
  }

  //Encoder as bearing
  void handleEncoderBearingGet( void )
  {
    int returnCode = 200;
    String timeString = "";
    String message = "";
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    root["time"] = getTimeAsString( timeString );
    
    if( encPresent )
    {
      if ( server.method() == HTTP_GET ) 
      {
        root["bearing"] = (float) (position * 360.0F / ppRollover);
        returnCode = 200;
      }
      else
      {
        root["Message"]     = "HTML verb not supported - only GET";
        returnCode = 403;
      }
    }
    else
    {
      root["Message"]     = "Encoder sensor not present";
      returnCode = 403;
    }

    //return response
    root.printTo(message);
    server.send(returnCode, F("application/json"), message);  
  }
 
 void handleEncoderBearingPut( void ) 
{
  int i=0;
  int returnCode = 200;
  String errMsg;
  int newPosition = 0;
  String timeString;
  String formatCode; 
 
  DynamicJsonBuffer jsonBuffer(256);
  JsonObject& root = jsonBuffer.createObject();

  DEBUGSL1( "Entered handleEncoderSetupPut" );
  String argsToSearchFor[] = {"bearing", };
  
  root["time"] = getTimeAsString( timeString );

  //throw error message
  if ( !hasArgIC( argsToSearchFor[0] , server, false) )
  {
      returnCode = 403;
      errMsg = "Bearing arg not found"; //Should'nt happen from our own form! 
      formatCode = F("text/plain");
  }
  else 
  {
    newPosition = server.arg( argsToSearchFor[0] ).toFloat();
    if ( (newPosition >= 0 && newPosition <= 360.0F  ) ) 
    {  
      enc.write( newPosition/360.0F * ppRollover );
      root["bearing"] = enc.read( )*360.0/ppRollover;
      root.printTo( errMsg );//"Invalid bearing value found"; //Should'nt happen from our own form! 
      returnCode = 200;
      formatCode = F("application/json");
    }
    else
    {
      errMsg = "bearing argument out of range 0-360";
      returnCode = 403;
      formatCode = F("text/plain");
    }
  }

  server.send( returnCode, formatCode, errMsg ); 
  DEBUGSL1( "Exiting handleEncoderBearingPut" );
}

 void handleStatusGet( void)
 {
    int returnCode = 200;
    String timeString = "", message = "";
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    //Update the error message handling.
    // JsonArray errors = root.createArray( "errors" );
    
    root["time"] = getTimeAsString( timeString );
    root["encPresent"] = (encPresent) ? "true": "false";
    root["position"] = position;
    root["ppr"] = ppr;
    root["ppRollover"] = ppRollover;
    root["wheelDiameter"] = wheelDiameter;
    root["Bearing"] = position * 360.0F / ppRollover ;

    root.printTo( message );
    server.send( returnCode, "application/json", message);  
 }

   /*
   * Web server handler functions
   */
  void handleNotFound()
  {
  String uri = "http://";
  String message;

  uri += myHostname;
  message = "<!DOCTYPE HTML> <html><body><H1> URL not understood</h1><ul>";
  message.concat( "<li><a href=\"");
  message.concat( uri );
  message.concat ( "/status\" > Simple status read </a></li>");
  message.concat( "<li><a href=\"");
  message.concat( uri );
  message.concat ( "/encoder\" >Encoder read/write (GET/PUT position=value)</a></li>");
  message.concat( "<li><a href=\"");
  message.concat( uri );
  message.concat ( "/encoder/bearing\" >Encoder as bearing (GET) </a></li>");
  message.concat( "<li><a href=\"");
  message.concat( uri );
  message.concat ( "/setup/ppr\" >Encoder pulses per revolution (GET|PUT ppr=value ) </a></li>");
  message.concat( "<li><a href=\"");
  message.concat( uri );
  message.concat ( "/setup/ppRollover\" >Encoder rollover count (GET|PUT ppRollover=value ) </a></li>");
  message.concat( "<li><a href=\"");
  message.concat( uri );
  message.concat ( "/setup/wheelDiameter\" >Encoder tracking wheel diameter count (GET|PUT wheelDiameter=value ) </a></li>");
  message.concat ( "</ul></body></html>");
  server.send(404, "text/html", message);
  }
 
  void handlerRestart( void)
  {
    //Trying to do a redirect to the rebooted host so we pick up from where we left off. 
    server.sendHeader( WiFi.hostname().c_str(), String("/status"), true);
    server.send ( 302, F("text/html"), "<!Doctype html><html>Redirecting for restart</html>");
    DEBUGSL1("Reboot requested");
    device.restart();
  }
  
  //Return sensor status
  void handleRoot()
  {
    String timeString = "", message = "";
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    root["time"] = getTimeAsString( timeString );
    //Todo add system status here
    
    
    //root.printTo( Serial );
    root.printTo(message);
    server.send(200, "application/json", message);
  }
#endif
