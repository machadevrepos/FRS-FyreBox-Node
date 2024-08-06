/*
 Project Name ------  FRS
 Task --------------  DWIN LCD Firmware with Esp32
 Engineer ----------- Muhammad Usman
 File --------------- Main File (Code Exceution Start in this file)
 Company -----------  Machadev Pvt Limited

Firmware Details

Implemented:

- LoRa Task: To frequently update the mesh network. (Only Node discovery)
- Task Management:
  - All tasks are suspended except for the login task.
  - Upon completion of the login task, the device configuration task is initiated.
  - In the configuration task, company details, manufacturer details, unit details, and device direction are set.
  - Once the device is configured, it enters an infinite slideshow loop until a screen touch is detected.
  - If a touch is detected, it proceeds to handle homepage tasks.

  - If the LoRa module is not found, the system will remain in a loop and retry until the module is detected.
  - If the RTC module is not found, the system will remain in a loop and retry until the module is detected.

- Added activation and deactivation
  - if you activate the alarm from button it should be deactivated from button of same node
  - if you activate the alarm from lcd it should be deactivated from lcd of same node 
- If activated ring bell for 6 sec and play audio both in a loop until deactivated accordingly
- Integrated RGBs
- Activate and deactivate other nodes through LoRa
- Send messages to visitors

Mesh Network Development:

- Developed a mesh network using Ebyte LoRa E32 with ESP32.
- The same logic applies to each node.

Flow:

- The mesh network program is identical for all nodes in the network:
  - Each node listens to other nodes in a loop.
  - Each node updates the activity status of other nodes every 10 seconds.
  - Each node broadcasts a message every 5 seconds.
  - Each node prints network stats every 20 seconds.

Tests:

- The network consists of 4 nodes:
  - Each node knows how many nodes are available, active, and dead in its network.
  - Each node successfully updates its node info container every 10 seconds.
  - Note: There is no acknowledgment message for broadcasted messages.

PlatformIO Configuration:

[env:esp32-s3-devkitm-1]
platform = espressif32
board = esp32-s3-devkitm-1
framework = arduino
monitor_speed = 115200

Escalator Node:

- The relay is active low, with an LED connected in series with the relay.

FyreBox Node:

- Serial0: Used for program uploading and debugging.
- Serial1: DWIN LCD is connected at IO15 (TX of LCD) and IO16 (RX of LCD).
- LoRa Module: Connected at IO35 (RX) and IO36 (TX) using software serial.
- SD Card Connection: The SIG pin must be HIGH (IO5) for the SD card to connect with the DWIN LCD.

- Added Firmware update Over-The-Air (FOTA) in FyreBox Node
- Update the firmware version number in the `FirmwareVer` constant in the `constant.cpp` file and in the `firmware_version.txt` file, 
- then compile/build the firmware then push the changes to the `FRS-FyreBox-Node` public repository owned by `machadevrepos`.
- The node then downloads the latest version from GitHub, uploads it to the ESP32, and reboots. This process takes 1 to 2 minutes.

TODO:

  - Activate screen saver slide show after 6 sec of inactivity
  - Get date time from internet 
  - Must be able to activate evacuation procedure using switch from anywhere in the screen (Independant from screen)
  - Must be able to activate and deactivate from any screen
  - ⁠Menus bar must be completed on the GUI- each tab i.e. Battery calculations maintenance and installation procedure etc
  - The Fire depatment contact number. An SMS must only be sent to that number when two units have been activated.
  - Give the client an option on the GUI when he puts that number in if it should send the notication when one unit or two units are activated
  - N.B Internal nodes a problem (same node number) IP Address?? When a box is activated it resets the other boxes on the network ****

DONE:

  - Update site informtion company detail screens
  - Update date and time screen
  - Add installation procedure screens
  - Add yes or no screen upon activation
  - Add sign language screen  
  - Upon Activation of the Fyrebox, the site map should be displayed for a duration of 8 seconds during the bell sound. 
    When the Fyrebox unit announces "ATTENTION", the sign language image should be displayed for 8 seconds. 
    And lastly when the Fyrebox unit announces "EMERGENCY", the emergency procedure image should be displayed for 8 seconds.

  - Reassign VP addresses of icon display in show/hide password
  - Replace supplier information with Manufacturer details
  - Replace client details with company details
  - Remove upload pdf
  - Remove setup unit
  - Remove weekly testing
  - Add settings button
  - Add factory reset button

  - Design screen for settings 
  - Design screen for factory reset
  - Change font colour to white

  - updated maintenance procedure, settings, wifi and unique key screens
  - integrated confirmation screen for time and date update for all units
  - Added password protection for manufacturer and company details in menu bar
  - Added WiFi, battery and device configured icon just for testing

EXTRA:

  - DWIN LCD datasheet page not found
  - Used ESP32 s3 not ESP32U
  - GPS not used

*/

// Import Libraries
#include "lcd.h"
#include "constant.h"
#include <WiFi.h>
#include <EEPROM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <OTA_cert.h>

// Initialize and define SoftwareSerial object
SoftwareSerial SerialGPS(GPSRXPin, GPSTXPin); // Not currently used
SoftwareSerial LoRaSerial(LORA_TX_PIN, LORA_RX_PIN); // ESP32(RX), ESP32(TX)

// Setup Function: Call Once when Code Starts
void setup() {
  Serial.begin(BAUD_RATE_SERIAL); // Start the Serial Communication with PC 
  enableDebugging();  // Uncomment this line to enable helpful debug messages on Serial
  if (printDebug) {
    Serial.println("Debug Serial is ready.");

    Serial.println("NODEID: " + String(NODEID));

    Serial.println("Active Firmware version: " + FirmwareVer);
  }

  Serial1.begin(BAUD_RATE_DWIN, SERIAL_8N1, DWIN_TX_PIN, DWIN_RX_PIN); // Start the Serial Communication with DWIN LCD 
  if (printDebug) Serial.println("Serial1 is ready.");

  LoRaSerial.begin(BAUD_RATE_LORA); // Start the Serial Communication with LoRa module
  if (printDebug) Serial.println("LoRaSerial is ready.");

  // Init driver(LoRa E32) and mesh manager
  if (printDebug) Serial.println("Initializing mesh");
  while(! initializeMESH()){  // stays in a loop until LoRa found 
    if (printDebug) {
        Serial.println("Mesh initialization failed");
        Serial.println("Retyring...");
    }
    delay(3000);
  }
  if (printDebug) Serial.println("Mesh initialized successfully.");

  // RTC pins for ESP32-S3-Mini
  Wire.begin(RTC_SDA, RTC_SCL);
  delay(5);

  // Mandatory for gps task
  if (printDebug) Serial.println("Initializing RTC");
  while (!rtc.begin()){ // stays in a loop until RTC found 
    if (printDebug) {
        Serial.println("Couldn't find RTC");
        Serial.println("Retyring...");
    }
    delay(3000);
  }
  if (printDebug) Serial.println("RTC Initialized.");

  setupLeds(); // Led Setup
  
  pinMode(SirenPIN, OUTPUT); // Declare siren bell pin as output

  // SIG pin must be HIGH for the sdcard to connect with the DWIN LCD
  pinMode(SIGPIN, OUTPUT); // Declare signal pin as output
  // digitalWrite(SIGPIN, HIGH); // commented to connect sd card with ESP32 
  // systemReset(); // testing DWIN LCD to download files

  pinMode(siteEvacuation_buttonPin, INPUT_PULLUP); // Declare button pin as input and enable internal pull up

  EEPROM.begin(512); // Initialize EEPROM
  preferences.begin("credentials", false); // Open Preferences with "credentials" namespace
  preferences.begin("configuration", false); // Open Preferences with "configuration" namespace
  delay(5);

  // SD card configuration
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SD.begin(SD_CS);

  // initAudio(); // initialize audio

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(16);                     // default 0...21

  setCpuFrequencyMhz(240);
  audioSemaphore = xSemaphoreCreateBinary(); 

  delay(1000);
  
  // To perform login
  xTaskCreate(loginTask, "LoginTask", 5000, NULL, 1, &xHandlelogin);

  // To configure device
  xTaskCreate(configuredeviceTask, "ConfigureDeviceTask", 4096, NULL, 2, &xHandleconfigdevice);
  vTaskSuspend(xHandleconfigdevice);

  // To update date and time info
  xTaskCreate(dateTimeTask, "DateTimeTask", 2048, NULL, 3, &xHandledatetime);
  vTaskSuspend(xHandledatetime);

  // To handle home page tasks
  xTaskCreate(homepageTasks, "HomepageTasks", 9000, NULL, 10, &xHandlehomepage);
  vTaskSuspend(xHandlehomepage);

  // To handle node discovery
  xTaskCreate(LoRatask, "LoRatask", 3072, NULL, 8, &xHandleLoRa);
  vTaskSuspend(xHandleLoRa);  

  // To handle button activation and deactivation
  xTaskCreate(buttonTask, "buttonTask", 3072, NULL, 8, &xHandleButton);
  // vTaskSuspend(xHandleButton);

  // To receive messages on LoRa
  xTaskCreate(RecvMessageTask, "RecvMessageTask", 2048, NULL, 8, &xHandleRecmessage);
  vTaskSuspend(xHandleRecmessage);

  // Create the FreeRTOS task for checking internet connectivity
  xTaskCreate(checkInternetTask, "Check Internet Task", 4096, NULL, 1, &xHandlewifi);
  vTaskSuspend(xHandlewifi);

  // reset_allText();

  delay(50);

  // preferences.putBool("configuration", deviceConfiguredFlag); // only for testing

  deviceConfiguredFlag = preferences.getBool("configuration", false); 

  if (!deviceConfiguredFlag) {
    pageSwitch(COPYRIGHT); // Switch to Copyright Page
    if (printDebug) Serial.println("COPYRIGHT");
    delay(5);
  }
}

// Run Code in Loop
void loop()
{
  // **************** Main Code starts here !!!!! ******************* //
  // OTA_repeatedCall();

  DateTime now = rtc.now();

  day = DAY.toInt(); 
  month = MONTH.toInt(); 
  year = YEAR.toInt();

  weekByMonth = getWeekNumberByMonth(day, month, year); // returns int (number of weeks in a month 1 to 4)
  weekByYear = getWeekNumberByYear(day, month, year); // returns int (number of weeks in a year 1 to 54)

  int currentWeekByMonth = weekByMonth + 1;
  // Serial.println("Week passed by month: "+weekByMonth);
  // Serial.println("Current Week by month: "+currentWeekByMonth);

  int currentWeekByYear = weekByYear + 1;
  // Serial.println("Week passed by year: "+weekByYear);
  // Serial.println("Current Week by year: "+currentWeekByYear);

  // UBaseType_t uxHighWaterMark;
  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandlelogin);
  // Serial.print("Login Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);

  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandleconfigdevice);
  // Serial.print("Configure Device Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);

  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandledatetime);
  // Serial.print("xHandledatetime Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);

  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandlehomepage);
  // Serial.print("xHandlehomepage Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);

  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandleLoRa);
  // Serial.print("xHandleLoRa Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);

  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandleButton);
  // Serial.print("xHandleButton Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);

  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandleRecmessage);
  // Serial.print("xHandleRecmessage Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);

  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandleRGB);
  // Serial.print("xHandleRGB Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);

  // uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandleSound);
  // Serial.print("xHandleSound Task HighWaterMark: ");
  // Serial.println(uxHighWaterMark);


//  // Check task states periodically
//   printTaskState(xHandlelogin, "Login Task");
//   printTaskState(xHandleconfigdevice, "Configure Device Task");
//   printTaskState(xHandledatetime, "DateTime Task");
//   printTaskState(xHandlehomepage, "Homepage Task");
//   printTaskState(xHandleLoRa, "LoRa Task");
//   printTaskState(xHandleButton, "Button Task");
//   printTaskState(xHandleRecmessage, "RecvMessage Task");
//   printTaskState(xHandleRGB, "RGB Task");
//   printTaskState(xHandleSound, "Sound Task");

  delay(100);
  
}

