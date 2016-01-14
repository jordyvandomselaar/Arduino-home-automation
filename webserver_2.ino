// Include libraries
#include <SPI.h>
#include <Ethernet2.h>
#include <NewRemoteReceiver.h>
#include <NewRemoteTransmitter.h>
#include <SD.h>

// Create MAC
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Create IP address
// IPAddress ip(192,168,1,102);

// Create the server
EthernetServer server(80);

// To save whether the light is on or not
boolean light_on = false;
NewRemoteTransmitter transmitter(123, 3, 260, 3); // Create transmitter
String HTTP_req; // stores the HTTP request

File webFile;
// Let's do the setup
void setup()
{
    Ethernet.begin(mac); // Initialize Ethernet device
    server.begin(); // Start to listen for clients


    // Initialize SD card
    if(!SD.begin(4)){
        return;    // init failed
    }

    // check for index.htm file
    if (!SD.exists("index.htm")) {
        return;  // can't find index file
    }
}

void loop()
{
    EthernetClient client = server.available(); // Try to get a client

    if(client) // If there is a client
    {
        boolean currentLineIsBlank = true; // Blank lines

        while(client.connected()){ // If client is connected
            if(client.available()){ // If client is available
                char c = client.read(); // Read 1 byte/character from client
                HTTP_req += c;  // save the HTTP request 1 char at a time
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if(c == '\n' && currentLineIsBlank){ // If response is a blank line
                    // Send standard http response header
                    client.println(F("HTTP/1.1 200 OK"));
                    client.println(F("Content-Type: text/html"));
                    client.println(F("Connection: close"));
                    client.println();

                    if(HTTP_req.indexOf("button") > -1){// Check for ajax
                        handle_ajax();
                    }

                    else {
                    // Let's send our webpage
                    webFile = SD.open("index.htm"); // Open webpage
                    if(webFile){
                        while(webFile.available()){
                            client.write(webFile.read()); // Send webfile to client
                        }
                        webFile.close(); // Close the file
                    }
                }
                    HTTP_req = "";    // finished with request, empty string

                    break;
                } // End if c == '\n' && currentLineIsBlank

                if(c == '\n'){
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                }

                else if(c != '\r'){
                    // A text character was received from client
                    currentLineIsBlank = false;
                }

            } // End if client.available()
        } // End while client.connected()
        delay(1); // Give the browser time to receive the data
        client.stop(); // Close the connection
    } // End if client
}

/*
 * Process a light from GET request *
 */

void handle_ajax(){
    int startUrlIndex= HTTP_req.indexOf("button");
    int endUrlIndex = HTTP_req.indexOf(" HTTP");
    String url = HTTP_req.substring(startUrlIndex, endUrlIndex);

    int startButtonIndex = url.indexOf("device-") + 7;// 7 is length of device-, I really just want the number.
    int endButtonIndex = url.indexOf("&");
    String button = url.substring(startButtonIndex, endButtonIndex);

    int startStateIndex = url.indexOf("state=") + 6; // 6 is length of state=, I really just want the number.
    String state = url.substring(startStateIndex);
    
    int device = button.toInt();
    int newState = state.toInt();

    dim_light(device, newState * 12);
}

/*
 * Dim a light
 * and set light_on to true or false depending on the status.
 */
void dim_light(int unit, int level) {
  if (level == 0) {
    transmitter.sendUnit(unit, false);
    light_on = false;
  }
  else if (level >= 1 && level <= 12) {
    transmitter.sendDim(unit, level);
    light_on = true;
  }
  else {
  }
}