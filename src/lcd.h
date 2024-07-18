/*
 Project Name ------  FRS
 Task --------------  DWIN LCD Firmware with Esp32
 Engineer ----------- Muhammad Usman
 File --------------- Lcd Header File
 Company -----------  Machadev Pvt Limited
 */

// lcd.h
#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include <Preferences.h>
#include <FastLED.h>
#include "Audio.h"
#include "esp32-hal-cpu.h"
#include "freertos/semphr.h"
#include "esp_task_wdt.h"
#include <Adafruit_Sensor.h>
#include <HardwareSerial.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "OTA_cert.h"

// For LoRa Mesh networking
#include <RH_E32.h>
#include <RHMesh.h>
#include <vector>

extern RTC_DS3231 rtc; // Define rtc
extern SoftwareSerial SerialGPS;

extern SoftwareSerial LoRaSerial;
extern RHMesh mesh;
 
extern Preferences preferences;

extern Audio audio;
extern AudioBuffer InBuff;
extern SemaphoreHandle_t audioSemaphore;

struct Credentials 
{
    String ssid;
    String password;
};

struct NodeStatus {
    uint8_t nodeId;
    unsigned long lastSeen;
    bool isActive;
};

void readData();
void checkVersion();
void sendReadCommand(uint16_t address, uint16_t data_length);
void sendWriteCommand(uint16_t address, byte data);
void resetVP(uint16_t address);
String readDataFromDisplay(uint16_t address, uint16_t totalLength, uint16_t maxChunkSize);
bool readCheckboxState();
String extractDataBetweenMarkers(String input, String startMarker, String endMarker);
String extractDataBeforeMarker(String input, String startMarker);
String tempreadResponse();
bool containsPattern(const String &str, const String &pattern);
String extractDataBetweenPatterns(const String &input, const String &startPattern, const String &endPattern);
String dummyReadResponse();
String extractVpAddress(const String &inputHex, const String &vpAddressPattern);
String removeFirst6Bytes(const String &input);
String remove13Characters(const String &input);
String removeFirst7Bytes(const String &input);
String processFourthAndFifthBytes(const String &checkData);
void startCheckingPassword(uint16_t passwordDisplay, uint16_t passwordIcon, const String &checkData);
String processPasswordDisplay(uint16_t readPassword, uint16_t passwordDisplay, uint16_t passwordIcon);
String readText();
String hexToStringRemovedZeros(const String &hex);
String hexToString(const String &hex);
bool checkLastFourDigitsMatch(const String &inputString, const String &targetDigits);
bool checkLast3DigitsMatch(const String &inputString, const String &targetDigits);
String readResponse();
Credentials retrieveCredentials(uint16_t ssidCommand, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon);
void processClientLogin(uint16_t username, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon);
void processAdminLogin(uint16_t username, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon);
void processWiFiCredentials(uint16_t ssid, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon);
String readOneData(uint16_t ssidCommand);
void performLoginCheck(bool &clientLogin, bool &adminLogin);
void readPage();
void pageSwitch(byte pageNo);
void Display_AC_DEAC_Icon(byte iconNo);
void systemReset();
bool compareCredentials(String ssid, String password);
bool compareInternetCredentials(String ssid, String password);
String toHexString(const String &data);
void sendDataToLcd(uint16_t vpAddress, const String &data);
byte hexCharToByte(char c);
byte hexStringToByte(const String &hex);
byte hexToByte(const char *hex);
void writeString(uint16_t address, const String &hexData);
String ReturnJson(String url, DynamicJsonDocument &doc);
bool processGPRMC(String gprmcString);
void sendIconcommand(uint16_t pageVP, byte icon0, byte icon1, byte icon2, byte icon3, byte icon4, byte icon5);
String extractKeycode(const String &input);
String extractPageVP(const String &input, const String &vpAddressPattern);
String concatinate_checkboxData();
bool isActivityDetected();
String devicesAvailable_DB(String url);
int getWeekNumberByMonth(int day, int month, int year);
int getWeekNumberByYear(int day, int month, int year);
void checkGPSTask(void *parameter);
bool getGPSTime();
void dateTimeTask(void *parameter);
void loginTask(void *parameter);
void readeyeIcon(String temppassword, uint16_t passwordvp, uint16_t passwordIcon, uint16_t passwordDisplay);
void connectInternet();
void configureInternet();
void checkInternetTask(void *parameter);
void getGeolocation();
void configureLogin();
void configuredeviceTask(void *parameter);
void configuredeviceAgain();
void companyDetails();
void manufactureDetails();
void unitDetails();
void devicesDirectionDetails();
void slideShow();
void slideShow_EvacuationDiagrams();
void slideShow_EvacuationDiagrams_forButton();
void homepageTasks(void *parameter);
void CheckBoxes();
void displayIcons();
void saveClientCredentials(const String& username, const String& password);
void removeClientCredentials();
void saveAdminCredentials(const String& username, const String& password);
void removeAdminCredentials();
void saveInternetCredentials(const String& ssid, const String& password);
bool RememberIcon(uint16_t rememberLogin);
void showMessage(uint16_t VP_ADDRESS, String displaymessage);
void displayFyreBoxUnitList();
void FyreBoxUnitList();
int readAndConvertHexString(uint16_t vp_address);
bool isLeapYear(int year);
int daysInMonth(int month, int year);

// for LoRa Mesh 
void LoRatask(void* parameter);
const __FlashStringHelper* getErrorString(uint8_t status);
bool initializeMESH();
void broadcastPresence();
void updateNodeStatus(uint8_t nodeId);
void checkNodeActivity();
void listenForNodes();
size_t getTotalNodes();
void printNodeStatuses();
void printNetworkStats();

// Led functions
void setupLeds();
void FillSolidLeds(struct CRGB * targetArray, int numToFill, const struct CRGB& color);
void ActivateRGBs(bool activate, bool dir = false);
void BlinkLeds(int duration);
void RgbArrowMove(bool dir, uint8_t speed);

void initAudio();
void download_audio();
void sd_card();
void downloadFile(const char *resourceURL, const char *filename);
void buttonTask(void * parameter);
void RecvMessageTask(void *parameter);
void sendActivationMessage();
void sendDeactivationMessage();
void rgbTask(void *parameter);
void soundTask(void *parameter);
void sendNotificationSMS(void *parameter);

// FOTA functions
int FirmwareVersionCheck(void);
void firmwareUpdate(void);
void OTA_repeatedCall();

// Function to print task state
void printTaskState(TaskHandle_t taskHandle, const char *taskName);

#endif // LCD_H
