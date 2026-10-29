/************************************************************
   WEBSOCKET_HANDLER.INO
   Connect/disconnect logging for the telemetry WebSocket
   (the actual JSON frames are broadcast from Telemetry.ino).
************************************************************/

 /******** WebSocket event handler ********/
 
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {

    case WStype_CONNECTED:
      Serial.println("WebSocket Client Connected");
      break;

    case WStype_DISCONNECTED:
      Serial.println("WebSocket Client Disconnected");
      break;

    default:
      break;
  }
}
