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

TODO:

  - Activate screen saver slide show after 6 sec of inactivity

DONE:

  - Added Firmware update Over-The-Air (FOTA)

EXTRA:

  // 24 X LEDs for each of the sides - so 24x 2 (each side). Always on in white. Activation, flashing red. (48 total)

  // 3  X LEDS for the big hexagon (always on in white) always on (3 total)

  // 12  X LEDS small hexagon (always on, always white) activation flashing red, same as the sides. (12 total)

  // 3 X LEDs per arrow (6 in total for both arrows) white normally then on activation the directional arrow turns red and runs (6 total)

  // 18 X LEDs for the alarm call point sign - always white, always on (18 total)

  // 24 X LEDs for the FIRE sign (always RED, always on. flashing red on activation) (24 total)

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
  Serial.begin(115200); // Start the Serial Communication with PC 
  Serial.println("Debug Serial is ready.");

  Serial.println("NODEID: " + String(NODEID));

  Serial.print("Active Firmware version: ");
  Serial.println(FirmwareVer);

  Serial1.begin(115200, SERIAL_8N1, DWIN_TX_PIN, DWIN_RX_PIN); // Start the Serial Communication with DWIN LCD 
  Serial.println("Serial1 is ready.");

  LoRaSerial.begin(9600); // Start the Serial Communication with LoRa module
  Serial.println("LoRaSerial is ready.");

  // Init driver(LoRa E32) and mesh manager
  Serial.println("Initializing mesh");
  while(! initializeMESH()){  // stays in a loop until LoRa found 
    Serial.println("Mesh initialization failed");
    Serial.println("Retyring...");
    delay(3000);
   }
  Serial.println("Mesh initialized successfully.");

  // RTC pins for ESP32-S3-Mini
  Wire.begin(RTC_SDA, RTC_SCL);
  delay(5);

  // Mandatory for gps task
  Serial.println("Initializing RTC");
  while (!rtc.begin()){ // stays in a loop until RTC found 
    Serial.println("Couldn't find RTC");
    Serial.println("Retyring...");
    delay(3000);
  }
  Serial.println("RTC Initialized.");

  // This line sets date and time on RTC (year, month, date, hour, min, sec)
  // rtc.adjust(DateTime(2024, 5, 25, 16, 16, 0));

  setupLeds(); // Led Setup
  
  pinMode(SirenPIN, OUTPUT); // Declare siren bell pin as output

  // SIG pin must be HIGH for the sdcard to connect with the DWIN LCD
  pinMode(SIGPIN, OUTPUT); // Declare signal pin as output
  // digitalWrite(SIGPIN, HIGH); // commented to connect sd card with ESP32 

  pinMode(siteEvacuation_buttonPin, INPUT_PULLUP); // Declare button pin as input and enable internal pull up

  EEPROM.begin(512); // Initialize EEPROM
  preferences.begin("credentials", false); // Open Preferences with "credentials" namespace
  delay(5);

  // SD card configuration
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SD.begin(SD_CS);

  // initAudio(); // initialize audio

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);                     // default 0...21

  setCpuFrequencyMhz(240);
  audioSemaphore = xSemaphoreCreateBinary(); 

  delay(1000);
  
  // To perform login
  xTaskCreate(loginTask, "LoginTask", 8192, NULL, 1, &xHandlelogin);

  // To configure device
  xTaskCreate(configuredeviceTask, "ConfigureDeviceTask", 4096, NULL, 2, &xHandleconfigdevice);
  vTaskSuspend(xHandleconfigdevice);

  // To update date and time info
  xTaskCreate(dateTimeTask, "DateTimeTask", 2048, NULL, 3, &xHandledatetime);
  vTaskSuspend(xHandledatetime);

  // To handle home page tasks
  xTaskCreate(homepageTasks, "HomepageTasks", 10000, NULL, 9, &xHandlehomepage);
  vTaskSuspend(xHandlehomepage);

  // To handle node discovery
  xTaskCreate(LoRatask, "LoRatask", 10000, NULL, 10, &xHandleLoRa);
  vTaskSuspend(xHandleLoRa);  

  // To handle button activation and deactivation
  xTaskCreate(buttonTask, "buttonTask", 10000, NULL, 8, &xHandleButton);
  vTaskSuspend(xHandleButton);

  // To receive messages on LoRa
  xTaskCreate(RecvMessageTask, "RecvMessageTask", 10000, NULL, 8, &xHandleRecmessage);
  vTaskSuspend(xHandleRecmessage);

  // To run leds in infinite loop upon activation
  xTaskCreate(rgbTask, "rgbTask", 10000, NULL, 9, &xHandleRGB);
  vTaskSuspend(xHandleRGB);

  // To play audio and siren bell in infinite loop upon activation
  xTaskCreate(soundTask, "soundTask", 10000, NULL, 9, &xHandleSound);
  vTaskSuspend(xHandleSound);

  // xTaskCreate(checkGPSTask, "CheckGPS", 2048, NULL, 1, &xHandlegps);
  // vTaskSuspend(xHandlegps);

  // resetVP(CLIENT_SSID);
  resetVP(VP_UNIT_DATE);
  resetVP(VP_UNIT_TIME);
  // resetVP(CLIENT_PASSWORD);
  resetVP(CLIENT_PASSWORD_DISPLAY);
  // resetVP(ADMIN_SSID);
  // resetVP(ADMIN_PASSWORD);
  resetVP(ADMIN_PASSWORD_DISPLAY);
  resetVP(clientLoginStatus);
  resetVP(adminLoginStatus);
  resetVP(INTERNET_SSID);
  // resetVP(INTERNET_PASSWORD);
  resetVP(INTERNET_PASSWORD_DISPLAY);
  // resetVP(INTERNET_PASSWORD_ICON);
  resetVP(INTERNET_CONNECT_BUTTON);
  resetVP(UNIQUE_KEY_OKAY_BUTTON);
  resetVP(UNIQUE_KEY);
  delay(10);
  sendWriteCommand(LOGIN, RESET);
  resetVP(COMPANY_DONE_BUTTON_ADDRESS);
  resetVP(VP_DEVICE_DRIVER_RETURN_KEY);
  resetVP(VP_COMPANY_NAME);
  resetVP(VP_COMPANY_ADDRESS);
  resetVP(VP_KEY_RESPONSIBLE_PERSON_NAME);
  resetVP(VP_KEY_RESPONSIBLE_PERSON_CONTACT);
  resetVP(VP_KEY_RESPONSIBLE_PERSON1_NAME);
  resetVP(VP_KEY_RESPONSIBLE_PERSON1_CONTACT);
  resetVP(VP_AUTO_UPLOAD_COMPANY_DETAILS);
  resetVP(VP_KEY_RESPONSIBLE_PERSON2_NAME);
  resetVP(VP_KEY_RESPONSIBLE_PERSON2_CONTACT);
  resetVP(VP_KEY_RESPONSIBLE_PERSON3_NAME);
  resetVP(VP_KEY_RESPONSIBLE_PERSON3_CONTACT);
  resetVP(VP_KEY_RESPONSIBLE_PERSON4_NAME);
  resetVP(VP_KEY_RESPONSIBLE_PERSON4_CONTACT);
  resetVP(VP_LOCAL_FIRE_DEPARTMENT_NAME);
  resetVP(VP_LOCAL_FIRE_DEPARTMENT_CONTACT);
  resetVP(VP_MANUFACTURING_DETAILS);
  resetVP(VP_MANUFACTURE_NAME);
  resetVP(VP_MANUFACTURE_CONTACT);
  resetVP(VP_MANUFACTURE_EMAIL);
  resetVP(VP_MANUFACTURE_DATE);
  resetVP(VP_MANUFACTURE_SERIAL_N0);
  resetVP(VP_UNIT_DONE);
  resetVP(VP_LOCATION_OF_UNIT);
  resetVP(VP_ASSIGNED_UNIT_NUMBER);
  resetVP(VP_DATE_OF_UNIT_INSTALLATION);
  resetVP(VP_UNIT_INSTALLER);
  resetVP(VP_UNIT_CONTACT_DETAILS);
  resetVP(VP_UNIT_IP_ADDRESS);
  resetVP(VP_DEVICE_DRIVER_RETURN_KEY);
  resetVP(notificationStatus1);
  resetVP(notificationStatus2);
  resetVP(notificationStatus3);
  resetVP(notificationStatus4);
  delay(50);

  pageSwitch(COPYRIGHT); // Switch to Copyright Page
  Serial.println("Page Switched");
  delay(5);

}

// Run Code in Loop
void loop()
{
  // **************** Main Code starts here !!!!! ******************* //
  OTA_repeatedCall();

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
  
}

