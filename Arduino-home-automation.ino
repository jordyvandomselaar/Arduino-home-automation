// Include libraries
#include <SPI.h>
#include <Ethernet2.h>
#include <NewRemoteReceiver.h>
#include <NewRemoteTransmitter.h>
#include <SD.h>
#include <OneWire.h>
#include <EEPROM.h>

// Create MAC
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xFD };

// Create IP address
// IPAddress ip(192,168,1,102);

// Create the server
EthernetServer server(80);

// To save whether the light is on or not
NewRemoteTransmitter transmitter(123, 3, 260, 3); // Create transmitter
String HTTP_req; // stores the HTTP request

File webFile;

int DS18S20_Pin = 2; //DS18S20 Signal pin on digital 2

//Temperature chip i/o
OneWire ds(DS18S20_Pin); // on digital pin 2

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
                       handle_light_ajax();
                    }

                    else if(HTTP_req.indexOf("getTemperature") > -1){
                        client.println(getTemp());
                    }

                    else {
                    // Let's send our webpage
                    webFile = SD.open("index.htm"); // Open webpage
                    if(webFile){
                        int device = 1; // Start at device one
                        char output[16];
                        byte out;
                        while(webFile.available()){
                            for(int i = 0; i < sizeof(output) + 5; i++){
                                out = webFile.read();
                                if(out == 94){
                                    if(read_config(device) == 1){
                                        out = 49;
                                    }
                                    else{
                                        out = 48;
                                    }
                                    device++;
                                }
                                if(out == 255){out = 0;}
                                output[i] = out;
                            }
                            client.write(output);
                        }
                        webFile.close(); // Close the file
                    }
                }
                    HTTP_req = "";    // finished with request, empty string

                    break;
                } // End if c == '\n' && currentLineIsBlank

                if(c == '\n'){
                    // last character on line of receiracter read
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

void handle_light_ajax(){
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

    send_signal(device, newState, "toggle");
    write_config(device, newState); // Save state
}

/*
 * Dim a light
 */
bool send_signal(int unit, int level, String button_type) {
  if (level == 0) {
    transmitter.sendUnit(unit, false);
    return true;
  }
    else{
        if(button_type == "dimmer"){
            if (level >= 1 && level <= 12) {
                transmitter.sendDim(unit, level);
                return true;
            }
            else {
                return false;
            }
        }

        else if(button_type == "toggle"){
            transmitter.sendUnit(unit, true);
            return true;
        }
        else{
            return false;
        }
    }
}



void write_config(int address, byte value){
    EEPROM.write(address, value);
}

int read_config(int address){
    return EEPROM.read(address);
}

/*
 * Get the temperature from the temperature sensor
 */

float getTemp(){
 //returns the temperature from one DS18S20 in DEG Celsius

 byte data[12];
 byte addr[8];

 if ( !ds.search(addr)) {
   ds.reset_search();
   return -1000;
 }

 if ( OneWire::crc8( addr, 7) != addr[7]) {
   return -1001;
 }

 if ( addr[0] != 0x10 && addr[0] != 0x28) {
   return -1002;
 }

 ds.reset();
 ds.select(addr);
 ds.write(0x44,1); // start conversion, with parasite power on at the end

 byte present = ds.reset();
 ds.select(addr);  
 ds.write(0xBE); // Read Scratchpad

 
 for (int i = 0; i < 9; i++) { // we need 9 bytes
  data[i] = ds.read();
 }
 
 ds.reset_search();
 
 byte MSB = data[1];
 byte LSB = data[0];

 float tempRead = ((MSB << 8) | LSB); //using two's compliment
 float TemperatureSum = tempRead / 16;

 return TemperatureSum;
 

 
}