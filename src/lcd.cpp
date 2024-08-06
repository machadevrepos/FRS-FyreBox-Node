/*
 Project Name ------  FRS
 Task --------------  DWIN LCD Firmware with Esp32
 Engineer ----------- Muhamamd Usman
 File --------------- Lcd Source File
 Company -----------  Machadev Pvt Limited
 */

// Import Libraries
#include "lcd.h"
#include "constant.h"
#include <EEPROM.h>

RTC_DS3231 rtc;
Preferences preferences;

RH_E32 driver(&LoRaSerial, MOPIN, M1PIN, AUXPIN);
RHMesh* mesh = new RHMesh(driver, NODEID);  // Initialize the mesh with the initial NODEID

// RHMesh mesh(driver, NODEID); // Node ID 

std::vector<NodeStatus> nodeStatuses;

CRGBArray<NUM_LEDS_RGB1> BigHexagonAndAlarmCallPointLEDs;
CRGBArray<NUM_LEDS_RGB2> SmallHexagonsAndFireLEDs;
CRGBArray<NUM_LEDS_RGB3> LeftArrowLEDs;
CRGBArray<NUM_LEDS_RGB4> RightArrowLEDs;
CRGBArray<NUM_LEDS_RGB5> SideLEDs;

Audio audio;
AudioBuffer InBuff;
SemaphoreHandle_t audioSemaphore;

void reset_allText() {
    resetVP(CLIENT_SSID);
    resetVP(VP_UNIT_DATE);
    resetVP(VP_UNIT_TIME);
    resetVP(CLIENT_PASSWORD);
    resetVP(CLIENT_PASSWORD_DISPLAY);
    // resetVP(CLIENT_PASSWORD_ICON);
    resetVP(ADMIN_SSID);
    resetVP(ADMIN_PASSWORD);
    resetVP(ADMIN_PASSWORD_DISPLAY);
    resetVP(clientLoginStatus);
    resetVP(adminLoginStatus);
    resetVP(INTERNET_SSID);
    resetVP(INTERNET_PASSWORD);
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
}

void enableDebugging() {
    printDebug = true;
}

// Dummy Function to Read Data from Lcd
void readData() { // Not used
    byte open[] = {90, 165, 0x04, 0x83, 0x00, 0x14, 0x01};
    Serial1.write(open, sizeof(open));
    delay(5);

    String data = "";
    while (Serial1.available() > 0)
    {
        char Received = Serial1.read();
        Serial.println(Received, HEX);
        if ((Received == '0') || (Received == '1') || (Received == '2') || (Received == '3') || (Received == '4') || (Received == '5') || (Received == '6') || (Received == '7') || (Received == '8') || (Received == '9') || (Received == '.'))
        {
            data += Received;
        }
    }
    if (printDebug) Serial.println(data);
}

// Check Lcd Version
void checkVersion() { // Not used
    byte version[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x0F, 0x01};
    Serial1.write(version, sizeof(version));
}

// Write Dataframe to Lcd at Specified VP Addresses
void sendReadCommand(uint16_t address, uint16_t data_length) {
    byte frames[] = {0x5A, 0xA5, 0x04, 0x83, (byte)(address >> 8), (byte)(address & 0xFF), (byte)data_length};
    Serial1.write(frames, sizeof(frames));
}

// Used to reset specific VP Address
void sendWriteCommand(uint16_t address, byte data) {
    byte frame[] = {0x5A, 0xA5, 0x04, 0x82, (byte)(address >> 8), (byte)(address & 0xFF), data};
    Serial1.write(frame, sizeof(frame));
}

// Reset vp address
void resetVP(uint16_t address) {
    byte frame[] = {0x5A, 0xA5, 0x2B, 0x82, (byte)(address >> 8), (byte)(address & 0xFF)};
    Serial1.write(frame, sizeof(frame));
    // for (int i = 0; i < sizeof(frame); i++)
    // {
    //     Serial.print("Frame ");
    //     Serial.print(i);
    //     Serial.print(": ");
    //     Serial.println(frame[i], HEX);
    // }

    for (int i = 0; i < 40; i++)
    {
        Serial1.write(0x00);
    }
}

// Read Data in Chunks from Display
String readDataFromDisplay(uint16_t address, uint16_t totalLength, uint16_t maxChunk) { // Not used
    const uint16_t maxChunkSize = maxChunk; // Maximum read length
    String completeData = "";

    while (totalLength > 0) {
        uint16_t chunkSize = (totalLength > maxChunkSize) ? maxChunkSize : totalLength;
        sendReadCommand(address, chunkSize);

        delay(10);

        // completeData += tempreadResponse().substring(13);
        completeData += dummyReadResponse();

        address += chunkSize;     // Increment address for next chunk
        totalLength -= chunkSize; // Reduce remaining data length
    }

    return completeData;
}

bool readCheckboxState() { // Not used
    sendReadCommand(checkboxVP, 0x01);
    delay(100); // Delay for the display to respond

    if (Serial1.available()) {
        int available_bytes = Serial1.available();
        byte response[available_bytes];
        Serial1.readBytes(response, available_bytes);

        // Check if the response is for the correct VP address and extract the state
        if (response[3] == 0x83 && response[4] == (byte)(checkboxVP >> 8) && response[5] == (byte)(checkboxVP & 0xFF)) {
            return response[7] == 1;
        }
    }
    return false;
}

String extractDataBetweenMarkers(String input, String startMarker, String endMarker) { // Not used
    int startPos = input.indexOf(startMarker);
    if (startPos == -1) {
        // Start marker not found in the input
        return "";
    }

    int endPos = input.indexOf(endMarker, startPos + startMarker.length());
    if (endPos == -1) {
        // End marker not found in the input after the start marker
        return "";
    }

    // Extract the data between the markers
    String extractedData = input.substring(startPos + startMarker.length(), endPos);

    return extractedData;
}

String extractDataBeforeMarker(String input, String startMarker) {
    int startPos = input.indexOf(startMarker);
    if (startPos == -1)
    {
        // Start marker not found in the input, return the entire string
        return input;
    }

    // Extract the data before the start marker
    String extractedData = input.substring(0, startPos);

    return extractedData;
}

String tempreadResponse() {
    String completeData = "";
    while (Serial1.available()) {
        char a = Serial1.read();
        completeData += String(a, HEX);
    }
    return completeData;
}

bool containsPattern(const String &str, const String &pattern) {
    return str.indexOf(pattern) != -1;
}

String extractDataBetweenPatterns(const String &input, const String &startPattern, const String &endPattern) {
    int start = input.indexOf(startPattern);
    if (start == -1) {
        return ""; // Start pattern not found
    }
    start += startPattern.length(); // Move to the end of the start pattern

    int end = input.indexOf(endPattern, start);
    if (end == -1) {
        return ""; // End pattern not found
    }

    return input.substring(start, end); // Extract the data between the patterns
}

String dummyReadResponse() {
    String completeData = "";
    int byteCount = 0;

    while (Serial1.available()) {
        // Serial.print(Serial1.read(),HEX);
        char a = Serial1.read();
        // completeData += String(a, HEX);
        if (byteCount > 6) {
            //    String hexValue = String(a, HEX);
            //    if (hexValue != "00")
            //    {
            //     completeData += hexValue;
            //   }
            completeData += String(a, HEX);
        }
        byteCount++;
    }

    return completeData;
}

String extractVpAddress(const String &inputHex, const String &vpAddressPattern) { // Not used
    int startPos = inputHex.indexOf(vpAddressPattern);
    if (startPos == -1) {
        // VP address pattern not found in the input
        return "";
    }

    // Extract the VP address
    String extractedVpAddress = inputHex.substring(startPos, startPos + vpAddressPattern.length());
    return extractedVpAddress;
}

String removeFirst6Bytes(const String &input) {
    // Check if the string has at least 12 characters (6 bytes)
    if (input.length() < 12) {
        return ""; // Return an empty string if there are not enough characters
    }

    // Return the substring starting from the 13th character
    return input.substring(12);
}

String remove13Characters(const String &input) {
    // Check if the string has at least 12 characters (6 bytes)
    if (input.length() < 13) {
        return ""; // Return an empty string if there are not enough characters
    }

    // Return the substring starting from the 13th character
    return input.substring(13);
}

String removeFirst7Bytes(const String &input) {
    // Check if the string has at least 12 characters (6 bytes)
    if (input.length() < 14) {
        return ""; // Return an empty string if there are not enough characters
    }

    // Return the substring starting from the 13th character
    return input.substring(14);
}

String processFourthAndFifthBytes(const String &checkData) {
    String vpAddress = extractDataBetweenPatterns(checkData, "a5", "83");
    int start_index;
    int end_index;
    // Ensure the string is long enough (at least 11 characters for 5.5 bytes)
    if (checkData.length() < 11) {
        return ""; // Not enough data to process
    }

    if (vpAddress.length() == 1) {
        start_index = 7;
        end_index = 11;
    } 
    else {
        start_index = 8;
        end_index = 12;
    }
    // Extract 4th and 5th bytes (8th to 11th characters in the string)
    String fourthAndFifthBytes = checkData.substring(start_index, end_index);

    // You can now process this extracted data as needed
    // For example, convert it to a different format, interpret it, etc.

    return fourthAndFifthBytes; // Return the processed data
}

void startCheckingPassword(uint16_t passwordDisplay, uint16_t passwordIcon, const String &checkData) {
    if (printDebug) Serial.println("Data from password field: " + checkData);
    String dataLength = extractDataBetweenPatterns(checkData, "a5", "83");
    String removerHeaders;

    if (dataLength.length() == 1) {
        removerHeaders = removeFirst6Bytes(checkData);
    }
    else {
        removerHeaders = remove13Characters(checkData);
    }

    String passwordData = extractDataBeforeMarker(removerHeaders, "ffff");
    if (printDebug) Serial.println(passwordData);
    String password = hexToString(passwordData);
    String hexPassword = toHexString(password); // Convert data to a hex string
    if (printDebug) Serial.println("Hex Data :" + hexPassword);

    delay(100);
    sendReadCommand(passwordIcon, 0x1);
    delay(100);
    String iconRead = tempreadResponse();
    if (printDebug) Serial.println("Read Icon :" + iconRead);
    if (checkLast3DigitsMatch(iconRead, showPassword)) {
        if (printDebug) Serial.println("Show Password");
        delay(100);
        writeString(passwordDisplay, hexPassword);
    }
    else if (checkLast3DigitsMatch(iconRead, hidePassword)) {
        if (printDebug) Serial.println("Hide Password");
        String hiddenPassword = "";
        for (int i = 0; i < password.length(); i++)
        {
            hiddenPassword += '*';
        }
        if (printDebug) Serial.println("Hidden Password: " + hiddenPassword);
        String hexData = toHexString(hiddenPassword);
        if (printDebug) Serial.println("Hex Data: " + hexData);
        writeString(passwordDisplay, hexData);
    }
}

String processPasswordDisplay(uint16_t readPassword, uint16_t passwordDisplay, uint16_t passwordIcon) {
    // Read password
    delay(100);
    sendReadCommand(readPassword, 0x28);
    delay(100);
    String passWord = tempreadResponse();
    if (printDebug) Serial.println("Password Data :" + passWord);

    String removerHeaders = removeFirst7Bytes(passWord);
    String passwordData = "";

    if (containsPattern(passWord, "ffff"))
        passwordData = extractDataBeforeMarker(removerHeaders, "ffff");
    else
        passwordData = extractDataBeforeMarker(removerHeaders, "0000");

    if (printDebug) Serial.println("Actual Data :" + passwordData);
    String password = hexToString(passwordData);
    delay(100);
    String hexPassword = toHexString(password); // Convert data to a hex string

    // Read icon
    delay(100);
    sendReadCommand(passwordIcon, 0x1);
    delay(100);
    String iconRead = tempreadResponse();
    if (printDebug) Serial.println("Read Icon Again :" + iconRead);

    if (checkLast3DigitsMatch(iconRead, showPassword)) {
        if (printDebug) Serial.println("Show Password");
        writeString(passwordDisplay, hexPassword);
    }
    else if (checkLast3DigitsMatch(iconRead, hidePassword)) {
        if (printDebug) Serial.println("Hide Password");
        String hiddenPassword = "";
        for (int i = 0; i < password.length(); i++) {
            hiddenPassword += '*';
        }
        String hexData = toHexString(hiddenPassword);
        writeString(passwordDisplay, hexData);
    }
    return password;
}

// Read the Response from LCD
String readText() { // Not used
    String completeData = "";

    while (Serial1.available()) {
        char a = Serial1.read();
        completeData += String(a);
    }

    return completeData;
}

// Remove the zeros and Converted to String
String hexToStringRemovedZeros(const String &hex) { // Not used
    String ascii = "";

    // Skip leading zeros
    unsigned int start = 0;
    while (start < hex.length() && hex.substring(start, start + 2) == "00") {
        start += 2;
    }

    // Process the string until trailing zeros
    for (unsigned int i = start; i < hex.length(); i += 2) {
        String part = hex.substring(i, i + 2);

        // Break if trailing zeros start
        if (part == "00") {
            break;
        }

        char ch = (char)strtol(part.c_str(), NULL, 16);
        ascii += ch;
    }

    return ascii;
}

// Convert Hexadecimal String to Text
String hexToString(const String &hex) {
    String ascii = "";
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        String part = hex.substring(i, i + 2);
        char ch = (char)strtol(part.c_str(), NULL, 16);
        ascii += ch;
    }
    return ascii;
}

// Check last four digits: return true if condition meet else false
bool checkLastFourDigitsMatch(const String &inputString, const String &targetDigits) {
    // Ensure the input string is at least 4 characters long
    if (inputString.length() < 4) {
        return false;
    }

    // Extract the last 4 characters of the input string
    String lastFourDigits = inputString.substring(inputString.length() - 4);

    // Compare the last 4 digits with the target digits
    return lastFourDigits.equals(targetDigits);
}

bool checkLast3DigitsMatch(const String &inputString, const String &targetDigits) {
    // Ensure the input string is at least 3 characters long
    if (inputString.length() < 3) {
        return false;
    }

    // Extract the last 3 characters of the input string
    String lastThreeDigits = inputString.substring(inputString.length() - 3);

    // Compare the last 3 digits with the target digits
    return lastThreeDigits.equals(targetDigits);
}

String readResponse() {
    const int maxResponseLength = 8;
    char responseBytes[maxResponseLength]; // Array to store the bytes
    int availableBytes = Serial1.available();

    // Check if available bytes are less than 5, then keep reading
    while (availableBytes < 5) {
        delay(100);
        availableBytes = Serial1.available();
    }

    // Read bytes from Serial1, up to the maximum response length
    int bytesRead = 0;
    while (bytesRead < maxResponseLength && Serial1.available()) {
        responseBytes[bytesRead] = Serial1.read();
        bytesRead++;
    }

    // Convert the byte array to a hexadecimal string
    String dataStr = "";
    for (int i = 0; i < bytesRead; i++) {
        if (responseBytes[i] < 0x10) {
            // Add leading zero for single digit hex values
            dataStr += "0";
        }
        dataStr += String(responseBytes[i], HEX);
    }

    if (printDebug) {
        // For debugging: Print the full response
        Serial.print("Full Response: ");
        Serial.println(dataStr);
    }

    return dataStr + "," + String(availableBytes);
}

// Login Credentials for both Client and Admin Panel
Credentials retrieveCredentials(uint16_t ssidCommand, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon) {
    // Read SSID
    if (printDebug) Serial.println("In retrieveCredentials");
    Credentials creds;
    sendReadCommand(ssidCommand, 0x28);
    delay(100);
    String ssidData = dummyReadResponse();
    delay(100);
    String extractedSSID = extractDataBeforeMarker(ssidData, "ffff");
    creds.ssid = hexToString(extractedSSID);
    if (printDebug) Serial.println("SSID: " + creds.ssid);

    // Read Password
    creds.password = processPasswordDisplay(passwordCommand, passwordDisplay, passwordIcon);
    if (printDebug) Serial.println("Password: " + creds.password);

    return creds;
}

// Login Credentials for Client Panel
void processClientLogin(uint16_t username, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon) {
    // Read Username
    if (printDebug) Serial.println("In processClientLogin");

    sendReadCommand(username, 0x28);
    delay(100);
    String usernameData = dummyReadResponse();
    delay(100);
    String extractedusername = extractDataBeforeMarker(usernameData, "ffff");
    String saveusername = hexToString(extractedusername);

    if (saveusername != "") {
        preferences.putString("client_username", saveusername); // Save Username

        // Read Password
        String temppassword = processPasswordDisplay(passwordCommand, passwordDisplay, passwordIcon);

        preferences.putString("client_password", temppassword); // Save Password

        if (printDebug) {
            // Only for debugging
            String Username = preferences.getString("client_username", "");
            Serial.println("Username: " + Username);

            String password = preferences.getString("client_password", "");
            Serial.println("password: " + password);
            delay(100);
        }
    }
}

// Login Credentials for Admin Panel
void processAdminLogin(uint16_t username, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon) {
    // Read username
    if (printDebug) Serial.println("In processAdmin_Login");

    sendReadCommand(username, 0x28);
    delay(100);
    String usernameData = dummyReadResponse();
    delay(100);
    String extractedusername = extractDataBeforeMarker(usernameData, "ffff");
    String saveusername = hexToString(extractedusername);

    if (saveusername != "") {
        preferences.putString("admin_username", saveusername); // Save username

        // Read Password
        String temppassword = processPasswordDisplay(passwordCommand, passwordDisplay, passwordIcon);

        preferences.putString("admin_password", temppassword); // Save Password

        if (printDebug) {
            // Only for debugging
            String Username = preferences.getString("admin_username", "");
            Serial.println("Username: " + Username);

            String pass = preferences.getString("admin_password", "");
            Serial.println("PASSWORD: " + pass);
        }
    }
}

// Wi-Fi Credentials
void processWiFiCredentials(uint16_t ssid, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon) {
    // Read ssid
    if (printDebug) Serial.println("In processWiFiCredentials");

    sendReadCommand(ssid, 0x28);
    delay(100);
    String ssidData = dummyReadResponse();
    delay(100);
    String extractedssidData = extractDataBeforeMarker(ssidData, "ffff");
    String savessidData = hexToString(extractedssidData);

    preferences.putString("internetSSID", savessidData); // Save SSID

    // Read Password
    String temppassword = processPasswordDisplay(passwordCommand, passwordDisplay, passwordIcon);

    preferences.putString("internetPass", temppassword); // Save Password

}

String readOneData(uint16_t ssidCommand) {
    sendReadCommand(ssidCommand, 0x28);
    delay(100);
    String Data = dummyReadResponse();
    delay(100);
    String extractedData = extractDataBeforeMarker(Data, "ffff");
    String readData = hexToString(extractedData);
    if (printDebug) Serial.println("Unique Data: " + readData);
    return readData;
}

void performLoginCheck(bool &clientLogin, bool &adminLogin) { // Not used
    sendReadCommand(LOGIN, 0x01);
    delay(100);
    String loginData = tempreadResponse();

    if (checkLastFourDigitsMatch(loginData, clientPanelDigits)) {
        if (printDebug) Serial.println("Client Panel Login successful!");
        clientLogin = true;
        adminLogin = false;
    }
    else if (checkLastFourDigitsMatch(loginData, adminPanelDigits)) {
        if (printDebug) Serial.println("Admin Panel Login successful!");
        adminLogin = true;
        clientLogin = false;
    }
    else {
        if (printDebug) Serial.println("Login failed. Last 4 digits do not match.");
        clientLogin = false;
        adminLogin = false;
    }
}

void readPage() { // Not used
    byte readpage[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x14, 0x01};
    Serial1.write(readpage, sizeof(readpage));
}

// Page Switching
void pageSwitch(byte pageNo) {
    // Frame array
    byte open[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x84, 0x5A, 0x01, 0x00, pageNo};
    Serial1.write(open, sizeof(open));
    //    for (int i = 0; i < sizeof(open); i++) {
    //     Serial.print("Open Byte ");
    //     Serial.print(i);
    //     Serial.print(": ");
    //     Serial.println(open[i], HEX);
    // }
    delay(500);
}

// Icon Switching
void Display_AC_DEAC_Icon(byte iconNo) { // 00 or 01
    // Frame array
    byte open[] = {0x5A, 0xA5, 0x05, 0x82, 0x62, 0x18, 0x00, iconNo};
    Serial1.write(open, sizeof(open));
    //    for (int i = 0; i < sizeof(open); i++) {
    //     Serial.print("Open Byte ");
    //     Serial.print(i);
    //     Serial.print(": ");
    //     Serial.println(open[i], HEX);
    // }
    delay(500);
}

// Icon Switching
void Display_Wifi_Icon(byte iconNo) { // 01 wifi connected or 02 wifi not connected
    // Frame array
    byte open[] = {0x5A, 0xA5, 0x05, 0x82, 0x67, 0x31, 0x00, iconNo};
    Serial1.write(open, sizeof(open));
    delay(50);
}

// Icon Switching
void Display_Battery_Icon(byte iconNo) { // 03 on charging or 04 on battery
    // Frame array
    byte open[] = {0x5A, 0xA5, 0x05, 0x82, 0x67, 0x33, 0x00, iconNo};
    Serial1.write(open, sizeof(open));
    delay(50);
}

// Icon Switching
void Display_DeviceConfigured_Icon(byte iconNo) { // 05 device configured 
    // Frame array
    byte open[] = {0x5A, 0xA5, 0x05, 0x82, 0x67, 0x36, 0x00, iconNo};
    Serial1.write(open, sizeof(open));
    delay(50);
}

// Lcd Reset
void systemReset() { // Not used
    byte reset[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x04, 0x55, 0xAA, 0x5A, 0xA5};
    Serial1.write(reset, sizeof(reset));
}

// Compare String
bool compareCredentials(String ssid, String password) {
    return ssid == predefinedusername && password == predefinedPassword;
}

// Compare Internet
bool compareInternetCredentials(String ssid, String password) {
    return ssid == predefinedInternetSSID && password == predefinedInternetPassword;
}

String toHexString(const String &data) {
    String hexString = "";
    for (char c : data) {
        char hex[3];
        sprintf(hex, "%02X", c); // Convert each character to a two-digit hexadecimal
        hexString += hex;
    }
    return hexString;
}

void sendDataToLcd(uint16_t vpAddress, const String &data) { // Not used
    String hexData = toHexString(data);
    writeString(vpAddress, hexData);
}

// Function to convert two hexadecimal characters to one byte
byte hexCharToByte(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    return 0;
}

byte hexStringToByte(const String &hex) { // Not used
    return (hexCharToByte(hex.charAt(0)) << 4) | hexCharToByte(hex.charAt(1));
}

// Helper function to convert two hexadecimal characters to one byte
byte hexToByte(const char *hex) {
    return (byte)strtol(hex, NULL, 16);
}

void writeString(uint16_t address, const String &hexData) {
    size_t dataLength = hexData.length() / 2; // Each byte is represented by two hex characters
    byte *charData = new byte[dataLength];

    // Convert hex string to bytes
    for (size_t i = 0; i < dataLength; i++) {
        charData[i] = hexToByte(hexData.substring(i * 2, i * 2 + 2).c_str());
    }

    // Calculate total frame size: Header (6 bytes) + dataLength
    size_t frameSize = 6 + dataLength;
    byte *frame = new byte[frameSize];

    // Construct the frame
    frame[0] = 0x5A;
    frame[1] = 0xA5;
    frame[2] = frameSize - 3;
    frame[3] = 0x82;
    frame[4] = (byte)(address >> 8);
    frame[5] = (byte)(address & 0xFF);

    // Copy the character data into the frame
    memcpy(frame + 6, charData, dataLength);

    // Write the frame to the serial port
    Serial1.write(frame, frameSize);

    // Clean up the dynamically allocated buffers
    delete[] frame;
    delete[] charData;

    // Add a delay to allow the display to process the data
    delay(100);
}

String ReturnJson(String url, DynamicJsonDocument &doc) {
    String jsonResult = ""; // String to hold the JSON result

    HTTPClient http;
    delay(5);
    http.begin(url); // API URL
    if (printDebug) Serial.println("URL " + url);
    int httpCode = http.GET();
    if (printDebug) Serial.println(httpCode);

    if (httpCode > 0) { // Check for the returning code
        String payload = http.getString();
        if (printDebug) Serial.println(payload);

        // Parse JSON (optional, if you need to process it)
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            if (printDebug) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                // return;
            }
        }

        jsonResult = payload; // Store the JSON string
    }
    else {
        if (printDebug) Serial.println("Error on HTTP request");
        jsonResult = "Error"; // Error indication
    }

    http.end(); // Free resources

    return jsonResult; // Return the JSON string
}

void sendIconcommand(uint16_t pageVP, byte icon0, byte icon1, byte icon2, byte icon3, byte icon4, byte icon5) {
    byte iconcommand[] = {0x5A, 0xA5, 0x2D, 0x82, (byte)(pageVP >> 8), (byte)(pageVP & 0xFF), 0x3D, 0x07, 0x00, 0x06, 0x03, 0x3E, 0x00, 0xC8, 0x00, icon0,
                          0x03, 0x3E, 0x01, 0x04, 0x00, icon1,
                          0x03, 0x3E, 0x01, 0x40, 0x00, icon2,
                          0x03, 0x3E, 0x01, 0x7C, 0x00, icon3,
                          0x03, 0x3E, 0x01, 0xB8, 0x00, icon4,
                          0x03, 0x3E, 0x01, 0xF4, 0x00, icon5, 0xFF, 0x00};

    
    // // Printing on serial monitor to check command
    // for (int i = 0; i < sizeof(iconcommand); i++)
    // {
    //     Serial.print("Icon Byte ");
    //     Serial.print(i);
    //     Serial.print(": ");
    //     Serial.println(iconcommand[i], HEX);
    // }
    
    Serial1.write(iconcommand, sizeof(iconcommand));
    if (printDebug) Serial.println("Show icon command sent");
    delay(300);
}

String extractKeycode(const String &input) {
    // Check if the string has at least 13 characters
    if (input.length() < 13 || input.length() > 17) {
        return ""; // Return an empty string
    }

    // Return the substring starting from the 12th character
    return input.substring(13);
}

String extractPageVP(const String &input, const String &vpAddressPattern) {
    int startPos = input.indexOf(vpAddressPattern);
    // Check if the string has at least 8 characters
    if (input.length() < 8 || input.length() > 17 || startPos == -1) {
        return ""; // Return an empty string
    }

    // Extract the VP address
    String extractedVpAddress = input.substring(startPos, startPos + 4);
    return extractedVpAddress;
}

String concatinate_checkboxData() { // Not used
    String checkboxData = controlFunction + speakerActivate + firemanActivateBox + bellRingSystemActivation + batteryHealth + ledLightOnWhite + ledRedActivation + smsReceivedFyreboxActivated + lcdScreenWork + systemActivateWeeklySelfTest + evacuatioDiagram + arrowWorking + permanentPower + illuminatedSignalsWorking + batteriesReplacement + flashSignPanel + unitSecured + faciaComponentSecured + evacuationDiagramUptodate + fyreboxFreeObstructions + LogbookUptodate + fyreboxUnitWipedCleaned + anyDamageBox + anyRustUnit;

    return checkboxData;
}

bool isActivityDetected() { // Not used
    // Implement logic to check for activity (e.g., checkData is not empty)
    if (checkData != "")
        return true;

    else
        return false;
}

String getResponse(String url) {
    HTTPClient http;

    // Send request
    http.begin(url);
    int httpResponseCode = http.GET(); // Use GET method for request

    String STATUS = "";
    String orgID = "";

    if (httpResponseCode > 0) {
        // if (printDebug) Serial.println("Response code: " + httpResponseCode);
        String response = http.getString();

        // Parse JSON
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        STATUS = doc["STATUS"].as<String>();
        // if (printDebug) Serial.println("STATUS: " + STATUS);
        orgID = doc["USER_DATA"]["org_id"].as<String>();

        if (STATUS == "ERROR") {
            String ERROR_DESCRIPTION = doc["ERROR_DESCRIPTION"].as<String>();
            // if (printDebug) Serial.println("ERROR_DESCRIPTION: " + ERROR_DESCRIPTION);

              return ERROR_DESCRIPTION;
        }
        else if (STATUS == "SUCCESSFUL") {
            // return STATUS;
            if (printDebug) {
                // Serial.print("Response: ");
                // Serial.println(response);
            }
        }
    }
    else {
        if (printDebug) {
            // Serial.print("Error in request, error code: ");
            // Serial.println(httpResponseCode);

            // // Print HTTP error
            // Serial.print("HTTP error: ");
            // Serial.println(http.errorToString(httpResponseCode));
        }
    }

    // Free resources
    http.end();

    return STATUS;
    // return orgID;
}

String getorg_ID(String url) {
    HTTPClient http;

    // Send request
    http.begin(url);
    int httpResponseCode = http.GET(); // Use GET method for request

    String STATUS = "";
    String orgID = "";

    if (httpResponseCode > 0) {
        // if (printDebug) Serial.println("Response code: " + httpResponseCode);
        String response = http.getString();

        // Parse JSON
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        STATUS = doc["STATUS"].as<String>();
        // if (printDebug) Serial.println("STATUS: " + STATUS);
        orgID = doc["USER_DATA"]["org_id"].as<String>();

        if (STATUS == "ERROR") {
            String ERROR_DESCRIPTION = doc["ERROR_DESCRIPTION"].as<String>();
            // if (printDebug) Serial.println("ERROR_DESCRIPTION: " + ERROR_DESCRIPTION);

              return ERROR_DESCRIPTION;
        }
        else if (STATUS == "SUCCESSFUL") {
            // return STATUS;
            if (printDebug) {
                // Serial.print("Response: ");
                // Serial.println(response);
            }
        }
    }
    else {
        if (printDebug) {
            // Serial.print("Error in request, error code: ");
            // Serial.println(httpResponseCode);

            // // Print HTTP error
            // Serial.print("HTTP error: ");
            // Serial.println(http.errorToString(httpResponseCode));
        }
    }

    // Free resources
    http.end();

    // return STATUS;
    return orgID;
}

int getWeekNumberByMonth(int day, int month, int year) {
    // Array to store the number of days in each month
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Check if the year is a leap year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[2] = 29; // Update February days for leap years
    }

    // Calculate the day of the week the month starts on
    int startDayOfMonth = (day + 6) % 7; // Assuming week starts from Sunday (0 for Sunday, 6 for Saturday)

    // Calculate the number of days passed since the beginning of the month
    int daysPassed = day;

    // Adjust for the start day of the month
    daysPassed -= startDayOfMonth;

    // Calculate the number of full weeks passed
    int weeksPassed = daysPassed / 7;

    return weeksPassed;
}

int getWeekNumberByYear(int day, int month, int year) {
    // Array to store the number of days in each month
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Check if the year is a leap year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[2] = 29; // Update February days for leap years
    }

    // Calculate the number of days passed since the beginning of the year
    int daysPassed = day;
    for (int i = 1; i < month; i++) {
        daysPassed += daysInMonth[i];
    }

    // Calculate the number of full weeks passed
    int weeksPassed = daysPassed / 7;

    return weeksPassed;
}

void checkGPSTask(void *parameter) { // Not used
    if (printDebug) Serial.println("GPS Task started, waiting for signal...");

    while (1) {
        if (getGPSTime()) {
            if (printDebug) Serial.println("GPS signal acquired.");
            break; // GPS time obtained, exit the loop
        }
        else {
            if (printDebug) Serial.println("Waiting for GPS signal...");
            delay(1000);
        }
    }

    if (printDebug) Serial.println("GPS Task completed.");
    vTaskDelete(xHandlegps); // Delete this task when done
}

bool getGPSTime() {
    if (SerialGPS.available()) {
        if (printDebug) Serial.println("GPS Data Availabole");
        String gpsData = SerialGPS.readStringUntil('\n'); // Read a line of GPS data
        if (printDebug) {
            Serial.print("Raw GPS Data: ");
            Serial.println(gpsData);
        }

        // Check if the line is a GPRMC string
        if (gpsData.startsWith("$GPRMC")) {
            if (printDebug) Serial.println("GPRMC");
            return processGPRMC(gpsData);
        }
    }
    return false;
}

bool processGPRMC(String gprmcString) {
    if (printDebug) Serial.println("INGPRMC");
    int timeIndex = gprmcString.indexOf(',');
    int validityIndex = gprmcString.indexOf(',', timeIndex + 1);
    int latitudeIndex = gprmcString.indexOf(',', validityIndex + 1);
    int northSouthIndex = gprmcString.indexOf(',', latitudeIndex + 1);
    int longitudeIndex = gprmcString.indexOf(',', northSouthIndex + 1);
    int eastWestIndex = gprmcString.indexOf(',', longitudeIndex + 1);
    int dateIndex = gprmcString.indexOf(',', eastWestIndex + 1);
    for (int i = 0; i < 2; ++i) { // Skipping two unused fields
        dateIndex = gprmcString.indexOf(',', dateIndex + 1);
    }

    // Extracting data
    String timeString = gprmcString.substring(timeIndex + 1, validityIndex);
    String validity = gprmcString.substring(validityIndex + 1, latitudeIndex);
    String latitudeString = gprmcString.substring(latitudeIndex + 1, northSouthIndex);
    String longitudeString = gprmcString.substring(longitudeIndex + 1, eastWestIndex);
    String dateString = gprmcString.substring(dateIndex + 1, gprmcString.indexOf(',', dateIndex + 1));

    // Check if data is valid
    if (validity != "A") {
        return false;
    }

    int hour = timeString.substring(0, 2).toInt();
    int minute = timeString.substring(2, 4).toInt();
    int second = timeString.substring(4, 6).toInt();
    int day = dateString.substring(0, 2).toInt();
    int month = dateString.substring(2, 4).toInt();
    int year = dateString.substring(4, 6).toInt() + 2000; // Adjust for century

    if (printDebug) {
        Serial.print("Validity: ");
        Serial.println(validity);
        Serial.print("Latitude: ");
        Serial.println(latitudeString);
        Serial.print("Longitude: ");
        Serial.println(longitudeString);
        Serial.print("Date: ");
        Serial.println(dateString);
    }

    // Set the RTC with the GPS time
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    if (printDebug) {
        Serial.print("RTC set to GPS time: ");
        Serial.println(rtc.now().timestamp());
    }
    String locationOfUnit = "Latitude: " + latitudeString + " Longitude: " + longitudeString;
    String locationBytes = toHexString(locationOfUnit);
    delay(10);
    writeString(VP_LOCATION_OF_UNIT, locationBytes);
    delay(10);

    return true;
}

void dateTimeTask(void *parameter) {
    for(;;) { // Infinite loop for the task
        DateTime now = rtc.now();

        DAY = String(now.day());
        MONTH = String(now.month());
        YEAR = String(now.year() % 100);

        HOUR = String(now.hour());
        MINUTE = String(now.minute());

        if (DAY.length() == 1) {
            DAY = "0" + DAY;
        }

        if (MONTH.length() == 1) {
            MONTH = "0" + MONTH;
        }

        if (HOUR.length() == 1) {
            HOUR = "0" + HOUR;
        }
        if (MINUTE.length() == 1) {
            MINUTE = "0" + MINUTE;
        }

        dateString = DAY + "/" + MONTH + "/" + YEAR;
        timeString = HOUR + ":" + MINUTE;

        // String dateString = String(now.day()) + "/" + String(now.month()) + "/" + String(now.year() % 100);
        // String timeString = String(now.hour()) + ":" + String(now.minute());

        if (printDebug) {
            // Serial.print("ESP32 RTC Date Time: ");
            // Serial.println(timeString);
            // Serial.println(dateString);
        }

        String dateBytes = toHexString(dateString);
        delay(10);
        writeString(VP_UNIT_DATE, dateBytes);
        delay(10);

        String timeBytes = toHexString(timeString);
        delay(10);
        writeString(VP_UNIT_TIME, timeBytes);

        String response = tempreadResponse();

        if (response.length() > 14)
        {
            response = "";
        }
        delay(1000); // Wait for a second before updating again
    }

    vTaskDelete(xHandledatetime); // Delete the task if it ever breaks out of the loop
}

void loginTask(void *parameter) {
    if (printDebug) Serial.println("Login Task Started");
    for(;;) {
        checkData = tempreadResponse();

        // Ack response from lcd
        if (checkData == "5aa53824f4b") checkData = "";
        if (checkData != "") {
            if (printDebug) Serial.println("Data in loginTask:" +checkData);
        }
        // if deviceConfiguredFlag is false accept copyright
        if (containsPattern(checkData, "2e70") || deviceConfiguredFlag) { 

            if (!deviceConfiguredFlag) { // First time startup
                resetVP(clientLoginStatus);
                pageSwitch(INTERNETPAGE);
                if (printDebug) Serial.println("INTERNETPAGE");
                configureInternet();
                if(wifiConnectedFlag) {
                    resetVP(clientLoginStatus);
                    pageSwitch(CLIENTPAGE);
                    if (printDebug) Serial.println("CLIENTPAGE");
                    configureLogin();
                    resetVP(clientLoginStatus);
                    pageSwitch(UNIQUE_KEY_PAGE);
                    getUniqueKey();

                    // configure device 
                    if (printDebug) Serial.println("Start Config Device");
                    vTaskResume(xHandleconfigdevice); // Resume the next task before exit
                    break;
                }
            }
            // Start slideShow
            slideShowFlag = true;
            slideShow();

            if (printDebug) Serial.println("start Wifi Task");
            vTaskResume(xHandlewifi);

            if (printDebug) Serial.println("Start LoRa task");
            vTaskResume(xHandleLoRa); // Resume the next task

            if (printDebug) Serial.println("Start receiving task");
            vTaskResume(xHandleRecmessage); // Resume the next task 

            // if (printDebug) Serial.println("Start button task");
            // vTaskResume(xHandleButton); // Resume the next task

            if (printDebug) Serial.println("Start Homepage Tasks");
            vTaskResume(xHandlehomepage); // Resume the next task before exit

            break;
        }        
        delay(50);
    }
    if (printDebug) {
        Serial.println("Login Task completed");
        Serial.println("Login Task Deleted");
    }
    vTaskDelete(xHandlelogin); // Delete the task
}

void getUniqueKey() {
    if (printDebug) Serial.println("getUniqueKey Started");
    while (true)
    {
        delay(100);
        String uniqueButton = tempreadResponse();
        if (uniqueButton != "") Serial.println("Data from uniqueButton: " + uniqueButton);

        if (containsPattern(uniqueButton, uniqueButtonDigits))
        {
            delay(100);
            String uniqueData = readOneData(UNIQUE_KEY);
            delay(100);
            storedUniqueData = uniqueData;

            if (storedUniqueData != uniqueData)
            {
                resetVP(clientLoginStatus);
                Serial.println("Database Data is . " + storedUniqueData);
                Serial.println("Unique button pressed. Updating data. " + uniqueData);
                Serial.println("Device Failed to Connect to Fyrebox Network");
                String LoginStatus = "Device Failed to Connect to Fyrebox Network";
                String LoginStatusBytes = toHexString(LoginStatus);
                delay(100);
                writeString(clientLoginStatus, LoginStatusBytes);
                delay(100);
                Serial.println("Updating Device:");
                storedUniqueData = uniqueData;
                Serial.println("Database Updated is . " + storedUniqueData);
                continue;
            }
            else
            {
                resetVP(clientLoginStatus);
                delay(5);
                Serial.println("Updated Unique Data: " + storedUniqueData);
                Serial.println("Device Succesfully Added to Fyrebox Network");
                String LoginStatus = "Device Succesfully Added to Fyrebox Network";
                String LoginStatusBytes = toHexString(LoginStatus);
                delay(100);
                writeString(clientLoginStatus, LoginStatusBytes);
                delay(1000);
                uniqueKeyFlag = true;
                resetVP(clientLoginStatus);
                break;
            }
        }
        // Skip button
        else if (containsPattern(uniqueButton, "6211")) {
            if (deviceConfiguredFlag) {
                delay(5);
                break;
            }
            else{
                String message = "Please Enter Unique key"; 
                showMessage(clientLoginStatus, message);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }
    }
    if (printDebug) Serial.println("getUniqueKey completed");
}

void readeyeIcon(String temppassword, uint16_t passwordvp, uint16_t passwordIcon, uint16_t passwordDisplay) {
    // Important to write password first
    String hexdatapassword = toHexString(temppassword);
    writeString(passwordvp, hexdatapassword); // Display password

    // Read icon
    delay(100);
    sendReadCommand(passwordIcon, 0x1);
    delay(100);
    String iconRead = tempreadResponse();
    if (printDebug) Serial.println("Read Icon Again :" + iconRead);

    if (checkLast3DigitsMatch(iconRead, showPassword)) {
        if (printDebug) Serial.println("Show Password");
        hexdatapassword = toHexString(temppassword);
        writeString(passwordDisplay, hexdatapassword); // Display password
    }
    else if (checkLast3DigitsMatch(iconRead, hidePassword)) {
        if (printDebug) Serial.println("Hide Password");
        String hiddenPassword = "";
        for (int i = 0; i < temppassword.length(); i++) {
            hiddenPassword += '*';
        }
        // if (printDebug) Serial.println("Hidden Password: " + hiddenPassword);
        String hexData = toHexString(hiddenPassword);
        // if (printDebug) Serial.println("Hex Data: " + hexData);
        writeString(passwordDisplay, hexData);
    }
}

void connectInternet() {
    if (printDebug) Serial.println("connectInternet Started");
    while (true) {
        String internetData = tempreadResponse();
        if (internetData != "") {
            if (printDebug) Serial.println("Data in connect internet:" + internetData);
        }
        delay(100);

        // Start of Auto connect
        internetSSID = preferences.getString("internetSSID", "");
        internetPassword = preferences.getString("internetPass", "");

        // if credentials are available
        if ((!internetSSID.isEmpty() && !internetPassword.isEmpty())) {
            WiFi.begin(internetSSID.c_str(), internetPassword.c_str());

            unsigned long startAttemptTime = millis();

            while (millis() - startAttemptTime < 10000) { // 10 seconds in milliseconds
                if (WiFi.status() == WL_CONNECTED) {
                    wifiConnectedFlag = true;
                    String wifiMessage = "WiFi Connected";
                    if (printDebug) Serial.println(wifiMessage);
                    showMessage(notificationStatus2, wifiMessage);

                    break;
                }
            }

            if (wifiConnectedFlag) {
                // String message2 = "Downloading Audio";
                // if (printDebug) Serial.println(message2);
                // showMessage(notificationStatus2, message2);

                // download_audio();

                // message2 = "Audio Downloaded";
                // if (printDebug) Serial.println(message2);
                // showMessage(notificationStatus2, message2);
                // delay(100);

                break;
            }

            else if (!wifiConnectedFlag) {
                internetSSID = "";
                internetPassword = "";
                resetVP(INTERNET_PASSWORD);
                resetVP(INTERNET_PASSWORD_DISPLAY);
                // resetVP(INTERNET_PASSWORD_ICON);
                resetVP(INTERNET_CONNECT_BUTTON);

                String InternetLoginStatus = "Failed to connect to WiFi within 10 seconds.";
                if (printDebug) Serial.println(InternetLoginStatus);

                showMessage(notificationStatus2, InternetLoginStatus);
                break; // to start wifi task in both cases i.e wifi connected, wifi not connected
            }
        }
    }
    if (printDebug) Serial.println("connectInternet Completed");
}

void configureInternet() {
    if (printDebug) Serial.println("configureInternet Started");
    while (true) {
        String configure_internetData = tempreadResponse();
        if (configure_internetData != "") Serial.println("Data in configure internet:" + configure_internetData);
        delay(100);

        if (containsPattern(configure_internetData, "ffff")) {
            // if (printDebug) Serial.println("Data VP Address :" + configure_internetData);
            resetVP(INTERNET_PASSWORD_DISPLAY);
            delay(100);

            String vpAddress = processFourthAndFifthBytes(configure_internetData);
            // if (printDebug) Serial.println("Vp Address Internet :" + vpAddress);

            if (vpAddress == "33fa") {
                delay(100);
                startCheckingPassword(INTERNET_PASSWORD_DISPLAY, INTERNET_PASSWORD_ICON, configure_internetData);
            }
        }

        //  Check Icon
        else if (containsPattern(configure_internetData, "345e")) {
            if (printDebug) Serial.println("In Wi-Fi icon");
            delay(100);
            processPasswordDisplay(INTERNET_PASSWORD, INTERNET_PASSWORD_DISPLAY, INTERNET_PASSWORD_ICON);
        }

        // Connect button
        else if (containsPattern(configure_internetData, "345f")) {
            delay(100);
            processWiFiCredentials(INTERNET_SSID, INTERNET_PASSWORD, INTERNET_PASSWORD_DISPLAY, INTERNET_PASSWORD_ICON);

            internetSSID = preferences.getString("internetSSID", "");
            internetPassword = preferences.getString("internetPass", "");

            if (printDebug) {
                Serial.println("Saved Internet SSID: " + internetSSID);
                Serial.println("Saved Internet Password: " + internetPassword);
            }
            delay(100);

            if (!internetSSID.isEmpty() && !internetPassword.isEmpty()) {
                if (printDebug) Serial.println("Checking Wifi");
                WiFi.begin(internetSSID.c_str(), internetPassword.c_str());

                unsigned long startAttemptTime = millis();

                while (millis() - startAttemptTime < 10000) { // 10 seconds in milliseconds 
                    if (WiFi.status() == WL_CONNECTED) {
                        wifiConnectedFlag = true;
                        String wifiMessage = "Wifi Connected";
                        if (printDebug) Serial.println(wifiMessage);
                        resetVP(clientLoginStatus);

                        showMessage(clientLoginStatus, wifiMessage);
                        delay(500);
                        showMessage(notificationStatus2, wifiMessage);

                        break;
                    }
                    delay(500);
                    String InternetLoginStatus = "Connecting to WiFi...";
                    if (printDebug) Serial.println(InternetLoginStatus);
                    resetVP(clientLoginStatus);

                    String InternetLoginStatusBytes = toHexString(InternetLoginStatus);
                    delay(100);
                    writeString(clientLoginStatus, InternetLoginStatusBytes);
                    delay(500);
                    resetVP(clientLoginStatus);
                }

                if (wifiConnectedFlag) {
                    saveInternetCredentials(internetSSID, internetPassword);
                    break;
                }

                else if (!wifiConnectedFlag) {
                    internetSSID = "";
                    internetPassword = "";
                    resetVP(INTERNET_PASSWORD);
                    resetVP(INTERNET_PASSWORD_DISPLAY);
                    // resetVP(INTERNET_PASSWORD_ICON);
                    resetVP(INTERNET_CONNECT_BUTTON);
                    String InternetLoginStatus = "Failed to connect to WiFi within 10 seconds.";
                    if (printDebug) Serial.println(InternetLoginStatus);
                    resetVP(clientLoginStatus);
                    String InternetLoginStatusBytes = toHexString(InternetLoginStatus);
                    delay(100);
                    writeString(clientLoginStatus, InternetLoginStatusBytes);
                    delay(1000);
                    // break; // to start wifi task in both cases i.e wifi connected, wifi not connected
                }
            }
        }

        // Skip button
        else if (containsPattern(configure_internetData, "6211")) {
            if (deviceConfiguredFlag) {
                delay(5);
                break;
            }
            else{
                String message = "Please Connect to internet"; 
                showMessage(clientLoginStatus, message);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }
    }

    if (printDebug) Serial.println("configureInternet Completed");
}

void checkInternetTask(void *parameter) {
    bool locationFetched = false;
    for (;;) {
        if (WiFi.status() == WL_CONNECTED) {
            if (printDebug) Serial.println("WiFi Connected");
            wifiConnectedFlag = true;
            // Display_Wifi_Icon(0x01); // 01 Wifi connected
            if (!locationFetched) {
                locationFetched = true; // Set flag to true after fetching location
            } 
        } else {
            if (printDebug) Serial.println("WiFi Disconnected. Trying to reconnect...");
            wifiConnectedFlag = false;
            // Display_Wifi_Icon(0x02); // 02 Wifi not connected
            WiFi.disconnect();
            internetSSID = preferences.getString("internetSSID", "");
            internetPassword = preferences.getString("internetPass", "");
            delay(50);
            WiFi.begin(internetSSID.c_str(), internetPassword.c_str());

            // Wait for 10 seconds to see if the connection is successful
            int retryCount = 0;
            while (WiFi.status() != WL_CONNECTED && retryCount < 10) {
                delay(1000);
                // if (printDebug) Serial.print(".");
                retryCount++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                if (printDebug) Serial.println("Reconnected to WiFi");
                wifiConnectedFlag = true;
                // Display_Wifi_Icon(0x01); // 01 Wifi connected
                locationFetched = false; // Reset flag to fetch location again
            } else {
                if (printDebug) Serial.println("Failed to reconnect to WiFi");
                wifiConnectedFlag = false;
                // Display_Wifi_Icon(0x02); // 02 Wifi not connected
            }
        }
        // Wait for 10 seconds before the next check
        delay(10000);
    }
}

void configureLogin() {
    if (printDebug) Serial.println("configureLogin started");
    while (true) {
        checkData = tempreadResponse();
        // Ack response from lcd
        if (checkData == "5aa53824f4b") checkData = "";
        if (checkData != "") {
            if (printDebug) Serial.println("Data in configure login: "+ checkData);
        }

        // Switch user show admin
        if (containsPattern(checkData, switchUser) && containsPattern(checkData, showAdmin)) {
            resetVP(ADMIN_SSID);
            resetVP(ADMIN_PASSWORD);
            resetVP(ADMIN_PASSWORD_DISPLAY);
            resetVP(adminLoginStatus);
            delay(100);

            // Get Admin info
            String tempAdminusername = preferences.getString("admin_username", "");
            String tempAdminpassword = preferences.getString("admin_password", "");
            delay(100);

            if (printDebug) {
                Serial.println("Saved Admin username is: " + tempAdminusername);
                Serial.println("Saved Admin password is: " + tempAdminpassword);
            }
            delay(100);

            String hexdataAdminusername = toHexString(tempAdminusername);
            writeString(ADMIN_SSID, hexdataAdminusername); // Display Admin's username
            delay(100);

            readeyeIcon(tempAdminpassword, ADMIN_PASSWORD, ADMIN_PASSWORD_ICON, ADMIN_PASSWORD_DISPLAY);

            pageSwitch(ADMINPAGE);
            if (printDebug) Serial.println("ADMINPAGE");
        }

        // Switch user show client
        else if (containsPattern(checkData, switchUser) && containsPattern(checkData, showClient)) {
            resetVP(CLIENT_SSID);
            resetVP(CLIENT_PASSWORD);
            resetVP(CLIENT_PASSWORD_DISPLAY);
            resetVP(clientLoginStatus);
            delay(100);

            // Get client info
            String tempClientusername = preferences.getString("client_username", "");
            String tempClientpassword = preferences.getString("client_password", "");
            delay(100);

            if (printDebug) {
                Serial.println("Saved client username is: " + tempClientusername);
                Serial.println("Saved client password is: " + tempClientpassword);
            }
            delay(100);

            String hexdataClientusername = toHexString(tempClientusername);
            writeString(CLIENT_SSID, hexdataClientusername); // Display client's username
            delay(100);

            readeyeIcon(tempClientpassword, CLIENT_PASSWORD, CLIENT_PASSWORD_ICON, CLIENT_PASSWORD_DISPLAY);

            pageSwitch(CLIENTPAGE);
            if (printDebug) Serial.println("CLIENTPAGE");
        }

        // Client panel or admin panel
        else if (containsPattern(checkData, "ffff")) {
            resetVP(CLIENT_PASSWORD_DISPLAY);
            // resetVP(ADMIN_PASSWORD_DISPLAY);
            delay(100);
            String vpAddress = processFourthAndFifthBytes(checkData);
            if (printDebug) Serial.println("Vp Address :" + vpAddress);
            if (vpAddress == "3164") {
                if (printDebug) Serial.println("Client Panel");
                delay(100);
                startCheckingPassword(CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON, checkData);
            }
            else if (vpAddress == "332f") {
                if (printDebug) Serial.println("Admin Panel");
                delay(100);
                startCheckingPassword(ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON, checkData);
            }
        }

        // Client password show/hide icons
        else if (containsPattern(checkData, "31c8")) {
            if (printDebug) Serial.println("In client Icon");
            delay(100);
            processPasswordDisplay(CLIENT_PASSWORD, CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON);
        }

        // Admin password show/hide icons
        else if (containsPattern(checkData, "3393")) {
            if (printDebug) Serial.println("In admin Icon");
            delay(100);
            processPasswordDisplay(ADMIN_PASSWORD, ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON);
        }

        // Admin/Client login button
        else if (containsPattern(checkData, "31ca")) {
            if (checkLastFourDigitsMatch(checkData, clientPanelDigits)) {
                if (printDebug) Serial.println("Data from client login button");
                    clientLogin = true;

                if (RememberIcon(CLIENT_REMEMBER_LOGIN))
                    rememberClient = true;
                else
                    rememberClient = false;
            }
            else if (checkLastFourDigitsMatch(checkData, adminPanelDigits)) {
                if (printDebug) Serial.println("Data from admin login button");
                    adminLogin = true;

                if (RememberIcon(ADMIN_REMEMBER_LOGIN))
                    rememberAdmin = true;
                else
                    rememberAdmin = false;
            }
        }

        // Client Login Mode
        if (clientLogin) {
            if (printDebug) Serial.println("Client Login Mode");
            delay(100);

            processClientLogin(CLIENT_SSID, CLIENT_PASSWORD, CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON);

            String tempusernameClient = preferences.getString("client_username", "");
            String temppasswordClient = preferences.getString("client_password", "");

            if (printDebug) {
                Serial.println("Saved username for client login: " + tempusernameClient);
                Serial.println("Saved password for client login: " + temppasswordClient);
            }
            delay(100);

            String message = "Please Wait"; 
            showMessage(clientLoginStatus, message);

            // assign parameters to process login
            userEmail = tempusernameClient;
            userPassword = temppasswordClient;

            String logInUrl = loginBaseUrl + "operation=" + logInOperation + "&user_email=" + userEmail + "&user_password=" + userPassword;
            // https://fyreboxhub.com/api/get_data.php?operation=user_login_form&user_email=usman@gmail.com&user_password=fyrebox
            DynamicJsonDocument doc(1024);
            delay(100);
            String OrgID = ""; 

            if (OrgID == "" && !APIresponseFlag) {
                // Send http get request to process login
                getOrgId = getorg_ID(logInUrl);
                if (printDebug) Serial.println(getOrgId);
            }
            int newNodeid = getOrgId.toInt();

            devicesDetailsUrl = getDataBaseUrl + "operation=" + getOperation + "&org_id=" + getOrgId;

            if (getOrgId != 0) {
                // Delete the existing mesh object
                delete mesh;
                // Create a new mesh object with the new node ID
                mesh = new RHMesh(driver, newNodeid);

                APIresponseFlag = true;
                loggedIn = true;
                String message = "Login Success"; 
                showMessage(clientLoginStatus, message);
                delay(1000);
                resetVP(clientLoginStatus);
                break;
            } else {
                loggedIn = false;
                String message = "Login Failed"; // Wait here and get login credentials again
                showMessage(clientLoginStatus, message);
                delay(1000);
                resetVP(clientLoginStatus);
            }

            delay(10);
            sendWriteCommand(LOGIN, RESET);
            adminLogin = false;
            clientLogin = false;
        }

        // Admin Login Mode
        else if (adminLogin) {
            if (printDebug) Serial.println("Admin Login Mode");
            delay(100);

            processAdminLogin(ADMIN_SSID, ADMIN_PASSWORD, ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON);

            String tempusernameAdmin = preferences.getString("admin_username", "");
            String temppasswordAdmin = preferences.getString("admin_password", "");

            if (printDebug) {
                Serial.println("Saved username for admin login: " + tempusernameAdmin);
                Serial.println("Saved password for admin login: " + temppasswordAdmin);
            }
            delay(100);

            if (compareCredentials(tempusernameAdmin, temppasswordAdmin)) {
                if (rememberAdmin) {
                    saveAdminCredentials(tempusernameAdmin, temppasswordAdmin);
                }
                else if (!rememberAdmin) {
                    removeAdminCredentials();
                }

                loggedIn = true;
                String LoginStatus = "Login Success";
                String LoginStatusBytes = toHexString(LoginStatus);
                writeString(adminLoginStatus, LoginStatusBytes);
                delay(1000);
                resetVP(adminLoginStatus);
                /*
                pageSwitch(UNIQUE_KEY_PAGE);

                while (true)
                {
                    delay(100);
                    String uniqueButton = tempreadResponse();
                    Serial.println("Data from uniqueButton: " + uniqueButton);

                    if (checkLastFourDigitsMatch(uniqueButton, uniqueButtonDigits))
                    {
                        delay(100);
                        String uniqueData = readOneData(UNIQUE_KEY);
                        delay(100);
                        storedUniqueData = uniqueData;

                        if (storedUniqueData != uniqueData)
                        {
                            resetVP(clientLoginStatus);
                            Serial.println("Database Data is . " + storedUniqueData);
                            Serial.println("Unique button pressed. Updating data. " + uniqueData);
                            Serial.println("Device Failed to Connect to Fyrebox Network");
                            String LoginStatus = "Device Failed to Connect to Fyrebox Network";
                            String LoginStatusBytes = toHexString(LoginStatus);
                            delay(100);
                            writeString(clientLoginStatus, LoginStatusBytes);
                            delay(100);
                            Serial.println("Updating Device:");
                            storedUniqueData = uniqueData;
                            Serial.println("Database Updated is . " + storedUniqueData);
                            continue;
                        }
                        else
                        {
                            resetVP(clientLoginStatus);
                            delay(5);
                            Serial.println("Updated Unique Data: " + storedUniqueData);
                            Serial.println("Device Succesfully Added to Fyrebox Network");
                            String LoginStatus = "Device Succesfully Added to Fyrebox Network";
                            String LoginStatusBytes = toHexString(LoginStatus);
                            delay(100);
                            writeString(clientLoginStatus, LoginStatusBytes);
                            delay(1000);
                            uniqueKeyFlag = true;
                            resetVP(clientLoginStatus);
                            break;
                        }
                    }
                }
                */
                break;
            }
            else {
                loggedIn = false;
                String LoginStatus = "Login Failed";
                String LoginStatusBytes = toHexString(LoginStatus);
                delay(100);
                writeString(adminLoginStatus, LoginStatusBytes);
            }

            delay(10);
            sendWriteCommand(LOGIN, RESET);
            clientLogin = false;
            adminLogin = false;
        }
        delay(50);
    }
    APIresponseFlag = false;
    if (printDebug) Serial.println("configureLogin completed");
}

void configuredeviceTask(void *parameter) {
    if (printDebug) Serial.println("configuredeviceTask Started");

    for(;;) {
        DynamicJsonDocument doc(1024);
        // if (printDebug) Serial.println("In Device Configuration");
        delay(100);

        String APIresponse = "";

        if (APIresponse == "" && !APIresponseFlag) {
            // Send http get request to check devices are avaiable
            APIresponse = getResponse(getOrgDetailsUrl);
            if (printDebug) Serial.println(APIresponse);
        }

        if (APIresponse == "SUCCESSFUL") {
            vTaskResume(xHandledatetime); // Resume date time task

            APIresponseFlag = true;
            APIresponse = "";
            pageSwitch(COMPANY_DETAIL_AUTO_LOAD);
            if (printDebug) Serial.println("COMPANY_DETAIL_AUTO_LOAD");
            delay(5);

            while (true) {
                // sendReadCommand(VP_AUTO_UPLOAD_COMPANY_DETAILS, 0x1);
                delay(100);
                String autoLoad = tempreadResponse();
                // unwanted lcd response
                if (autoLoad == "5aa53824f4b") autoLoad = "";
                if (autoLoad != "") {
                    if (printDebug) Serial.println("Details auto load: " + autoLoad);
                }

                // Autoload details
                if (containsPattern(autoLoad, companyDoneButtonYes)) {
                    if (printDebug) Serial.println("Will Work with Database API");
                    
                    String Status = "Please Wait";
                    showMessage(clientLoginStatus, Status);
                    String discardMessage = tempreadResponse();
                    delay(1000);

                    // Autoload getOrgDetails
                    String jsonOutput_getOrgDetails = ReturnJson(getOrgDetailsUrl, doc);
                    if (printDebug) {
                        Serial.println("Json Output");
                        Serial.println(jsonOutput_getOrgDetails);
                    }
                    companyName = doc["DB_DATA"]["org_name"].as<String>();
                    companyAddress = doc["DB_DATA"]["address"].as<String>();
                    String companyNameBytes = toHexString(companyName);
                    String companyAddressBytes = toHexString(companyAddress);
                    delay(100);
                    writeString(VP_COMPANY_NAME, companyNameBytes);
                    delay(10);
                    writeString(VP_COMPANY_ADDRESS, companyAddressBytes);
                    delay(10);

                    // Autoload deviceDetails
                    String jsonOutput_deviceDetails = ReturnJson(devicesDetailsUrl, doc);
                    if (printDebug) {
                        // Serial.println("Json Output jsonOutput_deviceDetails");
                        // Serial.println(jsonOutput_deviceDetails);
                    }
                    manufacturerName = doc["DB_DATA"][0]["mfr_name"].as<String>();
                    manufacturerContact = doc["DB_DATA"][0]["mfr_contact"].as<String>();
                    manufacturerEmail = doc["DB_DATA"][0]["mfr_email"].as<String>();
                    String manufacturerNameBytes = toHexString(manufacturerName);
                    String manufacturerContactBytes = toHexString(manufacturerContact);
                    String manufacturerEmailBytes = toHexString(manufacturerEmail);
                    delay(100);
                    writeString(VP_MANUFACTURE_NAME, manufacturerNameBytes);
                    delay(10);
                    writeString(VP_MANUFACTURE_CONTACT, manufacturerContactBytes);
                    delay(10);
                    writeString(VP_MANUFACTURE_EMAIL, manufacturerEmailBytes);
                    delay(10);

                    // Autoload unitdetails
                    locationOfUnit = doc["DB_DATA"][0]["device_location"].as<String>();
                    String locationOfUnitBytes = toHexString(locationOfUnit);
                    delay(100);
                    writeString(VP_LOCATION_OF_UNIT, locationOfUnitBytes);
                    delay(10);

                    // // get device key
                    // stringDeviceKey = doc["DB_DATA"][0]["device_key"].as<String>();
                    // if (printDebug) Serial.println(stringDeviceKey);
                    // delay(10);

                    resetVP(clientLoginStatus);

                    // Manually add remainig enteries
                    pageSwitch(COMPANY_DETAILS_PAGE1);
                    if (printDebug) Serial.println("COMPANY_DETAILS_PAGE1");
                    delay(100);
                    companyDetails();
                    ConfigureDeviceFlag = true;
                    break;
                }

                // Manually enter details
                else if (containsPattern(autoLoad, companyDoneButtonNo)) {
                    pageSwitch(COMPANY_DETAILS_PAGE1);
                    if (printDebug) Serial.println("COMPANY_DETAILS_PAGE1");
                    delay(100);
                    companyDetails();
                    ConfigureDeviceFlag = true;
                    break;
                }
            }
        }

        // Manually enter details
        else {
            vTaskResume(xHandledatetime); // Resume date time task

            APIresponseFlag = true;
            APIresponse = "";
            resetVP(COMPANY_DONE_BUTTON_ADDRESS);
            delay(100);

            pageSwitch(COMPANY_DETAILS_PAGE1);
            if (printDebug) Serial.println("COMPANY_DETAILS_PAGE1");
            delay(100);
            companyDetails();
            ConfigureDeviceFlag = true;
        }

        if (ConfigureDeviceFlag) {

            // we got wifi, client login, company, manufacturer, unit, arrow and device key details
            deviceConfiguredFlag = true;
            preferences.putBool("configuration", deviceConfiguredFlag);

            delay(1000); // For device configuration picture
            // Start slideShow
            slideShowFlag = true;
            slideShow();

            if (printDebug) Serial.println("start Wifi Task");
            vTaskResume(xHandlewifi);

            if (printDebug) Serial.println("Start LoRa task");
            vTaskResume(xHandleLoRa); // Resume the next task

            if (printDebug) Serial.println("Start receiving task");
            vTaskResume(xHandleRecmessage); // Resume the next task 

            // if (printDebug) Serial.println("Start button task");
            // vTaskResume(xHandleButton); // Resume the next task

            if (printDebug) Serial.println("Start Home page Tasks");
            vTaskResume(xHandlehomepage); // Resume the next task before exit

            APIresponseFlag = false;

            break;
        }
        delay(50);
    }

    if (printDebug) {
        Serial.println("configuredeviceTask completed");
        Serial.println("configuredeviceTask Deleted");
    }
    // REVISIT save all details befor deleting task
    vTaskDelete(xHandleconfigdevice); // Delete the task 
}

void configuredeviceAgain() { // Not used

    resetVP(COMPANY_DONE_BUTTON_ADDRESS);
    delay(100);

    pageSwitch(COMPANY_DETAILS_PAGE1);
    if (printDebug) Serial.println("COMPANY_DETAILS_PAGE1");
    delay(100);
    companyDetails();

    if (printDebug) Serial.println("Configuredevice Again completed");
}

void companyDetails() {
    if (printDebug) Serial.println("companyDetails Started");

    while (true) {
        delay(100);
        String companyDetails = tempreadResponse();
        if (companyDetails == "5aa53824f4b") companyDetails = "";
        if (companyDetails != "") {
            if (printDebug) Serial.println("Company Details: " + companyDetails);
        }

        if (containsPattern(companyDetails, companyDetails_page1_next)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcess(VP_COMPANY_NAME, "Company Name: ", companyName);
            readAndProcess(VP_COMPANY_ADDRESS, "Company Address: ", companyAddress);
            readAndProcess(VP_KEY_RESPONSIBLE_PERSON_NAME, "Key Responsible Person Name: ", keyResponsiblePersonName);
            readAndProcess(VP_KEY_RESPONSIBLE_PERSON_CONTACT, "Key Responsible Person Contact: ", keyResponsiblePersonContact);
            readAndProcess(VP_KEY_RESPONSIBLE_PERSON1_NAME, "Key Responsible Person1 Name: ", keyResponsiblePerson1Name);
            readAndProcess(VP_KEY_RESPONSIBLE_PERSON1_CONTACT, "Key Responsible Person1 Contact: ", keyResponsiblePerson1Contact);
            
            // add contact number into recepients list here
            recipients[0] = keyResponsiblePersonContact.c_str(); // Update recipient 1
            recipients[1] = keyResponsiblePerson1Contact.c_str(); // Update recipient 2

            if ((companyName == "") || (companyAddress == "") || (keyResponsiblePersonName == "") || (keyResponsiblePersonContact == "") || (keyResponsiblePerson1Name == "") || (keyResponsiblePerson1Contact == "")) {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else {
                resetVP(clientLoginStatus);
                pageSwitch(COMPANY_DETAILS_PAGE2);
                if (printDebug) Serial.println("COMPANY_DETAILS_PAGE2");
                delay(5);
            }
        }

        else if (containsPattern(companyDetails, companyDetails_exit)) {
            resetVP(clientLoginStatus);
            pageSwitch(HOME_PAGE);
            if (printDebug) Serial.println("HOME_PAGE");
            delay(5);
            break;
        }

        else if (containsPattern(companyDetails, companyDetails_page2_next)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcess(VP_KEY_RESPONSIBLE_PERSON2_NAME, "Key Responsible Person2 Name: ", keyResponsiblePerson2Name);
            readAndProcess(VP_KEY_RESPONSIBLE_PERSON2_CONTACT, "Key Responsible Person2 Contact: ", keyResponsiblePerson2Contact);
            readAndProcess(VP_KEY_RESPONSIBLE_PERSON3_NAME, "Key Responsible Person3 Name: ", keyResponsiblePerson3Name);
            readAndProcess(VP_KEY_RESPONSIBLE_PERSON3_CONTACT, "Key Responsible Person3 Contact: ", keyResponsiblePerson3Contact);

            // add contact number into recepients list here
            recipients[2] = keyResponsiblePerson2Contact.c_str(); // Update recipient 3
            recipients[3] = keyResponsiblePerson3Contact.c_str(); // Update recipient 4

            if ((keyResponsiblePerson2Name == "") || (keyResponsiblePerson2Contact == "") || (keyResponsiblePerson3Name == "") || (keyResponsiblePerson3Contact == "")) {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else {
                resetVP(clientLoginStatus);
                pageSwitch(COMPANY_DETAILS_PAGE3);
                if (printDebug) Serial.println("COMPANY_DETAILS_PAGE3");
                delay(5);
            }
        }

        else if (containsPattern(companyDetails, companyDetails_page2_back)) {
            resetVP(clientLoginStatus);
            pageSwitch(COMPANY_DETAILS_PAGE1);
            if (printDebug) Serial.println("COMPANY_DETAILS_PAGE1");
            delay(5);
        }

        else if (containsPattern(companyDetails, companyDetails_page3_next)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcess(VP_KEY_RESPONSIBLE_PERSON4_NAME, "Key Responsible Person4 Name: ", keyResponsiblePerson4Name);
            readAndProcess(VP_KEY_RESPONSIBLE_PERSON4_CONTACT, "Key Responsible Person4 Contact: ", keyResponsiblePerson4Contact);
            readAndProcess(VP_LOCAL_FIRE_DEPARTMENT_NAME, "Local Fire Department Name: ", localFireDepartmentName);
            readAndProcess(VP_LOCAL_FIRE_DEPARTMENT_CONTACT, "Local Fire Department Contact: ", localFireDepartmentContact);
            
            // add contact number into recepients list here
            recipients[4] = keyResponsiblePerson4Contact.c_str(); // Update recipient 5

            if ((keyResponsiblePerson4Name == "") || (keyResponsiblePerson4Contact == "") || (localFireDepartmentName == "") || (localFireDepartmentContact == "")) {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else {
                resetVP(clientLoginStatus);
                companyDetailsFlag = true;
            }
        }

        else if (containsPattern(companyDetails, companyDetails_page3_back)) {
            pageSwitch(COMPANY_DETAILS_PAGE2);
            if (printDebug) Serial.println("COMPANY_DETAILS_PAGE2");
            delay(5);
        }

        else if (containsPattern(companyDetails, companyDetails_page3_skip)) {
            resetVP(clientLoginStatus);
            companyDetailsFlag = true;
        }

        if (companyDetailsFlag)
            break;
    }
    if (companyDetailsFlag) {
        companyDetailsFlag = false;
        pageSwitch(COMPANY_MANUFACTURE_DETAILS);
        if (printDebug) Serial.println("COMPANY_MANUFACTURE_DETAILS");
        delay(5);
        manufactureDetails();
    }

    if (printDebug) Serial.println("companyDetails completed");
}

void manufactureDetails() {
    if (printDebug) Serial.println("manufactureDetails Started");

    while (true) {
        delay(100);
        String manufacturerDetails = tempreadResponse();
        if (manufacturerDetails == "5aa53824f4b") manufacturerDetails = "";
        if (manufacturerDetails != "") {
            if (printDebug) Serial.println("Manufacturing Details :" + manufacturerDetails);
        }

        if (containsPattern(manufacturerDetails, Manufacturing_Details_Upload)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcess(VP_MANUFACTURE_NAME, "Manufacturer Name: ", manufacturerName);
            readAndProcess(VP_MANUFACTURE_CONTACT, "Manufacturer Contact: ", manufacturerContact);
            readAndProcess(VP_MANUFACTURE_EMAIL, "Manufacturer Email: ", manufacturerEmail);
            readAndProcess(VP_MANUFACTURE_DATE, "Manufacturer Date: ", dateOfManufacture);
            readAndProcess(VP_MANUFACTURE_SERIAL_N0, "Manufacturer Serial No: ", serialNumber);

            if ((manufacturerName == "") || (manufacturerContact == "") || (manufacturerEmail == "") || (dateOfManufacture == "") || (serialNumber == "")) {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else {
                resetVP(clientLoginStatus);
                companyManufacturerDetails = true;
            }
        }

        if (companyManufacturerDetails)
            break;

        else if (containsPattern(manufacturerDetails, Manufacturing_Details_Back)) {
            companyManufacturerDetailsBack = true;
            pageSwitch(COMPANY_DETAILS_PAGE3);
            if (printDebug) Serial.println("COMPANY_DETAILS_PAGE3");
            delay(5);
            break;
        }
    }

    if (companyManufacturerDetailsBack) {
        companyManufacturerDetailsBack = false;
        companyDetails();
    }

    else if (companyManufacturerDetails) {
        companyManufacturerDetails = false;
        pageSwitch(COMPANY_UNIT_DETAILS);
        if (printDebug) Serial.println("COMPANY_UNIT_DETAILS");
        delay(5);
        unitDetails();
    }
    if (printDebug) Serial.println("manufactureDetails completed");
}

void unitDetails() {
    if (printDebug) Serial.println("unitDetails Started");

    while (true) {
        // resetVP(VP_UNIT_DONE);
        // delay(100);
        // sendReadCommand(VP_UNIT_DONE, 0x1);
        delay(100);
        String unitDetails = tempreadResponse();
        if (unitDetails == "5aa53824f4b") unitDetails = "";
        if (unitDetails != "") {
            if (printDebug) Serial.println("Unit Details :" + unitDetails);
        }

        if (containsPattern(unitDetails, Unit_Details_Upload)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            delay(100);
            sendReadCommand(VP_LOCATION_OF_UNIT, 0x28);
            delay(100);
            String templocationOfUnit = tempreadResponse();
            delay(100);
            String luremoverHeaders = removeFirst7Bytes(templocationOfUnit);
            String lucextractedData = extractDataBeforeMarker(luremoverHeaders, "ffff");
            locationOfUnit = hexToString(lucextractedData);
            if (printDebug) Serial.println("Location of Unit: " + locationOfUnit);

            delay(100);
            sendReadCommand(VP_ASSIGNED_UNIT_NUMBER, 0x28);
            delay(100);
            String tempassignedUnitNumber = tempreadResponse();
            delay(100);
            String auremoverHeaders = removeFirst7Bytes(tempassignedUnitNumber);
            String aucextractedData = extractDataBeforeMarker(auremoverHeaders, "ffff");
            assignedUnitNumber = hexToString(aucextractedData);
            if (printDebug) Serial.println("Assigned Unit Number: " + assignedUnitNumber);

            delay(100);
            sendReadCommand(VP_DATE_OF_UNIT_INSTALLATION, 0x28);
            delay(100);
            String tempdateOfUnitInstallation = tempreadResponse();
            delay(100);
            String duiremoverHeaders = removeFirst7Bytes(tempdateOfUnitInstallation);
            String duicextractedData = extractDataBeforeMarker(duiremoverHeaders, "ffff");
            dateOfUnitInstallation = hexToString(duicextractedData);
            if (printDebug) Serial.println("Unit Installation Date: " + dateOfUnitInstallation);

            delay(100);
            sendReadCommand(VP_UNIT_INSTALLER, 0x28);
            delay(100);
            String tempunitInstaller = tempreadResponse();
            delay(100);
            String uiremoverHeaders = removeFirst7Bytes(tempunitInstaller);
            String uicextractedData = extractDataBeforeMarker(uiremoverHeaders, "ffff");
            unitInstaller = hexToString(uicextractedData);
            if (printDebug) Serial.println("Unit Installer: " + unitInstaller);

            delay(100);
            sendReadCommand(VP_UNIT_CONTACT_DETAILS, 0x28);
            delay(100);
            String tempunitContactDetails = tempreadResponse();
            delay(100);
            String ucdremoverHeaders = removeFirst7Bytes(tempunitContactDetails);
            String ucdcextractedData = extractDataBeforeMarker(ucdremoverHeaders, "ffff");
            unitContactDetails = hexToString(ucdcextractedData);
            if (printDebug) Serial.println("Unit contact Details: " + unitContactDetails);

            delay(100);
            sendReadCommand(VP_UNIT_IP_ADDRESS, 0x28);
            delay(100);
            String tempipAddress = tempreadResponse();
            delay(100);
            String ipremoverHeaders = removeFirst7Bytes(tempipAddress);
            String ipdcextractedData = extractDataBeforeMarker(ipremoverHeaders, "ffff");
            ipAddress = hexToString(ipdcextractedData);
            if (printDebug) Serial.println("Unit IP Address: " + ipAddress);

            if ((locationOfUnit == "") || (assignedUnitNumber == "") || (dateOfUnitInstallation == "") || (unitInstaller == "") || (unitContactDetails == "") || (ipAddress == "")) {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else {
                resetVP(clientLoginStatus);
                UnitDetailsFlag = true;
            }
        }

        if (UnitDetailsFlag)
            break;

        else if (containsPattern(unitDetails, Unit_Details_Back)) {
            unitDetailsBack = true;
            pageSwitch(COMPANY_MANUFACTURE_DETAILS);
            if (printDebug) Serial.println("COMPANY_MANUFACTURE_DETAILS");
            delay(5);
            break;
        }
    }

    if (unitDetailsBack) {
        unitDetailsBack = false;
        manufactureDetails();
    }

    else if (UnitDetailsFlag) {
        UnitDetailsFlag = false;
        pageSwitch(DEVICE_DIRECTION_DETAILS_PAGE);
        if (printDebug) Serial.println("DEVICE_DIRECTION_DETAILS_PAGE");
        delay(5);
        devicesDirectionDetails();
    }
    if (printDebug) Serial.println("unitDetails completed");
}

void devicesDirectionDetails() {
    if (printDebug) Serial.println("devicesDirectionDetails Started");
    resetVP(VP_DEVICE_DRIVER_RETURN_KEY);
    delay(100);
    sendReadCommand(VP_DEVICE_DRIVER_RETURN_KEY, 0x1);
    while (true) {
        String arrowIndication = tempreadResponse();
        if (arrowIndication == "5aa53824f4b") arrowIndication = "";
        if (arrowIndication != "") {
            if (printDebug) Serial.println("Arrow Details :" + arrowIndication);
        }

        if (containsPattern(arrowIndication, Left_Arrow_Indication)) {  
            if (printDebug) Serial.println("Left Side Move");
            arrowFlags = true;
            arrowDirection = false; // False for Left direction
        }

        else if (containsPattern(arrowIndication, Right_Arrow_Indication)) {
            if (printDebug) Serial.println("Right Side Move");
            arrowFlags = true;
            arrowDirection = true; // True for Right direction
        }

        else if (containsPattern(arrowIndication, Arrow_Details_Back)) {
            ArrowDetailsBack = true;
            pageSwitch(COMPANY_UNIT_DETAILS);
            if (printDebug) Serial.println("COMPANY_UNIT_DETAILS");
            delay(5);
            break;
        }
        if (arrowFlags)  break;
        
        delay(50);
    }

    if (ArrowDetailsBack) {
        ArrowDetailsBack = false;
        unitDetails();
    }

    else if (arrowFlags) {
        arrowFlags = false;
        pageSwitch(DEVICE_CONFIGURED_SUCCESSFULLY);
        if (printDebug) Serial.println("DEVICE_CONFIGURED_SUCCESSFULLY");
    }
    if (printDebug) Serial.println("devicesDirectionDetails completed");
}

// void NodeIdDetails() {
//     if (printDebug) Serial.println("NodeIdDetails Started");
//     bool nodeIdFlag = false;
//     while(true) {

//         if (nodeIdFlag) {
            
//             break;
//         }
//         delay(50);
//     }

//     if (printDebug) Serial.println("NodeIdDetails completed");
// }

void slideShow() {
    if (printDebug) Serial.println("slideShow Started");

    while (slideShowFlag) {
        vTaskSuspend(xHandledatetime); // Suspend date time task while slideShow

        if(activatedByLoRa) {
            slideShowFlag = false;
            vTaskResume(xHandledatetime); // date time task
            break;
        }
        
        String ack = tempreadResponse();

        // pageSwitch(FYREBOXLOGO);
        pageSwitch(NEW_LOGO); // Client side
        if (printDebug) Serial.println("FYREBOXLOGO");
        // Record the start time
        unsigned long startTime = millis();
        // Check if 5 seconds have passed
        while (millis() - startTime < 5000) {
            if(activatedByLoRa) {
            slideShowFlag = false;
            vTaskResume(xHandledatetime); // date time task
            break;
        }
            ack = tempreadResponse();

            if (ack == "5aa53824f4b") ack = "";
            if (ack != "") {
                if (printDebug) Serial.print("Acknowledgment from LCD: " + ack);
            }

            if (checkLastFourDigitsMatch(ack, Home_Screen)) {
                resetVP(CLIENT_PASSWORD2);
                resetVP(CLIENT_PASSWORD_DISPLAY2);
                resetVP(clientLoginStatus);
                pageSwitch(PASSWORD_PAGE);
                if (printDebug) Serial.println("PASSWORD_PAGE");
                
                while(true) { // Wait for user to enter password
                    ack = tempreadResponse();

                    // Okay button
                    if (containsPattern(ack, "31ca")) {
                        if (printDebug) Serial.println("In Okay button");

                        // Read Password
                        String temppassword = processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                        if(temppassword == predefinedPassword) {
                            pageSwitch(HOME_PAGE);
                            if (printDebug) Serial.println("HOME_PAGE");
                            delay(5);
                            slideShowFlag = false;
                            vTaskResume(xHandledatetime); // Resume if touch is detected
                            break;
                        }
                        else {
                            String LoginStatus = "Wrong Password";
                            if (printDebug) Serial.println(LoginStatus);
                            String LoginStatusBytes = toHexString(LoginStatus);
                            delay(100);
                            writeString(clientLoginStatus, LoginStatusBytes);
                            delay(1000);
                            resetVP(clientLoginStatus);
                        }
                    }

                    // show password
                    else if (containsPattern(ack, "ffff")) {
                        resetVP(CLIENT_PASSWORD_DISPLAY2);
                        delay(100);
                        String vpAddress = processFourthAndFifthBytes(ack);
                        if (printDebug) Serial.println("Vp Address :" + vpAddress);
                        if (vpAddress == "51e4") {  // case sensitive
                            if (printDebug) Serial.println("Client Panel");
                            delay(100);
                            startCheckingPassword(CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON, ack);
                        }
                    }

                    // Password show/hide icon
                    else if (containsPattern(ack, "6231")) {
                        if (printDebug) Serial.println("In show/hide icon");
                        delay(100);
                        processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                    }

                    // Back button
                    else if (containsPattern(ack, Report_Home_Screen)) {
                        pageSwitch(FYREBOXLOGO);
                        if (printDebug) Serial.println("FYREBOXLOGO");
                        delay(5);
                        break;
                    }
                    delay(50);
                }
            }
            delay(100);
        }
        if (!slideShowFlag) {
            break;
        }

        if (NODEID == 1) {
            pageSwitch(CLIENT_LOGO_SUN); 
            if (printDebug) Serial.println("CLIENT_LOGO_SUN");
        }
        else if (NODEID == 2) {
            pageSwitch(CLIENT_LOGO_SERVEST);
            if (printDebug) Serial.println("CLIENT_LOGO_SERVEST");
        }
        else if (NODEID == 3) {
            pageSwitch(CLIENT_LOGO_BROLL);
            if (printDebug) Serial.println("CLIENT_LOGO_BROLL");
        }

        // Record the start time
        startTime = millis();
        // Check if 10 seconds have passed
        while (millis() - startTime < 10000) {
            if(activatedByLoRa) {
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // date time task
                break;
            }
            ack = tempreadResponse();

            if (ack == "5aa53824f4b")  ack = "";

            if (ack != "") {
                if (printDebug) Serial.println("Acknowledgment from LCD: " + ack);
            }

            if (checkLastFourDigitsMatch(ack, Home_Screen)) {
                resetVP(CLIENT_PASSWORD2);
                resetVP(CLIENT_PASSWORD_DISPLAY2);
                resetVP(clientLoginStatus);
                pageSwitch(PASSWORD_PAGE);
                if (printDebug) Serial.println("PASSWORD_PAGE");
                while(true) { // Wait for user to enter password
                    ack = tempreadResponse();

                    // Okay button
                    if (containsPattern(ack, "31ca")) {
                        if (printDebug) Serial.println("In Okay button");

                        // Read Password
                        String temppassword = processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                        if(temppassword == predefinedPassword) {
                            pageSwitch(HOME_PAGE);
                            if (printDebug) Serial.println("HOME_PAGE");
                            delay(5);
                            slideShowFlag = false;
                            vTaskResume(xHandledatetime); // Resume if touch is detected
                            break;
                        }
                        else {
                            String LoginStatus = "Wrong Password";
                            if (printDebug) Serial.println(LoginStatus);
                            String LoginStatusBytes = toHexString(LoginStatus);
                            delay(100);
                            writeString(clientLoginStatus, LoginStatusBytes);
                            delay(1000);
                            resetVP(clientLoginStatus);
                        }
                    }

                    // show password
                    else if (containsPattern(ack, "ffff")) {
                        resetVP(CLIENT_PASSWORD_DISPLAY2);
                        delay(100);
                        String vpAddress = processFourthAndFifthBytes(ack);
                        if (printDebug) Serial.println("Vp Address :" + vpAddress);
                        if (vpAddress == "51e4") {  // case sensitive
                            if (printDebug) Serial.println("Client Panel");
                            delay(100);
                            startCheckingPassword(CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON, ack);
                        }
                    }

                    // Password show/hide icon
                    else if (containsPattern(ack, "6231")) {
                        if (printDebug) Serial.println("In show/hide icon");
                        delay(100);
                        processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                    }

                    // Back button
                    else if (containsPattern(ack, Report_Home_Screen)) {
                        if (NODEID == 1) pageSwitch(CLIENT_LOGO_SUN); 
                        else if (NODEID == 2) pageSwitch(CLIENT_LOGO_SERVEST);
                        else if (NODEID == 3) pageSwitch(CLIENT_LOGO_BROLL);
                        delay(5);
                        break;
                    }
                    delay(50);
                }
            }
            delay(100);
        }
        if (!slideShowFlag) {
            break;
        }

        pageSwitch(LOCALMAP_PAGE); // ground floor
        if (printDebug) Serial.println("LOCALMAP_PAGE");

        // Record the start time
        startTime = millis();
        // Check if 15 seconds have passed
        while (millis() - startTime < 15000) {
            if(activatedByLoRa) {
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // date time task
                break;
            }
            ack = tempreadResponse();

            if (ack == "5aa53824f4b") ack = "";

            if (ack != "") {
                if (printDebug) Serial.println("Acknowledgment from LCD: " + ack);
            }

            if (checkLastFourDigitsMatch(ack, Home_Screen)) {
                resetVP(CLIENT_PASSWORD2);
                resetVP(CLIENT_PASSWORD_DISPLAY2);
                resetVP(clientLoginStatus);
                pageSwitch(PASSWORD_PAGE);
                if (printDebug) Serial.println("PASSWORD_PAGE");
                while(true) { // Wait for user to enter password
                    ack = tempreadResponse();

                    // Okay button
                    if (containsPattern(ack, "31ca")) {
                        if (printDebug) Serial.println("In Okay button");

                        // Read Password
                        String temppassword = processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                        if(temppassword == predefinedPassword) {
                            pageSwitch(HOME_PAGE);
                            if (printDebug) Serial.println("HOME_PAGE");
                            delay(5);
                            slideShowFlag = false;
                            vTaskResume(xHandledatetime); // Resume if touch is detected
                            break;
                        }
                        else {
                            String LoginStatus = "Wrong Password";
                            if (printDebug) Serial.println(LoginStatus);
                            String LoginStatusBytes = toHexString(LoginStatus);
                            delay(100);
                            writeString(clientLoginStatus, LoginStatusBytes);
                            delay(1000);
                            resetVP(clientLoginStatus);
                        }
                    }

                    // show password
                    else if (containsPattern(ack, "ffff")) {
                        resetVP(CLIENT_PASSWORD_DISPLAY2);
                        delay(100);
                        String vpAddress = processFourthAndFifthBytes(ack);
                        if (printDebug) Serial.println("Vp Address :" + vpAddress);
                        if (vpAddress == "51e4") {  // case sensitive
                            if (printDebug) Serial.println("Client Panel");
                            delay(100);
                            startCheckingPassword(CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON, ack);
                        }
                    }

                    // Password show/hide icon
                    else if (containsPattern(ack, "6231")) {
                        if (printDebug) Serial.println("In show/hide icon");
                        delay(100);
                        processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                    }

                    // Back button
                    else if (containsPattern(ack, Report_Home_Screen)) {
                        pageSwitch(LOCALMAP_PAGE); // ground floor
                        delay(5);
                        break;
                    }
                    delay(50);
                }
            }
            delay(100);
        }
        if (!slideShowFlag) {
            break;
        }

        pageSwitch(EVACUATION_PROCEDURE_PAGE);
        if (printDebug) Serial.println("EVACUATION_PROCEDURE_PAGE");

        // Record the start time
        startTime = millis();
        // Check if 15 seconds have passed
        while (millis() - startTime < 15000) {
            if(activatedByLoRa) {
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // date time task
                break;
            }
            ack = tempreadResponse();

            if (ack == "5aa53824f4b") ack = "";

            if (ack != "") {
                if (printDebug) Serial.println("Acknowledgment from LCD: " + ack);
            }

            if (checkLastFourDigitsMatch(ack, Home_Screen)) {
                resetVP(CLIENT_PASSWORD2);
                resetVP(CLIENT_PASSWORD_DISPLAY2);
                resetVP(clientLoginStatus);
                pageSwitch(PASSWORD_PAGE);
                if (printDebug) Serial.println("PASSWORD_PAGE");
                while(true) { // Wait for user to enter password
                    ack = tempreadResponse();

                    // Okay button
                    if (containsPattern(ack, "31ca")) {
                        if (printDebug) Serial.println("In Okay button");

                        // Read Password
                        String temppassword = processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                        if(temppassword == predefinedPassword) {
                            pageSwitch(HOME_PAGE);
                            if (printDebug) Serial.println("HOME_PAGE");
                            delay(5);
                            slideShowFlag = false;
                            vTaskResume(xHandledatetime); // Resume if touch is detected
                            break;
                        }
                        else {
                            String LoginStatus = "Wrong Password";
                            if (printDebug) Serial.println(LoginStatus);
                            String LoginStatusBytes = toHexString(LoginStatus);
                            delay(100);
                            writeString(clientLoginStatus, LoginStatusBytes);
                            delay(1000);
                            resetVP(clientLoginStatus);
                        }
                    }

                    // show password
                    else if (containsPattern(ack, "ffff")) {
                        resetVP(CLIENT_PASSWORD_DISPLAY2);
                        delay(100);
                        String vpAddress = processFourthAndFifthBytes(ack);
                        if (printDebug) Serial.println("Vp Address :" + vpAddress);
                        if (vpAddress == "51e4") {  // case sensitive
                            if (printDebug) Serial.println("Client Panel");
                            delay(100);
                            startCheckingPassword(CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON, ack);
                        }
                    }

                    // Password show/hide icon
                    else if (containsPattern(ack, "6231")) {
                        if (printDebug) Serial.println("In show/hide icon");
                        delay(100);
                        processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                    }

                    // Back button
                    else if (containsPattern(ack, Report_Home_Screen)) {
                        pageSwitch(EVACUATION_PROCEDURE_PAGE);
                        if (printDebug) Serial.println("EVACUATION_PROCEDURE_PAGE");
                        delay(5);
                        break;
                    }
                    delay(50);
                }
            }
            delay(100);
        }
        if (!slideShowFlag) {
            break;
        }
    }
    if (printDebug) Serial.println("slideShow completed");
}

void slideShow_EvacuationDiagrams() {
    if (printDebug) Serial.println("slideShow_EvacuationDiagrams Started");

    while (slideShowFlag) {
        vTaskSuspend(xHandledatetime); // Suspend date time task while slideShow

        String ack = tempreadResponse();

        pageSwitch(SITEMAP_PAGE); // first floor
        if (printDebug) Serial.println("SITEMAP_PAGE");
        // Record the start time
        unsigned long startTime = millis();
        // Check if 7 seconds have passed
        while (millis() - startTime < 7000) {
            ack = tempreadResponse();

            if (ack == "5aa53824f4b") ack = "";
            if (ack != "") {
                if (printDebug) Serial.print("Acknowledgment from LCD: "+ ack);
            }

            if (checkLastFourDigitsMatch(ack, Home_Screen)) {
                if (evacuationActivefromLoRa) {
                    pageSwitch(HOME_PAGE);
                    if (printDebug) Serial.println("HOME_PAGE");
                    delay(5);
                    while(true) {
                        ack = tempreadResponse();
                        if (containsPattern(ack, "6218") && containsPattern(ack, "101")) {
                            // send message to other nodes to stop site evacuation
                            sendDeactivationMessage();
                            delay(5);
                            sendDeactivationMessage();

                            if (printDebug) Serial.println("deactivate site evacuation");
                            evacuationActivefromLCD = false;
                            evacuationActivefromLoRa = false;
                            activatedByLoRa = false;

                            // send message to other nodes to stop site evacuation
                            sendDeactivationMessage();
                            delay(5);
                            sendDeactivationMessage();

                            activateRGBflag = false;
                            activateSoundflag = false;

                            if (printDebug) Serial.println("deactivate site evacuation done");
                            slideShowFlag = false;
                            break;
                        }
                        delay(100);
                    }
                }
                else {
                    pageSwitch(HOME_PAGE);
                    if (printDebug) Serial.println("HOME_PAGE");
                    delay(5);
                    slideShowFlag = false;
                    vTaskResume(xHandledatetime); // Resume if touch is detected
                    break;
                }
            }

            delay(50);

            if (!activatedByLoRa && evacuationActivefromLoRa) {
                Display_AC_DEAC_Icon(0x01); // 01 show activate button
                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                activateRGBflag = false;
                activateSoundflag = false;
                delay(100);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
            else if (deactivate && evacuationActivefromLCD) {
                deactivate = false;
                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                activateRGBflag = false;
                activateSoundflag = false;
                Display_AC_DEAC_Icon(0x01); // 01 show site evacuation button
                delay(100);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
        }

        if (!slideShowFlag) {
            vTaskResume(xHandleLoRa); // node discovery
            break;
        }

        pageSwitch(HAND_GESTURES_PAGE);
        if (printDebug) Serial.println("HAND_GESTURES_PAGE");
        // Record the start time
        startTime = millis();
        // Check if 8 seconds have passed
        while (millis() - startTime < 8000) {
            ack = tempreadResponse();

            if (ack == "5aa53824f4b") ack = "";
            if (ack != "") {
                if (printDebug) Serial.print("Acknowledgment from LCD: "+ ack);
            }

            if (checkLastFourDigitsMatch(ack, Home_Screen)) {
                if (evacuationActivefromLoRa) {
                    pageSwitch(HOME_PAGE);
                    if (printDebug) Serial.println("HOME_PAGE");
                    delay(5);
                    while(true) {
                        ack = tempreadResponse();
                        if (containsPattern(ack, "6218") && containsPattern(ack, "101")) {
                            // send message to other nodes to stop site evacuation
                            sendDeactivationMessage();
                            delay(5);
                            sendDeactivationMessage();

                            if (printDebug) Serial.println("deactivate site evacuation");
                            evacuationActivefromLCD = false;
                            evacuationActivefromLoRa = false;
                            activatedByLoRa = false;

                            // send message to other nodes to stop site evacuation
                            sendDeactivationMessage();
                            delay(5);
                            sendDeactivationMessage();

                            activateRGBflag = false;
                            activateSoundflag = false;

                            if (printDebug) Serial.println("deactivate site evacuation done");
                            slideShowFlag = false;
                            break;
                        }
                        delay(100);
                    }
                }
                else {
                    pageSwitch(HOME_PAGE);
                    if (printDebug) Serial.println("HOME_PAGE");
                    delay(5);
                    slideShowFlag = false;
                    vTaskResume(xHandledatetime); // Resume if touch is detected
                    break;
                }
            }

            delay(50);

            if (!activatedByLoRa && evacuationActivefromLoRa) {
                Display_AC_DEAC_Icon(0x01); // 01 show activate button
                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                activateRGBflag = false;
                activateSoundflag = false;
                delay(100);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
            else if (deactivate && evacuationActivefromLCD) {
                deactivate = false;
                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                activateRGBflag = false;
                activateSoundflag = false;
                Display_AC_DEAC_Icon(0x01); // 01 show site evacuation button
                delay(100);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
        }

        if (!slideShowFlag) {
            vTaskResume(xHandleLoRa); // node discovery
            break;
        }

        pageSwitch(EVACUATION_PROCEDURE_PAGE);
        if (printDebug) Serial.println("EVACUATION_PROCEDURE_PAGE");

        // Record the start time
        startTime = millis();
        // Check if 8 seconds have passed
        while (millis() - startTime < 8000) {
            ack = tempreadResponse();

            if (ack == "5aa53824f4b") ack = "";
            if (ack != "") {
                if (printDebug) Serial.print("Acknowledgment from LCD: "+ ack);
            }

            if (checkLastFourDigitsMatch(ack, Home_Screen)) {
                if (evacuationActivefromLoRa) {
                    pageSwitch(HOME_PAGE);
                    if (printDebug) Serial.println("HOME_PAGE");
                    delay(5);
                    while(true) {
                        ack = tempreadResponse();
                        if (containsPattern(ack, "6218") && containsPattern(ack, "101")) {
                            // send message to other nodes to stop site evacuation
                            sendDeactivationMessage();
                            delay(5);
                            sendDeactivationMessage();

                            if (printDebug) Serial.println("deactivate site evacuation");
                            evacuationActivefromLCD = false;
                            evacuationActivefromLoRa = false;
                            activatedByLoRa = false;

                            // send message to other nodes to stop site evacuation
                            sendDeactivationMessage();
                            delay(5);
                            sendDeactivationMessage();

                            activateRGBflag = false;
                            activateSoundflag = false;

                            if (printDebug) Serial.println("deactivate site evacuation done");
                            slideShowFlag = false;
                            break;
                        }
                        delay(100);
                    }
                }
                else {
                    pageSwitch(HOME_PAGE);
                    if (printDebug) Serial.println("HOME_PAGE");
                    delay(5);
                    slideShowFlag = false;
                    vTaskResume(xHandledatetime); // Resume if touch is detected
                    break;
                }
            }

            delay(50);

            if (!activatedByLoRa && evacuationActivefromLoRa) {
                Display_AC_DEAC_Icon(0x01); // 01 show activate button
                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                activateRGBflag = false;
                activateSoundflag = false;
                delay(100);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
            else if (deactivate && evacuationActivefromLCD) {
                deactivate = false;
                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                activateRGBflag = false;
                activateSoundflag = false;
                Display_AC_DEAC_Icon(0x01); // 01 show site evacuation button
                delay(100);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
        }

        if (!slideShowFlag) {
            vTaskResume(xHandleLoRa); // node discovery
            break;
        }
    }
    if (printDebug) Serial.println("slideShow_EvacuationDiagrams completed");
}

void slideShow_EvacuationDiagrams_forButton() {
    if (printDebug) Serial.println("slideShow_EvacuationDiagrams_forButton Started");
    vTaskSuspend(xHandleLoRa); // node discovery
    vTaskSuspend(xHandlehomepage);
    vTaskSuspend(xHandleRecmessage);

    while (slideShowFlag) {
        vTaskSuspend(xHandledatetime); // Suspend date time task while slideShow

        int val = digitalRead(siteEvacuation_buttonPin);
        if (printDebug) {
            Serial.print("val : ");
            Serial.println(val);
        }

        pageSwitch(SITEMAP_PAGE); // first floor
        if (printDebug) Serial.println("SITEMAP_PAGE");
        // Record the start time
        unsigned long startTime = millis();
        // Check if 7 seconds have passed
        while (millis() - startTime < 7000) {
            val = digitalRead(siteEvacuation_buttonPin);
            if (val == HIGH) {
                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                delay(5);

                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                activateRGBflag = false;
                activateSoundflag = false;

                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
        }
        if (!slideShowFlag) {
            // send message to other nodes to stop site evacuation
            sendDeactivationMessage();

            vTaskResume(xHandleLoRa); // node discovery
            vTaskResume(xHandlehomepage);
            vTaskResume(xHandleRecmessage);
            break;
        }

        pageSwitch(HAND_GESTURES_PAGE);
        if (printDebug) Serial.println("HAND_GESTURES_PAGE");
        // Record the start time
        startTime = millis();
        // Check if 8 seconds have passed
        while (millis() - startTime < 8000) {
            val = digitalRead(siteEvacuation_buttonPin);
            if (val == HIGH) {
                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                delay(5);

                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                activateRGBflag = false;
                activateSoundflag = false;

                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
        }
        if (!slideShowFlag) {
            // send message to other nodes to stop site evacuation
            sendDeactivationMessage();

            vTaskResume(xHandleLoRa); // node discovery
            vTaskResume(xHandlehomepage);
            vTaskResume(xHandleRecmessage);
            break;
        }

        pageSwitch(EVACUATION_PROCEDURE_PAGE);
        if (printDebug) Serial.println("EVACUATION_PROCEDURE_PAGE");

        // Record the start time
        startTime = millis();
        // Check if 15 seconds have passed
        while (millis() - startTime < 15000) {
            val = digitalRead(siteEvacuation_buttonPin);
            if (val == HIGH) {
                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                delay(5);

                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                activateRGBflag = false;
                activateSoundflag = false;

                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();

                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
        }
        if (!slideShowFlag) {
            // send message to other nodes to stop site evacuation
            sendDeactivationMessage();

            vTaskResume(xHandleLoRa); // node discovery
            vTaskResume(xHandlehomepage);
            vTaskResume(xHandleRecmessage);
            break;
        }
    }
    if (printDebug) Serial.println("slideShow_EvacuationDiagrams_forButton completed");
}

void homepageTasks(void *parameter) {
    if (printDebug) Serial.println("homepageTasks Started");
    for(;;) {
        // Reset the last activity time
        // lastActivityTime = millis();

        checkData = tempreadResponse();

        // unwanted lcd response
        if (checkData == "5aa53824f4b") checkData = "";
        if (checkData != "") {
            if (printDebug) Serial.println("Data in homepageTasks:" + checkData);
        }
        delay(50);

        // Show/Hide Menu & select between Menu functions
        if (containsPattern(checkData, "6211")) {
            if (printDebug) Serial.println("Data from home screen menu");

            // Home screen to menu screen
            if (containsPattern(checkData, Home_Screen_Menu)) {
                pageSwitch(MENU_PAGE);
                if (printDebug) Serial.println("MENU_PAGE");
                delay(5);
            }

            // Menu screen to home screen
            else if (containsPattern(checkData, Menu_Home_Screen)) {
                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                delay(5);
            }

            // Upload PDF
            else if (containsPattern(checkData, uploadPDF)) {
                String LoginStatus = "Currently not available";
                if (printDebug) Serial.println(LoginStatus);
                String LoginStatusBytes = toHexString(LoginStatus);
                delay(100);
                writeString(clientLoginStatus, LoginStatusBytes);
                delay(1000);
                resetVP(clientLoginStatus);
            }

            // Manufacturer Details
            else if (containsPattern(checkData, ManufacturerDetails)) {
                resetVP(CLIENT_PASSWORD2);
                resetVP(CLIENT_PASSWORD_DISPLAY2);
                resetVP(clientLoginStatus);
                pageSwitch(PASSWORD_PAGE);
                if (printDebug) Serial.println("PASSWORD_PAGE");
                vTaskSuspend(xHandledatetime);
                
                while(true) { // Wait for user to enter password
                    String ack = tempreadResponse();

                    // Okay button
                    if (containsPattern(ack, "31ca")) {
                        if (printDebug) Serial.println("In Okay button");

                        // Read Password
                        String temppassword = processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                        if(temppassword == predefinedManufacturerdetailsPassword) {
                            vTaskResume(xHandledatetime); // Resume date time now

                            pageSwitch(COMPANY_MANUFACTURE_DETAILS); 
                            delay(100);
                            if (printDebug) Serial.println("manufactureDetails Started");

                            while (true) {
                                delay(100);
                                String manufacturerDetails = tempreadResponse();
                                if (manufacturerDetails == "5aa53824f4b") manufacturerDetails = "";
                                if (manufacturerDetails != "") {
                                    if (printDebug) Serial.println("Manufacturing Details :" + manufacturerDetails);
                                }

                                if (containsPattern(manufacturerDetails, Manufacturing_Details_Upload)) {
                                    vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

                                    String Status = "Please Wait";
                                    showMessage(clientLoginStatus, Status);
                                    String discardMessage = tempreadResponse();
                                    delay(1000);

                                    readAndProcess(VP_MANUFACTURE_NAME, "Manufacturer Name: ", manufacturerName);
                                    readAndProcess(VP_MANUFACTURE_CONTACT, "Manufacturer Contact: ", manufacturerContact);
                                    readAndProcess(VP_MANUFACTURE_EMAIL, "Manufacturer Email: ", manufacturerEmail);
                                    readAndProcess(VP_MANUFACTURE_DATE, "Manufacturer Date: ", dateOfManufacture);
                                    readAndProcess(VP_MANUFACTURE_SERIAL_N0, "Manufacturer Serial No: ", serialNumber);

                                    if ((manufacturerName == "") || (manufacturerContact == "") || (manufacturerEmail == "") || (dateOfManufacture == "") || (serialNumber == "")) {
                                        String Status = "Please Enter Details";
                                        showMessage(clientLoginStatus, Status);
                                        delay(1000);
                                        resetVP(clientLoginStatus);
                                        vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                                    }
                                    else {
                                        resetVP(clientLoginStatus);
                                        companyManufacturerDetails = true;
                                    }
                                }

                                if (companyManufacturerDetails) {
                                    companyManufacturerDetails = false;
                                    pageSwitch(MENU_PAGE);
                                    if (printDebug) Serial.println("MENU_PAGE");
                                    delay(5);
                                    break;
                                }
                                else if (containsPattern(manufacturerDetails, Manufacturing_Details_Back)) {
                                    companyManufacturerDetailsBack = true;
                                    pageSwitch(MENU_PAGE);
                                    if (printDebug) Serial.println("MENU_PAGE");
                                    delay(5);
                                    break;  
                                }
                            }
                            if (printDebug) Serial.println("manufactureDetails completed");

                            break;
                        }
                        else {
                            String LoginStatus = "Wrong Password";
                            if (printDebug) Serial.println(LoginStatus);
                            String LoginStatusBytes = toHexString(LoginStatus);
                            delay(100);
                            writeString(clientLoginStatus, LoginStatusBytes);
                            delay(1000);
                            resetVP(clientLoginStatus);
                        }
                    }

                    // show password
                    else if (containsPattern(ack, "ffff")) {
                        resetVP(CLIENT_PASSWORD_DISPLAY2);
                        delay(100);
                        String vpAddress = processFourthAndFifthBytes(ack);
                        if (printDebug) Serial.println("Vp Address :" + vpAddress);
                        if (vpAddress == "51e4") {  // case sensitive
                            if (printDebug) Serial.println("Client Panel");
                            delay(100);
                            startCheckingPassword(CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON, ack);
                        }
                    }

                    // Password show/hide icon
                    else if (containsPattern(ack, "6231")) {
                        if (printDebug) Serial.println("In show/hide icon");
                        delay(100);
                        processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                    }

                    // Back button
                    else if (containsPattern(ack, Report_Home_Screen)) {
                        pageSwitch(MENU_PAGE);
                        if (printDebug) Serial.println("MENU_PAGE");
                        delay(5);
                        break;
                    }
                    delay(50);
                }
            } 

            // Company Details
            else if (containsPattern(checkData, CompanyDetails)) {
                resetVP(CLIENT_PASSWORD2);
                resetVP(CLIENT_PASSWORD_DISPLAY2);
                resetVP(clientLoginStatus);
                pageSwitch(PASSWORD_PAGE);
                if (printDebug) Serial.println("PASSWORD_PAGE");
                vTaskSuspend(xHandledatetime);
                
                while(true) { // Wait for user to enter password
                    String ack = tempreadResponse();

                    // Okay button
                    if (containsPattern(ack, "31ca")) {
                        if (printDebug) Serial.println("In Okay button");

                        // Read Password
                        String temppassword = processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                        if(temppassword == predefinedCompanydetailsPassword) {
                            vTaskResume(xHandledatetime); // Resume date time now

                            pageSwitch(COMPANY_DETAILS_PAGE1);
                            delay(100);
                            if (printDebug) Serial.println("companyDetails Started");

                            while (true) {
                                delay(100);
                                String companyDetails = tempreadResponse();
                                if (companyDetails == "5aa53824f4b") companyDetails = "";
                                if (companyDetails != "") {
                                    if (printDebug) Serial.println("Company Details: " + companyDetails);
                                }

                                if (containsPattern(companyDetails, companyDetails_page1_next)) {
                                    vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

                                    String Status = "Please Wait";
                                    showMessage(clientLoginStatus, Status);
                                    String discardMessage = tempreadResponse();
                                    delay(1000);

                                    readAndProcess(VP_COMPANY_NAME, "Company Name: ", companyName);
                                    readAndProcess(VP_COMPANY_ADDRESS, "Company Address: ", companyAddress);
                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON_NAME, "Key Responsible Person Name: ", keyResponsiblePersonName);
                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON_CONTACT, "Key Responsible Person Contact: ", keyResponsiblePersonContact);
                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON1_NAME, "Key Responsible Person1 Name: ", keyResponsiblePerson1Name);
                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON1_CONTACT, "Key Responsible Person1 Contact: ", keyResponsiblePerson1Contact);
                                    
                                    // add contact number into recepients list here
                                    recipients[0] = keyResponsiblePersonContact.c_str(); // Update recipient 1
                                    recipients[1] = keyResponsiblePerson1Contact.c_str(); // Update recipient 2

                                    if ((companyName == "") || (companyAddress == "") || (keyResponsiblePersonName == "") || (keyResponsiblePersonContact == "") || (keyResponsiblePerson1Name == "") || (keyResponsiblePerson1Contact == "")) {
                                        String Status = "Please Enter Details";
                                        showMessage(clientLoginStatus, Status);
                                        delay(1000);
                                        resetVP(clientLoginStatus);

                                        vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                                    }
                                    else {
                                        resetVP(clientLoginStatus);
                                        pageSwitch(COMPANY_DETAILS_PAGE2);
                                        if (printDebug) Serial.println("COMPANY_DETAILS_PAGE2");
                                        delay(5);
                                    }
                                }

                                else if (containsPattern(companyDetails, companyDetails_exit)) {
                                    resetVP(clientLoginStatus);
                                    pageSwitch(MENU_PAGE);
                                    if (printDebug) Serial.println("MENU_PAGE");
                                    delay(5);
                                    break;
                                }

                                else if (containsPattern(companyDetails, companyDetails_page2_next)) {
                                    vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

                                    String Status = "Please Wait";
                                    showMessage(clientLoginStatus, Status);
                                    String discardMessage = tempreadResponse();
                                    delay(1000);

                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON2_NAME, "Key Responsible Person2 Name: ", keyResponsiblePerson2Name);
                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON2_CONTACT, "Key Responsible Person2 Contact: ", keyResponsiblePerson2Contact);
                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON3_NAME, "Key Responsible Person3 Name: ", keyResponsiblePerson3Name);
                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON3_CONTACT, "Key Responsible Person3 Contact: ", keyResponsiblePerson3Contact);

                                    // add contact number into recepients list here
                                    recipients[2] = keyResponsiblePerson2Contact.c_str(); // Update recipient 3
                                    recipients[3] = keyResponsiblePerson3Contact.c_str(); // Update recipient 4

                                    if ((keyResponsiblePerson2Name == "") || (keyResponsiblePerson2Contact == "") || (keyResponsiblePerson3Name == "") || (keyResponsiblePerson3Contact == "")) {
                                        String Status = "Please Enter Details";
                                        showMessage(clientLoginStatus, Status);
                                        delay(1000);
                                        resetVP(clientLoginStatus);

                                        vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                                    }
                                    else {
                                        resetVP(clientLoginStatus);
                                        pageSwitch(COMPANY_DETAILS_PAGE3);
                                        if (printDebug) Serial.println("COMPANY_DETAILS_PAGE3");
                                        delay(5);
                                    }
                                }

                                else if (containsPattern(companyDetails, companyDetails_page2_back)) {
                                    resetVP(clientLoginStatus);
                                    pageSwitch(COMPANY_DETAILS_PAGE1);
                                    if (printDebug) Serial.println("COMPANY_DETAILS_PAGE1");
                                    delay(5);
                                }

                                else if (containsPattern(companyDetails, companyDetails_page3_next)) {
                                    vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

                                    String Status = "Please Wait";
                                    showMessage(clientLoginStatus, Status);
                                    String discardMessage = tempreadResponse();
                                    delay(1000);

                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON4_NAME, "Key Responsible Person4 Name: ", keyResponsiblePerson4Name);
                                    readAndProcess(VP_KEY_RESPONSIBLE_PERSON4_CONTACT, "Key Responsible Person4 Contact: ", keyResponsiblePerson4Contact);
                                    readAndProcess(VP_LOCAL_FIRE_DEPARTMENT_NAME, "Local Fire Department Name: ", localFireDepartmentName);
                                    readAndProcess(VP_LOCAL_FIRE_DEPARTMENT_CONTACT, "Local Fire Department Contact: ", localFireDepartmentContact);
                                    
                                    // add contact number into recepients list here
                                    recipients[4] = keyResponsiblePerson4Contact.c_str(); // Update recipient 5

                                    if ((keyResponsiblePerson4Name == "") || (keyResponsiblePerson4Contact == "") || (localFireDepartmentName == "") || (localFireDepartmentContact == "")) {
                                        String Status = "Please Enter Details";
                                        showMessage(clientLoginStatus, Status);
                                        delay(1000);
                                        resetVP(clientLoginStatus);

                                        vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                                    }
                                    else {
                                        resetVP(clientLoginStatus);
                                        companyDetailsFlag = true;
                                    }
                                }

                                else if (containsPattern(companyDetails, companyDetails_page3_back)) {
                                    pageSwitch(COMPANY_DETAILS_PAGE2);
                                    if (printDebug) Serial.println("COMPANY_DETAILS_PAGE2");
                                    delay(5);
                                }

                                else if (containsPattern(companyDetails, companyDetails_page3_skip)) {
                                    resetVP(clientLoginStatus);
                                    companyDetailsFlag = true;
                                }

                                if (companyDetailsFlag)
                                    break;
                            }
                            if (companyDetailsFlag) {
                                companyDetailsFlag = false;
                                pageSwitch(MENU_PAGE);
                                if (printDebug) Serial.println("MENU_PAGE");
                                delay(5);
                            }
                            if (printDebug) Serial.println("companyDetails completed");

                            break;
                        }
                        else {
                            String LoginStatus = "Wrong Password";
                            if (printDebug) Serial.println(LoginStatus);
                            String LoginStatusBytes = toHexString(LoginStatus);
                            delay(100);
                            writeString(clientLoginStatus, LoginStatusBytes);
                            delay(1000);
                            resetVP(clientLoginStatus);
                        }
                    }

                    // show password
                    else if (containsPattern(ack, "ffff")) {
                        resetVP(CLIENT_PASSWORD_DISPLAY2);
                        delay(100);
                        String vpAddress = processFourthAndFifthBytes(ack);
                        if (printDebug) Serial.println("Vp Address :" + vpAddress);
                        if (vpAddress == "51e4") {  // case sensitive
                            if (printDebug) Serial.println("Client Panel");
                            delay(100);
                            startCheckingPassword(CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON, ack);
                        }
                    }

                    // Password show/hide icon
                    else if (containsPattern(ack, "6231")) {
                        if (printDebug) Serial.println("In show/hide icon");
                        delay(100);
                        processPasswordDisplay(CLIENT_PASSWORD2, CLIENT_PASSWORD_DISPLAY2, PASSWORD_ICON);
                    }

                    // Back button
                    else if (containsPattern(ack, Report_Home_Screen)) {
                        pageSwitch(MENU_PAGE);
                        if (printDebug) Serial.println("MENU_PAGE");
                        delay(5);
                        break;
                    }
                    delay(50);
                }
            } 

            // Battery calculations
            else if (containsPattern(checkData, batteryCalculations)) {
                pageSwitch(BATTERY_CALC_PAGE);
                if (printDebug) Serial.println("BATTERY_CALC_PAGE");
                delay(5);
                batterysaverflag = true; // only for testing
            }

            // Installation Procedure
            else if (containsPattern(checkData, installationProcedure)) {
                if (printDebug) Serial.println("Installation Procedure");
                pageSwitch(INSTALLATION_PROCEDURE_PAGE);
                delay(100);
            } 

            // Maintenance Procedure
            else if (containsPattern(checkData, maintenanceProcedure)) {
                if (printDebug) Serial.println("Maintenance Procedure");
                pageSwitch(MAINTENANCE_PROCEDURE_PAGE);
                delay(100);
            } 

            // Settings
            else if (containsPattern(checkData, settings)) {
                if (printDebug) Serial.println("SETTINGS_PAGE");
                pageSwitch(SETTINGS_PAGE);
                delay(100);
                batterysaverflag = false; // only for testing
                while (true) {
                    delay(100);
                    String settingsData = tempreadResponse();
                    if (settingsData == "5aa53824f4b") settingsData = "";
                    if (settingsData != "") {
                        if (printDebug) Serial.println("settingsData: " + settingsData);
                    }
                    if (containsPattern(settingsData, "6211")) {
                        if (printDebug) Serial.println("MENU_PAGE");
                        pageSwitch(MENU_PAGE);
                        break;
                    }

                    // Change Device Password
                    else if (containsPattern(settingsData, "1101")) {
                        if (printDebug) Serial.println("CHANGE_PASSWORD_PAGE");
                        resetVP(VP_CURRENT_PASSWORD);
                        resetVP(VP_NEW_PASSWORD);
                        resetVP(VP_CONFIRM_NEW_PASSWORD);
                        pageSwitch(CHANGE_PASSWORD_PAGE);
                        while(true) {
                            checkData = tempreadResponse();
                            // Okay button
                            if (containsPattern(checkData, "31ca")) {
                                if (printDebug) Serial.println("In Okay button");
                                vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data
                                vTaskSuspend(xHandleLoRa); // node discovery
                                // vTaskSuspend(xHandleButton);
                                vTaskSuspend(xHandleRecmessage);

                                String Status = "Please Wait";
                                showMessage(clientLoginStatus, Status);
                                String discardMessage = tempreadResponse();
                                delay(1000);

                                readAndProcess(VP_CURRENT_PASSWORD, "Current Password: ", currentPassword);
                                readAndProcess(VP_NEW_PASSWORD, "New Password: ", newPassword);
                                readAndProcess(VP_CONFIRM_NEW_PASSWORD, "Confirm New Password: ", confirmPassword);

                                if (currentPassword == "") {
                                    String Status = "Please enter password";
                                    showMessage(clientLoginStatus, Status);
                                    delay(1000);
                                    resetVP(clientLoginStatus);
                                }
                                else if (currentPassword != predefinedPassword) {
                                    String Status = "Wrong Password";
                                    showMessage(clientLoginStatus, Status);
                                    delay(1000);
                                    resetVP(clientLoginStatus);
                                }
                                else if (newPassword == "") {
                                    String Status = "Please Enter New Password";
                                    showMessage(clientLoginStatus, Status);
                                    delay(1000);
                                    resetVP(clientLoginStatus);
                                }
                                else if (confirmPassword == "") {
                                    String Status = "Please Confirm New Password";
                                    showMessage(clientLoginStatus, Status);
                                    delay(1000);
                                    resetVP(clientLoginStatus);
                                }
                                else if (confirmPassword != newPassword) {
                                    String Status = "Password Mismatch";
                                    showMessage(clientLoginStatus, Status);
                                    delay(1000);
                                    resetVP(clientLoginStatus);
                                }
                                else {
                                    predefinedPassword = newPassword;
                                    String Status = "Password Changed";
                                    showMessage(clientLoginStatus, Status);
                                    delay(1000);
                                    resetVP(clientLoginStatus);
                                    pageSwitch(SETTINGS_PAGE);
                                    if (printDebug) Serial.println("SETTINGS_PAGE");
                                    delay(5);

                                    vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                                    vTaskResume(xHandleLoRa); // node discovery
                                    // vTaskResume(xHandleButton);
                                    vTaskResume(xHandleRecmessage);
                                    break;
                                }
                                
                            }
                            else if (containsPattern(checkData, "6211")) {
                                pageSwitch(SETTINGS_PAGE);
                                if (printDebug) Serial.println("SETTINGS_PAGE");
                                delay(5);
                                break;
                            }
                            delay(50);
                        }
                    }

                    // Change WiFi Credentials
                    else if (containsPattern(settingsData, "1102")) {
                        if (printDebug) Serial.println("INTERNETPAGE");
                        pageSwitch(INTERNETPAGE);
                        vTaskSuspend(xHandledatetime);
                        vTaskSuspend(xHandleLoRa);
                        configureInternet();
                        vTaskResume(xHandledatetime);
                        vTaskResume(xHandleLoRa);
                        resetVP(clientLoginStatus);
                        pageSwitch(SETTINGS_PAGE);
                        if (printDebug) Serial.println("SETTINGS_PAGE");
                    }

                    // Change Arrow Direction
                    else if (containsPattern(settingsData, "1104")) {
                        if (printDebug) Serial.println("DEVICE_DIRECTION_DETAILS_PAGE");
                        pageSwitch(DEVICE_DIRECTION_DETAILS_PAGE);

                        if (printDebug) Serial.println("devicesDirectionDetails Started");
                        resetVP(VP_DEVICE_DRIVER_RETURN_KEY);
                        delay(100);
                        sendReadCommand(VP_DEVICE_DRIVER_RETURN_KEY, 0x1);
                        while (true) {
                            String arrowIndication = tempreadResponse();
                            if (arrowIndication == "5aa53824f4b") arrowIndication = "";
                            if (arrowIndication != "") {
                                if (printDebug) Serial.println("Arrow Details :" + arrowIndication);
                            }

                            if (containsPattern(arrowIndication, Left_Arrow_Indication)) {  
                                if (printDebug) Serial.println("Left Side Move");
                                arrowFlags = true;
                                arrowDirection = false; // False for Left direction
                            }

                            else if (containsPattern(arrowIndication, Right_Arrow_Indication)) {
                                if (printDebug) Serial.println("Right Side Move");
                                arrowFlags = true;
                                arrowDirection = true; // True for Right direction
                            }

                            else if (containsPattern(arrowIndication, Arrow_Details_Back)) {
                                ArrowDetailsBack = true;
                                delay(5);
                                break;
                            }
                            if (arrowFlags) {
                              break;  
                            } 
                            delay(50);
                        }

                        if (ArrowDetailsBack) {
                            ArrowDetailsBack = false;
                            pageSwitch(SETTINGS_PAGE);
                            if (printDebug) Serial.println("SETTINGS_PAGE");
                        }

                        else if (arrowFlags) {
                            arrowFlags = false;
                            pageSwitch(SETTINGS_PAGE);
                            if (printDebug) Serial.println("SETTINGS_PAGE");
                        }
                        if (printDebug) Serial.println("devicesDirectionDetails completed");
                    }

                    // Factory reset
                    else if (containsPattern(settingsData, "1105")) {
                        if (printDebug) Serial.println("FACTORY_RESET_PAGE");
                        pageSwitch(FACTORY_RESET_PAGE);
                        while(true) {
                            delay(100);
                            String factoryResetData = tempreadResponse();
                            if (factoryResetData == "5aa53824f4b") factoryResetData = "";
                            if (factoryResetData != "") {
                                if (printDebug) Serial.println("factoryResetData: " + factoryResetData);
                            }
                            if (containsPattern(factoryResetData, "6211")) { // No
                                if (printDebug) Serial.println("SETTINGS_PAGE");
                                pageSwitch(SETTINGS_PAGE);
                                break;
                            }
                            else if (containsPattern(factoryResetData, "31ca")) { // Yes

                                deviceConfiguredFlag = false;
                                preferences.putBool("configuration", deviceConfiguredFlag);

                                // remove credentials
                                removeInternetCredentials();
                                removeClientCredentials();
                                removeAdminCredentials();

                                // reset_allText();
                                systemReset(); // reset LCD

                                ESP.restart();
                            }
                        }
                    } 
                } 
            }

            // Log a fault
            else if (containsPattern(checkData, logFault)) {
                if (printDebug) Serial.println("Log a Fault");
                resetVP(VP_FAULT_LOGGED_BY);
                resetVP(VP_FAULT_DETAILS);
                pageSwitch(LOGAFAULT_PAGE);
                if (printDebug) Serial.println("LOGAFAULT_PAGE");
                delay(5);
                while(true) {
                    checkData = tempreadResponse();
                    // Okay button
                    if (containsPattern(checkData, "31ca")) {
                        if (printDebug) Serial.println("In Okay button");
                        vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data
                        vTaskSuspend(xHandleLoRa); // node discovery
                        // vTaskSuspend(xHandleButton);
                        vTaskSuspend(xHandleRecmessage);

                        String Status = "Please Wait";
                        showMessage(clientLoginStatus, Status);
                        String discardMessage = tempreadResponse();
                        delay(1000);

                        readAndProcess(VP_FAULT_LOGGED_BY, "Fault logged by: ", FAULT_LOGGED_BY);
                        readAndProcess(VP_FAULT_DETAILS, "Fault details: ", FAULT_DETAILS);

                        if (FAULT_LOGGED_BY == "") {
                            String Status = "Please Enter Your Name";
                            showMessage(clientLoginStatus, Status);
                            delay(1000);
                            resetVP(clientLoginStatus);
                        }
                        else if (FAULT_DETAILS == "") {
                            String Status = "Please Enter Details";
                            showMessage(clientLoginStatus, Status);
                            delay(1000);
                            resetVP(clientLoginStatus);
                        }
                        else {
                            String Status = "Fault logged";
                            showMessage(clientLoginStatus, Status);
                            delay(1000);
                            resetVP(clientLoginStatus);
                            pageSwitch(HOME_PAGE);
                            if (printDebug) Serial.println("HOME_PAGE");
                            delay(5);

                            // send concatinated message which contains
                            // FAULT_LOGGED_BY + DATE + TIME + UNIT_LOCATION + FAULT_DETAILS
                            // String faultMessage = FAULT_LOGGED_BY + dateString + timeString + UNIT_LOCATION + FAULT_DETAILS;

                            String faultMessage = FAULT_DETAILS + " this fault was logged by " + FAULT_LOGGED_BY 
                            + " at " + timeString + " on " + dateString + " from " + " Fyrebox Hub.";

                            // To send fault SMS to 5 users
                            xTaskCreate(sendNotificationSMS, 
                                        "sendNotificationSMS", 
                                        8192,
                                        (void*) faultMessage.c_str(), // Parameter to pass
                                        1, 
                                        &xHandlesms);
                            delay(100);

                            vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                            vTaskResume(xHandleLoRa); // node discovery
                            // vTaskResume(xHandleButton);
                            vTaskResume(xHandleRecmessage);
                            break;
                        }
                        
                    }

                    else if (containsPattern(checkData, "6211")) {
                        pageSwitch(MENU_PAGE);
                        if (printDebug) Serial.println("MENU_PAGE");
                        delay(5);
                        break;
                    }

                    delay(50);
                }
            }

            // Handle Logout
            else if (containsPattern(checkData, logout)) {
                if(wifiConnectedFlag) {
                    adminLogin = false;
                    clientLogin = false;

                    // remove credentials
                    removeClientCredentials();
                    removeAdminCredentials();

                    resetVP(ADMIN_SSID);
                    resetVP(ADMIN_PASSWORD);
                    resetVP(ADMIN_PASSWORD_DISPLAY);
                    resetVP(adminLoginStatus);

                    resetVP(CLIENT_SSID);
                    resetVP(CLIENT_PASSWORD);
                    resetVP(CLIENT_PASSWORD_DISPLAY);
                    resetVP(clientLoginStatus);

                    pageSwitch(CLIENTPAGE);
                    if (printDebug) Serial.println("CLIENTPAGE");
                    delay(5);

                    if (printDebug) Serial.println("Suspend date time Task");
                    vTaskSuspend(xHandledatetime); // Not needed while login
                    delay(100);

                    configureLogin();

                    if (printDebug) Serial.println("Resume date time Task");
                    vTaskResume(xHandledatetime); // Login Completed
                    delay(100);

                    pageSwitch(HOME_PAGE);
                    if (printDebug) Serial.println("HOME_PAGE");
                }
                else if (!wifiConnectedFlag) {
                    String LoginStatus = "Not Connected to Internet!";
                    if (printDebug) Serial.println(LoginStatus);
                    String LoginStatusBytes = toHexString(LoginStatus);
                    delay(100);
                    writeString(clientLoginStatus, LoginStatusBytes);
                    delay(1000);
                    resetVP(clientLoginStatus);
                }
            }

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Update date and time
        else if (containsPattern(checkData, "6226")) {
            pageSwitch(UPDATE_DATE_TIME_PAGE);
            if (printDebug) Serial.println("UPDATE_DATE_TIME_PAGE");
            delay(5);
            
            while(true) {
                checkData = tempreadResponse();
                if (containsPattern(checkData, "31ca")) {
                    vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data
                    vTaskSuspend(xHandleLoRa); // node discovery

                    String Status = "Please Wait";
                    showMessage(clientLoginStatus, Status);
                    String discardMessage = tempreadResponse();
                    delay(1000);

                    int hoursValue = readAndConvertHexString(VP_HOURS);
                    int minutesValue = readAndConvertHexString(VP_MINUTES);
                    int secondsValue = readAndConvertHexString(VP_SECONDS);
                    int dateValue = readAndConvertHexString(VP_DATE);
                    int monthValue = readAndConvertHexString(VP_MONTH);
                    int yearValue = readAndConvertHexString(VP_YEAR);
                    yearValue += 2000; 

                    // Validate date and adjust RTC
                    if (monthValue >= 1 && monthValue <= 12 && dateValue >= 1 && dateValue <= daysInMonth(monthValue, yearValue)) {
                        rtc.adjust(DateTime(yearValue, monthValue, dateValue, hoursValue, minutesValue, secondsValue));
                        if (printDebug) Serial.println("RTC adjusted successfully.");
                        resetVP(clientLoginStatus);
                        
                        // get acknowledgment yes or no ?
                        while(true) {
                            pageSwitch(UPDATE_DATE_TIME_YES_NO_PAGE);
                            delay(100);
                            checkData = tempreadResponse();
                            if (containsPattern(checkData, "31ca")) { // yes
                                // send message through LoRa to update date and time on all fyrebox nodes available in a network
                                vTaskResume(xHandledatetime);
                                vTaskResume(xHandleLoRa);
                                delay(100);
                                broadcastDateTime();
                                break;
                            }
                            else if (containsPattern(checkData, "6211")) { // no
                                break;
                            }
                        }

                        // Check the task state before resuming it
                        eTaskState taskState = eTaskGetState(xHandleLoRa);
                        if (taskState == eSuspended) {
                            if (printDebug) Serial.println("node discovery Resumed"); 
                            vTaskResume(xHandleLoRa); // node discovery
                        }
                        delay(50);
                        // Check the task state before resuming it
                        taskState = eTaskGetState(xHandledatetime);
                        if (taskState == eSuspended) {
                            if (printDebug) Serial.println("date time Resumed"); 
                            vTaskResume(xHandledatetime); // node discovery
                        }
                        
                        if (printDebug) Serial.println("HOME_PAGE");
                        pageSwitch(HOME_PAGE);
                        
                        break;
                    } else {
                        Status = "Invalid date";
                        showMessage(clientLoginStatus, Status);
                        String discardMessage = tempreadResponse();
                        delay(1000);
                        resetVP(clientLoginStatus);
                    }                    
                }

                // Exit to home screen
                else if (containsPattern(checkData, "621a")) {
                    pageSwitch(HOME_PAGE);
                    if (printDebug) Serial.println("HOME_PAGE");
                    delay(5);
                    break;
                }
                delay(50);
            }

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // local map button
        else if (containsPattern(checkData, "6212")) {
            pageSwitch(LOCALMAP_PAGE); // ground floor
            if (printDebug) Serial.println("LOCALMAP_PAGE");
            delay(5);

            // Display_DeviceConfigured_Icon(0x05); // only for testing

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Exit local map
        else if (containsPattern(checkData, "6222")) {
            pageSwitch(HOME_PAGE);
            if (printDebug) Serial.println("HOME_PAGE");
            delay(5);

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Site map button
        else if (containsPattern(checkData, "6213")) {
            pageSwitch(SITEMAP_PAGE); // first floor
            if (printDebug) Serial.println("SITEMAP_PAGE");
            delay(5);

            // Display_DeviceConfigured_Icon(0x00); // only for testing

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Exit site map
        else if (containsPattern(checkData, "6220")) {
            pageSwitch(HOME_PAGE);
            if (printDebug) Serial.println("HOME_PAGE");
            delay(5);

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Show Units lists
        else if (containsPattern(checkData, "6214")) {
            pageSwitch(UNITSLISTS_PAGE);
            if (printDebug) Serial.println("UNITSLISTS_PAGE");
            delay(5);

            displayFyreBoxUnitList();

            slideShowFlag = false;
            FyreBoxUnitListFlag = true;

            FyreBoxUnitList();

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Show Report
        else if (containsPattern(checkData, "6215")) {
            if (printDebug) Serial.println("Data from home screen report");

            if (!dataEnteredtoday) {
                resetVP(clientLoginStatus);
                String _message = "Please complete checklist first";
                if (printDebug) Serial.println(_message);
                showMessage(clientLoginStatus, _message);
                delay(1000);
                resetVP(clientLoginStatus);
            }

            else if (containsPattern(checkData, Home_Screen_Report)) {
                pageSwitch(SHOW_REPORT_PAGE);
                if (printDebug) Serial.println("SHOW_REPORT_PAGE");
                delay(5);

                slideShowFlag = false;
                displayIconsFlag = true;

                displayIcons();
            }

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Self Test
        else if (containsPattern(checkData, "6216")) {

            resetVP(clientLoginStatus);
            String selfTest_message = "Self Test";
            if (printDebug) Serial.println(selfTest_message);
            showMessage(clientLoginStatus, selfTest_message);
                        
            audio.connecttoFS(SD, filename2); // SD card file
            while (audio.isRunning()) {
                audio.loop();
                delay(10);
            }
            delay(1000);
            resetVP(clientLoginStatus);

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Show Checklist
        else if (containsPattern(checkData, "6217")) {
            if (printDebug) Serial.println("Data from home screen checklist");

            if (!dataEnteredtoday) {
                lastDataEntryEEPROM = EEPROM.read(EEPROMAddress);
                if (printDebug) {
                    Serial.print("Last data entry week number: ");
                    Serial.println(lastDataEntryEEPROM);
                }

                if (weekByYear == lastDataEntryEEPROM) {
                    dataEnteredtoday = false;
                    if (printDebug) Serial.println("Data Entered today");
                }
                else if (weekByYear > lastDataEntryEEPROM) {
                    weekElapsed = true;
                    if (printDebug) Serial.println("Week Elapsed");
                }
                else {
                    if (printDebug) Serial.println("Next data will be received in next week");
                }

                if (containsPattern(checkData, Home_Screen_Checklist)) {
                    pageSwitch(CHECKLISTPAGE1);
                    if (printDebug) Serial.println("CHECKLISTPAGE1");
                    delay(5);

                    slideShowFlag = false;
                    checkBoxFlag = true;

                    CheckBoxes();
                }
            }

            // Checklist Data is Received
            else if (dataEnteredtoday) {
                resetVP(clientLoginStatus);
                String message = "Data is Received";
                if (printDebug) Serial.println(message);
                showMessage(clientLoginStatus, message);
                delay(2000);
                resetVP(clientLoginStatus);
            }

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // handle site activation
        else if (containsPattern(checkData, "6218") && containsPattern(checkData, "100") || activatedByLoRa) {
                vTaskSuspend(xHandleLoRa); // node discovery 

                if (activatedByLoRa) {
                    evacuationActivefromLoRa = true;
                    if (printDebug) Serial.println("activate site evacuation with LoRa");
                    Display_AC_DEAC_Icon(0x00); // 00 show deactivate button

                    activateRGBflag = true;
                    activateSoundflag = true;
                }
                if (containsPattern(checkData, "100")) {
                    vTaskSuspend(xHandledatetime);  // Suspend date time while processing password
                    resetVP(CLIENT_PASSWORD2);
                    resetVP(CLIENT_PASSWORD_DISPLAY2);
                    resetVP(clientLoginStatus);
                    pageSwitch(SITE_EVACUATION_PAGE);
                    if (printDebug) Serial.println("SITE_EVACUATION_PAGE");
                    while(true) { // Wait for user to ack
                        String ack = tempreadResponse();

                        // unwanted lcd response
                        if (ack == "5aa53824f4b")  ack = "";
                        if (ack != "") Serial.println("Data in site evacuation: " + ack);

                        // Yes button
                        if (containsPattern(ack, "31ca")) {
                            if (printDebug) Serial.println("In yes button");

                            // send message to other nodes to start site evacuation
                            sendActivationMessage();

                            evacuationActivefromLCD = true;
                            if (printDebug) Serial.println("activate site evacuation with LCD");

                            // send message to other nodes to start site evacuation
                            sendActivationMessage();

                            activateRGBflag = true;
                            activateSoundflag = true;

                            // send message to other nodes to start site evacuation
                            sendActivationMessage();

                            vTaskResume(xHandledatetime);   // Resume before exit
                            break;
                        }

                        // No button
                        else if (containsPattern(ack, Report_Home_Screen)) {
                            pageSwitch(HOME_PAGE);
                            if (printDebug) Serial.println("HOME_PAGE");
                            delay(50);
                            Display_AC_DEAC_Icon(0x01); // 01 show activate button
                            vTaskResume(xHandledatetime);   // Resume before exit
                            break;
                        }
                        delay(50);
                    }
                }

                if(evacuationActivefromLoRa || evacuationActivefromLCD) {

                    // To run leds in infinite loop upon activation
                    xTaskCreate(rgbTask, "rgbTask", 10000, NULL, 9, &xHandleRGB);

                    // To play audio and siren bell in infinite loop upon activation
                    xTaskCreate(soundTask, "soundTask", 10000, NULL, 9, &xHandleSound);

                    if (evacuationActivefromLCD) {
                        // To send activation SMS to 5 users
                        xTaskCreate(sendNotificationSMS, 
                        "sendNotificationSMS", 
                        8192,
                        (void*)"ALERT! A Fyrebox unit has been activated. Please assess and follow the emergency procedure once verified.", // Parameter to pass
                        1, 
                        &xHandlesms);
                    }
                    // // only for testing
                    // else {
                    //     // To send battery low SMS to 5 users
                    //     xTaskCreate(sendNotificationSMS, 
                    //     "sendNotificationSMS", 
                    //     8192,
                    //     (void*)"Battery Low! Charge your FyreBox Unit.", // Parameter to pass
                    //     1, 
                    //     &xHandlesms);
                    // }

                    // Start slideShow
                    slideShowFlag = true;
                    slideShow_EvacuationDiagrams();
                }
        }

        // handle site deactivation
        else if (containsPattern(checkData, "6218") && containsPattern(checkData, "101") || (!activatedByLoRa && evacuationActivefromLoRa)) {
            if (evacuationActivefromLCD) {
                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();
                delay(5);
                sendDeactivationMessage();

                if (printDebug) Serial.println("deactivate site evacuation with LCD");
                evacuationActivefromLCD = false;

                // send message to other nodes to stop site evacuation
                sendDeactivationMessage();
                delay(5);
                sendDeactivationMessage();
            }

            if(evacuationActivefromLoRa) {
                if (printDebug) Serial.println("deactivate site evacuation with LoRa");
                evacuationActivefromLoRa = false;
                Display_AC_DEAC_Icon(0x01); // 01 show site evacuation button
            }

            activateRGBflag = false;
            activateSoundflag = false;

            // Check the task state before resuming it
            eTaskState taskState = eTaskGetState(xHandleLoRa);
            if (taskState == eSuspended) {
                if (printDebug) Serial.println("node discovery Resumed"); 
                vTaskResume(xHandleLoRa); // node discovery
            }
            delay(50);

            if (printDebug) Serial.println("deactivate site evacuation done");

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Start Slide show
        else if (containsPattern(checkData, "6219")) {
            if (printDebug) Serial.println("Start slideShow");
            if (containsPattern(checkData, Home_Screen_Back)) {
                // Start slideShow
                slideShowFlag = true;
                slideShow();
            }

            // Reset the last activity time
            lastActivityTime = millis();
        }

        // Check if the idle timeout has elapsed
        else if ((millis() - lastActivityTime >= idleTimeout) && (!activatedByLoRa)) {
            if (printDebug) Serial.println("Idle timeout has elapsed");
            // Start slideShow
            // slideShowFlag = true;
            // slideShow();

            if(batterysaverflag) {
                FastLED.setBrightness(RGB_LED_BRIGHTNESS_BATTERYMODE);
                FastLED.show();
                // Display_Battery_Icon(0x04); // on battery
            }
            else if (!batterysaverflag) {
                FastLED.setBrightness(RGB_LED_BRIGHTNESS_IDEAL);
                FastLED.show();
                // Display_Battery_Icon(0x03); // on charging
            }

            // Reset the last activity time
            lastActivityTime = millis();
        }

        delay(50);
    }

    if (printDebug) {
        Serial.println("homepageTasks completed");
        Serial.println("homepageTasks Suspended");
    }
    vTaskSuspend(xHandlehomepage); // Suspend the task
}

// Function to send a read command, process the response, and store the result
void readAndProcess(int command, const char* description, String& output) {
    sendReadCommand(command, 0x28);
    delay(100);
    String tempResponse = tempreadResponse();
    delay(100);
    String removedHeaders = removeFirst7Bytes(tempResponse);
    String extractedData = extractDataBeforeMarker(removedHeaders, "ffff");
    output = hexToString(extractedData);
    if (printDebug) Serial.println(description + output);
}

// Generalized function to perform the operations to update date and time
int readAndConvertHexString(uint16_t vp_address) {
    sendReadCommand(vp_address, 0x01);
    delay(100);
    String temp = tempreadResponse();
    String removerHeaders = removeFirst6Bytes(temp);
    // Special case handling for VP_YEAR
    // if (vp_address == VP_YEAR) {
    //     removerHeaders = "20" + removerHeaders;
    // }
    int decimalValue = strtol(removerHeaders.c_str(), nullptr, 16);
    return decimalValue;
}

// Function to check if a year is a leap year
bool isLeapYear(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

// Function to get the number of days in a month
int daysInMonth(int month, int year) {
    switch (month) {
        case 1: return 31;
        case 2: return isLeapYear(year) ? 29 : 28;
        case 3: return 31;
        case 4: return 30;
        case 5: return 31;
        case 6: return 30;
        case 7: return 31;
        case 8: return 31;
        case 9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0; // Invalid month
    }
}

void CheckBoxes() {
    if (printDebug) Serial.println("CheckBoxes Started");
    while (checkBoxFlag) {
        delay(100);
        String checkBoxesData = tempreadResponse();

        if (checkBoxesData == "5aa53824f4b") checkBoxesData = "";

        if (checkBoxesData != "") {
            if (printDebug) Serial.println("Data in checkbox: " + checkBoxesData);
        }

        if (checkLastFourDigitsMatch(checkBoxesData, geticonPage1)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcessCheckbox(CONTROL_FUNCTION, "Control Function: ", controlFunction);
            readAndProcessCheckbox(SPEAKER_ACTIVATE, "Speaker Activate: ", speakerActivate);
            readAndProcessCheckbox(FIREMAN_ACTIVATE_BOX, "Fireman Activate Box: ", firemanActivateBox);
            readAndProcessCheckbox(BELL_RING_SYSTEM_ACTIVATION, "Bell Ring System Activation: ", bellRingSystemActivation);
            readAndProcessCheckbox(BATTERY_HEALTH, "Battery Health: ", batteryHealth);
            readAndProcessCheckbox(LED_LIGHT_ON_WHITE, "Led Light ON (White): ", ledLightOnWhite);

            if ((controlFunction != "0") && (speakerActivate != "0") && (firemanActivateBox != "0") && (bellRingSystemActivation != "0") && (batteryHealth != "0") && (ledLightOnWhite != "0")) {
                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                resetVP(clientLoginStatus);
                pageSwitch(CHECKLISTPAGE2);
                if (printDebug) Serial.println("CHECKLISTPAGE2");
                delay(5);
            }
            else {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage1)) {
            resetVP(clientLoginStatus);
            pageSwitch(HOME_PAGE);
            if (printDebug) Serial.println("HOME_PAGE");
            delay(5);
            if (printDebug) Serial.println("Exit command called");
            break;
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, geticonPage2)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcessCheckbox(LED_RED_ACTIVATION, "Led Red Activation: ", ledRedActivation);
            readAndProcessCheckbox(SMS_RECEIVED_FYREBOX_ACTIVATED, "SMS Received: ", smsReceivedFyreboxActivated);
            readAndProcessCheckbox(LCD_SCREEN_WORK, "LCD Screen Work: ", lcdScreenWork);
            readAndProcessCheckbox(SYSTEM_ACTIVATE_WEEKLY_SELF_TEST, "System Activate Weekly Self Test: ", systemActivateWeeklySelfTest);
            readAndProcessCheckbox(EVACUATION_DIAGRAM, "Evacuation Diagram: ", evacuatioDiagram);
            readAndProcessCheckbox(ARROW_WORKING, "Arrow Working: ", arrowWorking);

            if ((ledRedActivation != "0") && (smsReceivedFyreboxActivated != "0") && (lcdScreenWork != "0") && (systemActivateWeeklySelfTest != "0") && (evacuatioDiagram != "0") && (arrowWorking != "0")) {
                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                resetVP(clientLoginStatus);
                pageSwitch(CHECKLISTPAGE3);
                if (printDebug) Serial.println("CHECKLISTPAGE3");
                delay(5);
            }
            else {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage2)) {
            resetVP(clientLoginStatus);
            pageSwitch(CHECKLISTPAGE1);
            if (printDebug) Serial.println("CHECKLISTPAGE1");
            delay(5);
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, geticonPage3)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcessCheckbox(PERMANENT_POWER, "Permanent Power: ", permanentPower);
            readAndProcessCheckbox(ILLUMINATED_SIGNALS_WORKING, "Illuminated Signal Working: ", illuminatedSignalsWorking);
            readAndProcessCheckbox(BATTERIES_REPLACEMENT, "Batteries Replacement: ", batteriesReplacement);
            readAndProcessCheckbox(FLASH_SIGN_PANELS, "Flash Sign Panel: ", flashSignPanel);

            if ((permanentPower != "0") && (illuminatedSignalsWorking != "0") && (batteriesReplacement != "0") && (flashSignPanel != "0")) {
                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                resetVP(clientLoginStatus);
                pageSwitch(CHECKLISTPAGE4);
                if (printDebug) Serial.println("CHECKLISTPAGE4");
                delay(5);
            }
            else {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage3)) {
            resetVP(clientLoginStatus);
            pageSwitch(CHECKLISTPAGE2);
            if (printDebug) Serial.println("CHECKLISTPAGE2");
            delay(5);
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, geticonPage4)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcessCheckbox(UNIT_SECURED, "Unit Secured: ", unitSecured);
            readAndProcessCheckbox(FACIA_COMPONENT_SECURED, "Facia Component Secured: ", faciaComponentSecured);
            readAndProcessCheckbox(EVACUATION_DIAGRAM_UPTODATE, "Evacuation Diagram Up to Date: ", evacuationDiagramUptodate);
            readAndProcessCheckbox(FYREBOX_FREE_OBSTRUCTIONS, "FyreBox Free Obstructions: ", fyreboxFreeObstructions);
            readAndProcessCheckbox(LOGBOOK_UPTODATE, "LogBook Up to Date: ", LogbookUptodate);

            if ((unitSecured != "00") && (faciaComponentSecured != "00") && (evacuationDiagramUptodate != "00") && (fyreboxFreeObstructions != "00") && (LogbookUptodate != "00")) {
                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                resetVP(clientLoginStatus);
                pageSwitch(CHECKLISTPAGE5);
                if (printDebug) Serial.println("CHECKLISTPAGE5");
                delay(5);
            }
            else {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage4)) {
            resetVP(clientLoginStatus);
            pageSwitch(CHECKLISTPAGE3);
            if (printDebug) Serial.println("CHECKLISTPAGE3");
            delay(5);
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, Checklist_Done)) {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            String Status = "Please Wait";
            showMessage(clientLoginStatus, Status);
            String discardMessage = tempreadResponse();
            delay(1000);

            readAndProcessCheckbox(FYREBOX_UNIT_WIPED_CLEAN, "FyreBox Unit Wiped and Clean: ", fyreboxUnitWipedCleaned);
            readAndProcessCheckbox(ANY_DAMAGE_BOX, "Any Damage Box: ", anyDamageBox);
            readAndProcessCheckbox(ANY_RUST_UNIT, "Any Rust Unit: ", anyRustUnit);

            if ((fyreboxUnitWipedCleaned != "00") && (anyDamageBox != "00") && (anyRustUnit != "00")) {
                checkBoxFlag = false;
                if (printDebug) Serial.println("checkBoxFlag is false ");

                if (printDebug) Serial.println("Data entered for this week.");
                dataEnteredtoday = true;
                weekElapsed = false;
                lastDataEntryweekbyYear = weekByYear; // Store the day of the week when data is entered
                delay(100);
                if (printDebug) {
                    Serial.print("last data entry week: ");
                    Serial.println(lastDataEntryweekbyYear);
                }
                EEPROM.write(EEPROMAddress, lastDataEntryweekbyYear);
                EEPROM.commit(); // Commit changes

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
                resetVP(clientLoginStatus);
                pageSwitch(HOME_PAGE);
                if (printDebug) Serial.println("HOME_PAGE");
                delay(5);
                if(wifiConnectedFlag) {
                    String Status = "Sending checklist data to server.";
                    showMessage(clientLoginStatus, Status);

                    devCheck_deviceId = "1";
                    siren = "1";
                    switchDevice = "1";
                    voltIndicator = batteryHealth;
                    ledWhite = ledLightOnWhite;
                    ledRed = ledRedActivation;
                    activationSms = smsReceivedFyreboxActivated;
                    signage = "1";
                    autoTest = systemActivateWeeklySelfTest;
                    testSms = "1";
                    power = permanentPower;
                    voltLevel = "12";
                    battExpiery = batteriesReplacement;
                    deviceInstall = "01-03-24";
                    boxState = unitSecured;
                    diagUpdate = "01-02-24";
                    deviceObstacle = fyreboxFreeObstructions;
                    lastUpdate = "01-02-24";
                    deviceClean = fyreboxUnitWipedCleaned;
                    status = "0";

                    String setDeviceChecklists = manufacturerBaseUrl + "operation=" + devCheck_operation + "&device_id=" + devCheck_deviceId + 
                    "&siren=" + siren + "&switch=" + switchDevice + "&volt_indicator=" + voltIndicator + "&led_white=" + ledWhite + 
                    "&led_red=" + ledRed + "&activation_sms=" + activationSms + "&signage=" + signage + "&auto_test=" + autoTest + 
                    "&test_sms=" + testSms + "&power=" + power + "&volt_level=" + voltLevel + "&batt_expiry=" + battExpiery + 
                    "&devi_install=" + deviceInstall + "&box_state=" + boxState + "&diag_update=" + diagUpdate + 
                    "&devi_obstacle=" + deviceObstacle + "&last_update=" + lastUpdate + "&devi_clean=" + deviceClean + "&status=" + status;

                    HTTPClient http;
                    delay(5);
                    http.begin(setDeviceChecklists); // API URL
                    if (printDebug) Serial.println("URL " + setDeviceChecklists);
                    int httpCode = http.GET();
                    if (printDebug) Serial.println(httpCode);

                    if (httpCode > 0)
                    { // Check for the returning code
                        String payload = http.getString();
                        if (printDebug) Serial.println(payload);
                        Status = "Data Sent to Server.";
                        showMessage(clientLoginStatus, Status);
                        delay(1000);
                        resetVP(clientLoginStatus);
                    }
                    else
                    {
                        if (printDebug) Serial.println("Error on HTTP request");
                    }
                    http.end(); // Free resources

                    resetVP(clientLoginStatus);
                }
                else if (!wifiConnectedFlag) {
                    String Status = "Error sending data to cloud. No Internet";
                    showMessage(clientLoginStatus, Status);
                    delay(1000);
                    resetVP(clientLoginStatus);
                }
                
                /*
                // DynamicJsonDocument doc(1024);

                // Device Checklists Parameters
                // controlFunction + speakerActivate + firemanActivateBox + bellRingSystemActivation + batteryHealth + ledLightOnWhite + ledRedActivation
                // + smsReceivedFyreboxActivated + lcdScreenWork + systemActivateWeeklySelfTest + evacuatioDiagram + arrowWorking + permanentPower + illuminatedSignalsWorking +
                // batteriesReplacement + flashSignPanel + unitSecured + faciaComponentSecured + evacuationDiagramUptodate + fyreboxFreeObstructions
                // + LogbookUptodate + fyreboxUnitWipedCleaned + anyDamageBox + anyRustUnit;

                */
            }
            else {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage5)) {
            resetVP(clientLoginStatus);
            pageSwitch(CHECKLISTPAGE4);
            if (printDebug) Serial.println("CHECKLISTPAGE4");
            delay(5);
        }
    }
    if (printDebug) Serial.println("CheckBoxes completed");
}

// Function to send a read command, process the response, and store the result
void readAndProcessCheckbox(int command, const char* description, String& output) {
    sendReadCommand(command, 0x1);
    delay(100);
    String tempResponse = tempreadResponse();
    delay(100);
    
    // Check for the special case and clear if it matches
    if (tempResponse == "5aa53824f4b") {
        tempResponse = "";
    }

    // Process and store the final result
    output = removeFirst6Bytes(tempResponse);
    if (printDebug) Serial.println(description + output);
    delay(100);
}

void displayIcons() {
    if (printDebug) Serial.println("displayIcons Started");
    while (displayIconsFlag) {
        String LCD_RESPONSE = tempreadResponse();

        if (containsPattern(LCD_RESPONSE, Report_Home_Screen)) {
            displayIconsFlag = false;
            if (printDebug) Serial.println("displayIconsFlag is false ");

            pageSwitch(HOME_PAGE);
            if (printDebug) Serial.println("HOME_PAGE");
            delay(5);
        }

        String keyValue = extractKeycode(LCD_RESPONSE);
        // if (printDebug) Serial.print("Returned key value in string: "); Serial.println(keyValue);

        int keycode = 0;
        keycode = keyValue.toInt();
        // if (printDebug) Serial.print("Returned key value in int: ");Serial.println(keycode);

        byte icon_0 = controlFunction.toInt();
        byte icon_1 = speakerActivate.toInt();
        byte icon_2 = firemanActivateBox.toInt();
        byte icon_3 = bellRingSystemActivation.toInt();
        byte icon_4 = batteryHealth.toInt();
        byte icon_5 = ledLightOnWhite.toInt();
        byte icon_6 = ledRedActivation.toInt();
        byte icon_7 = smsReceivedFyreboxActivated.toInt();
        byte icon_8 = lcdScreenWork.toInt();
        byte icon_9 = systemActivateWeeklySelfTest.toInt();
        byte icon_10 = evacuatioDiagram.toInt();
        byte icon_11 = arrowWorking.toInt();
        byte icon_12 = permanentPower.toInt();
        byte icon_13 = illuminatedSignalsWorking.toInt();
        byte icon_14 = batteriesReplacement.toInt();
        byte icon_15 = flashSignPanel.toInt();
        byte icon_16 = 0x04;
        byte icon_17 = unitSecured.toInt();
        byte icon_18 = faciaComponentSecured.toInt();
        byte icon_19 = evacuationDiagramUptodate.toInt();
        byte icon_20 = fyreboxFreeObstructions.toInt();
        byte icon_21 = LogbookUptodate.toInt();
        byte icon_22 = fyreboxUnitWipedCleaned.toInt();
        byte icon_23 = anyDamageBox.toInt();
        byte icon_24 = anyRustUnit.toInt();

        // VP address pattern to extract vp from LCD_RESPONSE
        String vpaddress_pattern = "60";
        String vp = extractPageVP(LCD_RESPONSE, vpaddress_pattern);
        // if (printDebug) Serial.print("VP address in string: "); Serial.println(vp);
        int vpINT = 0;
        vpINT = vp.toInt();
        // if (printDebug) Serial.print("VP address in int: "); Serial.println(vpINT);

        if (vpINT == showWeek_page1_VP) { // for page 1
            if (keycode == Next_week) {
                resetVP(basicGraphic_page1);
                delay(100);
                // sendIconcommand(basicGraphic_page1, icon_5, icon_4, icon_3, icon_2, icon_1, icon_0);
                delay(5);
            }
            else if (keycode == Prev_week) {
                resetVP(basicGraphic_page1);
                delay(100);
                sendIconcommand(basicGraphic_page1, icon_0, icon_1, icon_2, icon_3, icon_4, icon_5);
                delay(5);
            }
        }

        else if (vpINT == showWeek_page2_VP) {// for page 2
            if (keycode == Next_week)
            {
                resetVP(basicGraphic_page2);
                delay(100);
                // sendIconcommand(basicGraphic_page2, icon_11, icon_10, icon_9, icon_8, icon_7, icon_6);
                delay(5);
            }
            else if (keycode == Prev_week)
            {
                resetVP(basicGraphic_page2);
                delay(100);
                sendIconcommand(basicGraphic_page2, icon_6, icon_7, icon_8, icon_9, icon_10, icon_11);
                delay(5);
            }
        }

        else if (vpINT == showWeek_page3_VP) { // for page 3
            if (keycode == Next_week) {
                resetVP(basicGraphic_page3);
                delay(100);
                // sendIconcommand(basicGraphic_page3, icon_15, icon_14, icon_13, icon_12, 0x04, 0x04);
                delay(5);
            }
            else if (keycode == Prev_week) {
                resetVP(basicGraphic_page3);
                delay(100);
                sendIconcommand(basicGraphic_page3, icon_12, icon_13, icon_14, icon_15, 0x04, 0x04);
                delay(5);
            }
        }

        else if (vpINT == showWeek_page4_VP) { // for page 4
            if (keycode == Next_week) {
                resetVP(basicGraphic_page4);
                delay(100);
                // sendIconcommand(basicGraphic_page4, icon_21, icon_20, icon_19, icon_18, icon_17, icon_16);
                delay(5);
            }
            else if (keycode == Prev_week) {
                resetVP(basicGraphic_page4);
                delay(100);
                sendIconcommand(basicGraphic_page4, icon_16, icon_17, icon_18, icon_19, icon_20, icon_21);
                delay(5);
            }
        }

        else if (vpINT == showWeek_page5_VP) { // for page 5
            if (keycode == Next_week) {
                resetVP(basicGraphic_page5);
                delay(100);
                // sendIconcommand(basicGraphic_page5, icon_24, icon_23, icon_22, 0x04, 0x04, 0x04);
                delay(5);
            }
            else if (keycode == Prev_week) {
                resetVP(basicGraphic_page5);
                delay(100);
                sendIconcommand(basicGraphic_page5, icon_22, icon_23, icon_24, 0x04, 0x04, 0x04);
                delay(5);
            }
        }

        switch (keycode) {
        case 51: // Next_page1
        {
            resetVP(basicGraphic_page2);
            delay(100);
            sendIconcommand(basicGraphic_page2, icon_6, icon_7, icon_8, icon_9, icon_10, icon_11);
            delay(5);
            break;
        }

        case 52: // Next_page2
        {
            resetVP(basicGraphic_page3);
            delay(100);
            sendIconcommand(basicGraphic_page3, icon_12, icon_13, icon_14, icon_15, 0x04, 0x04);
            delay(5);
            break;
        }

        case 53: // Next_page3
        {
            resetVP(basicGraphic_page4);
            delay(100);
            sendIconcommand(basicGraphic_page4, icon_16, icon_17, icon_18, icon_19, icon_20, icon_21);
            delay(5);
            break;
        }

        case 54: // Next_page4
        {
            resetVP(basicGraphic_page5);
            delay(100);
            sendIconcommand(basicGraphic_page5, icon_22, icon_23, icon_24, 0x04, 0x04, 0x04);
            delay(5);
            break;
        }

        case 55: // Next_page5
        {
            resetVP(basicGraphic_page1);
            delay(100);
            sendIconcommand(basicGraphic_page1, icon_0, icon_1, icon_2, icon_3, icon_4, icon_5);
            delay(5);
            break;
        }

        case 61: // Prev_page1
        {
            if (printDebug) Serial.println("No previous pages available");
            break;
        }

        case 62: // Prev_page2
        {
            resetVP(basicGraphic_page1);
            delay(100);
            sendIconcommand(basicGraphic_page1, icon_0, icon_1, icon_2, icon_3, icon_4, icon_5);
            delay(5);
            break;
        }

        case 63: // Prev_page3
        {
            resetVP(basicGraphic_page2);
            delay(100);
            sendIconcommand(basicGraphic_page2, icon_6, icon_7, icon_8, icon_9, icon_10, icon_11);
            delay(5);
            break;
        }

        case 64: // Prev_page4
        {
            resetVP(basicGraphic_page3);
            delay(100);
            sendIconcommand(basicGraphic_page3, icon_12, icon_13, icon_14, icon_15, 0x04, 0x04);
            delay(5);
            break;
        }

        case 65: // Prev_page5
        {
            resetVP(basicGraphic_page4);
            delay(100);
            sendIconcommand(basicGraphic_page4, icon_16, icon_17, icon_18, icon_19, icon_20, icon_21);
            delay(5);
            break;
        }

        default:
            break;
        }
    }
    if (printDebug) Serial.println("displayIcons completed");
}

void saveClientCredentials(const String &username, const String &password) {
    preferences.putString("client_username", username);
    preferences.putString("client_password", password);
}

void removeClientCredentials() {
    preferences.putString("client_username", " ");
    preferences.putString("client_password", " ");
}

void saveAdminCredentials(const String &username, const String &password) {
    preferences.putString("admin_username", username);
    preferences.putString("admin_password", password);
}

void removeAdminCredentials() {
    preferences.putString("admin_username", " ");
    preferences.putString("admin_password", " ");
}

void saveInternetCredentials(const String &ssid, const String &password) {
    preferences.putString("internetSSID", ssid);
    preferences.putString("internetPass", password);
}

void removeInternetCredentials() {
    preferences.putString("internetSSID", "");
    preferences.putString("internetPass", "");
}

bool RememberIcon(uint16_t rememberLogin) {
    delay(100);
    sendReadCommand(rememberLogin, 0x1);
    delay(100);
    String iconRead = tempreadResponse();
    if (printDebug) Serial.println("Read remember Icon:" + iconRead);

    if (checkLast3DigitsMatch(iconRead, "101")) {
        if (printDebug) Serial.println("Remember me is true");
        return true;
    }
    else {
        if (printDebug) Serial.println("Remember me is false");
        return false;
    }
}

void showMessage(uint16_t VP_ADDRESS, String displaymessage) {
    resetVP(VP_ADDRESS);
    if (printDebug) Serial.println(displaymessage);
    String StatusBytes = toHexString(displaymessage);
    delay(100);
    writeString(VP_ADDRESS, StatusBytes);
}

void displayFyreBoxUnitList() {
    // Reset all VPs before updating
    resetVP(Text_Active_Device_1);
    resetVP(Text_Active_Device_2);
    resetVP(Text_Active_Device_3);
    resetVP(Text_Active_Device_4);
    resetVP(Text_Active_Device_5);
    resetVP(Text_Active_Device_6);
    resetVP(Text_Active_Device_7);
    resetVP(Text_Active_Device_8);
    resetVP(Text_Active_Device_9);
    resetVP(Text_Active_Device_10);
    resetVP(Text_Active_Device_11);
    resetVP(Text_Active_Device_12);
    resetVP(Text_Active_Device_13);
    resetVP(Text_Active_Device_14);
    resetVP(Text_Active_Device_15);
    resetVP(Text_Active_Device_16);

    resetVP(Text_Inactive_Device_1);
    resetVP(Text_Inactive_Device_2);
    resetVP(Text_Inactive_Device_3);
    resetVP(Text_Inactive_Device_4);
    resetVP(Text_Inactive_Device_5);
    resetVP(Text_Inactive_Device_6);
    resetVP(Text_Inactive_Device_7);
    resetVP(Text_Inactive_Device_8);
    resetVP(Text_Inactive_Device_9);
    resetVP(Text_Inactive_Device_10);
    resetVP(Text_Inactive_Device_11);
    resetVP(Text_Inactive_Device_12);
    resetVP(Text_Inactive_Device_13);
    resetVP(Text_Inactive_Device_14);
    resetVP(Text_Inactive_Device_15);
    resetVP(Text_Inactive_Device_16);

    resetVP(Text_Units_online);

    delay(100);

    // Units online
    String tempactiveNodesString = String(activeNodes);
    String unitsOnlineBytes = toHexString(tempactiveNodesString);
    delay(10);
    writeString(Text_Units_online, unitsOnlineBytes);

    // Define the sizes of the arrays explicitly
    const size_t Text_Active_Devices_Size = 16;
    const size_t Text_Inactive_Devices_Size = 16;

    // Initialize indices for active and inactive sections
    size_t activeIndex = 0;
    size_t inactiveIndex = 0;

    // Loop through the node statuses and write the corresponding values
    for (const auto &status : nodeStatuses) {
        // Convert nodeId to string and then to hexadecimal
        String nodeIdString = String(status.nodeId);
        String message = "Node ID " + nodeIdString;
        String nodeBytes = toHexString(message);
        delay(10);

        if (status.isActive) {
            // Write to the active device address for this node
            if (activeIndex < Text_Active_Devices_Size) {
                writeString(Text_Active_Devices[activeIndex], nodeBytes);
                activeIndex++;
            }
        }
        else {
            // Write to the inactive device address for this node
            if (inactiveIndex < Text_Inactive_Devices_Size) {
                writeString(Text_Inactive_Devices[inactiveIndex], nodeBytes);
                inactiveIndex++;
            }
        }
    }
}

void FyreBoxUnitList() {
    if (printDebug) Serial.println("FyreBoxUnitsLists started");
    while (FyreBoxUnitListFlag) {
        String LCD_RESPONSE = tempreadResponse();

        if (containsPattern(LCD_RESPONSE, Report_Home_Screen)) {
            FyreBoxUnitListFlag = false;
            if (printDebug) Serial.println("FyreBoxUnitListFlag is false ");

            pageSwitch(HOME_PAGE);
            if (printDebug) Serial.println("HOME_PAGE");
            delay(5);
        }

        else if (containsPattern(LCD_RESPONSE, VP_ReturnKeyCode)) {
            while (true) {
                if (containsPattern(LCD_RESPONSE, ReturnKeyCode_Active_Next)) {
                    if (printDebug) Serial.println("ReturnKeyCode_Active_Next");
                    break;
                }
                else if (containsPattern(LCD_RESPONSE, ReturnKeyCode_Active_Prev)) {
                    if (printDebug) Serial.println("ReturnKeyCode_Active_Prev");
                    break;
                }
                else if (containsPattern(LCD_RESPONSE, ReturnKeyCode_Inactive_Next)) {
                    if (printDebug) Serial.println("ReturnKeyCode_Inactive_Next");
                    break;
                }
                else if (containsPattern(LCD_RESPONSE, ReturnKeyCode_Inactive_Prev)) {
                    if (printDebug) Serial.println("ReturnKeyCode_Inactive_Prev");
                    break;
                }
            }
        }
        delay(100);
    }
    if (printDebug) Serial.println("FyreBoxUnitsLists completed");
}

// For LoRa Mesh
void LoRatask(void *parameter) {
    if (printDebug) Serial.println("LoRatask Started");
    for(;;) {
        static unsigned long lastBroadcastTime = 0;
        static unsigned long lastCheckTime = 0;
        static unsigned long lastStatusPrintTime = 0;
        unsigned long currentMillis = millis();

        if (currentMillis - lastBroadcastTime > 5000) { // Every 5 seconds  
            broadcastPresence();
            lastBroadcastTime = currentMillis;
        }

        // listenForNodes();

        if (currentMillis - lastCheckTime > 10000) { // Every 10 seconds
            checkNodeActivity();
            lastCheckTime = currentMillis;
        }

        if (currentMillis - lastStatusPrintTime > 20000) { // Every 20 seconds
            // printNodeStatuses();  // Print the statuses of all nodes
            printNetworkStats();
            displayFyreBoxUnitList();
            lastStatusPrintTime = currentMillis;
        }
        delay(100);
    }
    if (printDebug) Serial.println("LoRatask deleted");
    vTaskDelete(xHandleLoRa); // Delete the task if ever breaks
}

const __FlashStringHelper *getErrorString(uint8_t error) {
    switch (error) {
    case 1:
        return F("invalid length");
        break;
    case 2:
        return F("no route");
        break;
    case 3:
        return F("timeout");
        break;
    case 4:
        return F("no reply");
        break;
    case 5:
        return F("unable to deliver");
        break;
    }
    return F("unknown");
}

bool initializeMESH() {
    if (!mesh->init()) {
        // if (printDebug) Serial.println("Mesh initialization failed");
        return false;
    }
    return true;
}

void broadcastPresence() {
    const char *presenceMsg = "Present";
    uint8_t status = mesh->sendtoWait((uint8_t *)presenceMsg, strlen(presenceMsg) + 1, RH_BROADCAST_ADDRESS);
    if (status == RH_ROUTER_ERROR_NONE) {
        if (printDebug) Serial.println("Presence Message sent successfully");
    }
    else {
        if (printDebug) {
            Serial.print("Failed to send Presence message, error: ");
            Serial.println(status);
            Serial.println((const __FlashStringHelper *)getErrorString(status));
        }
    }
}

void broadcastDateTime() {
    DateTime now = rtc.now();
    String DateTimeMsg = "DATETIME:" + String(now.year()) + "," + String(now.month()) + "," + String(now.day()) + "," + String(now.hour()) + "," + String(now.minute()) + "," + String(now.second());
    uint8_t status = mesh->sendtoWait((uint8_t *)DateTimeMsg.c_str(), DateTimeMsg.length() + 1, RH_BROADCAST_ADDRESS);
    if (status == RH_ROUTER_ERROR_NONE) {
        if (printDebug) Serial.println("broadcastDateTime Message sent successfully");
    } else {
        if (printDebug) {
            Serial.print("Failed to send message, error: ");
            Serial.println(status);
            Serial.println((const __FlashStringHelper *)getErrorString(status));
        }
    }
}

// Function to listen for other nodes and update their status
void listenForNodes() {
    uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;

    if (mesh->recvfromAckTimeout(buf, &len, 2000, &from)) {
        if (printDebug) {
            Serial.print("Received message from node ");
            Serial.print(from);
            Serial.print(": ");
            Serial.println((char *)buf);
        }

        // Update node information or add new node
        updateNodeStatus(from);
    }
}

void updateDateTime(const char* dateTimeMsg) {
    int year, month, day, hour, minute, second;
    sscanf(dateTimeMsg, "%d,%d,%d,%d,%d,%d", &year, &month, &day, &hour, &minute, &second);
    
    if (printDebug) {
        Serial.print("Year : ");
        Serial.println(year);
        Serial.print("Month : ");
        Serial.println(month);
        Serial.print("Day : ");
        Serial.println(day);
        Serial.print("Hour : ");
        Serial.println(hour);
        Serial.print("Minute : ");
        Serial.println(minute);
        Serial.print("Second : ");
        Serial.println(second);
    }

    DateTime newDateTime(year, month, day, hour, minute, second);
    rtc.adjust(newDateTime);
    if (printDebug) Serial.println("Date Time Updated");
}

// Function to update or add a node status in the list
void updateNodeStatus(uint8_t nodeId) {
    bool nodeFound = false;
    unsigned long currentTime = millis();
    for (auto &status : nodeStatuses) {
        if (status.nodeId == nodeId) {
            status.lastSeen = currentTime;
            status.isActive = true;
            nodeFound = true;
            break;
        }
    }
    if (!nodeFound) {
        nodeStatuses.push_back({nodeId, currentTime, true});
    }
}

// Function to check and update the activity status of nodes
void checkNodeActivity() {
    unsigned long currentTime = millis();
    const unsigned long timeout = 60000; // 1 minute timeout to consider a node as dead
    for (auto &status : nodeStatuses) {
        if (status.isActive && (currentTime - status.lastSeen > timeout)) {
            status.isActive = false;
            if (printDebug) {
                Serial.print("Node ");
                Serial.print(status.nodeId);
                Serial.println(" is now considered dead.");
            }
        }
    }
}

// Function to get the total number of known nodes
size_t getTotalNodes() {
    return nodeStatuses.size();
}

// Function to print the status of all nodes known to this node
void printNodeStatuses() {
    size_t totalNodes = getTotalNodes();
    if (printDebug) Serial.println("Total Nodes: " + totalNodes);

    for (const auto &status : nodeStatuses) { // range based loop
        if (printDebug) {
            Serial.print("Node ");
            Serial.print(status.nodeId);
            Serial.print(": ");
            Serial.println(status.isActive ? "Active" : "Dead");
        }
    }
}

void printNetworkStats() {
    // int totalNodes = nodeStatuses.size();  // Total number of nodes is the size of the vector
    // int activeNodes = 0;
    // int deadNodes = 0;

    totalNodes = 0;
    activeNodes = 0;
    deadNodes = 0;

    // Count active and dead nodes
    for (const auto &status : nodeStatuses) {
        if (printDebug) {
            Serial.print("Node ");
            Serial.print(status.nodeId);
            Serial.print(": ");
            Serial.println(status.isActive ? "Active" : "Dead");
        }

        if (status.isActive) {
            activeNodes++; // Increment active node count
        }
        else {
            deadNodes++; // Increment dead node count
        }
    }

    // Total number of nodes is the size of the vector
    totalNodes = nodeStatuses.size();

    if (printDebug) {
        // Print the results
        Serial.print("Total Nodes: ");
        Serial.println(totalNodes);
        Serial.print("Active Nodes: ");
        Serial.println(activeNodes);
        Serial.print("Dead Nodes: ");
        Serial.println(deadNodes);
    }
}

// Led functions
void setupLeds() {
    FastLED.setBrightness(RGB_LED_BRIGHTNESS_IDEAL);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB1>(BigHexagonAndAlarmCallPointLEDs, NUM_LEDS_RGB1);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB2>(SmallHexagonsAndFireLEDs, NUM_LEDS_RGB2);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB3>(LeftArrowLEDs, NUM_LEDS_RGB3);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB4>(RightArrowLEDs, NUM_LEDS_RGB4);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB5>(SideLEDs, NUM_LEDS_RGB5);

    // set all leds to white
    FillSolidLeds(BigHexagonAndAlarmCallPointLEDs, NUM_LEDS_RGB1, CRGB::White);
    FillSolidLeds(SmallHexagonsAndFireLEDs, NUM_LEDS_RGB2, CRGB::White);
    FillSolidLeds(LeftArrowLEDs, NUM_LEDS_RGB3, CRGB::White);
    FillSolidLeds(RightArrowLEDs, NUM_LEDS_RGB4, CRGB::White);
    FillSolidLeds(SideLEDs, NUM_LEDS_RGB5, CRGB::White);
}

void FillSolidLeds(struct CRGB *targetArray, int numToFill, const struct CRGB &color) {
    fill_solid(targetArray, numToFill, color);
    FastLED.show();
}

void ActivateRGBs(bool activate, bool dir) {
    if (activate) {
        FillSolidLeds(BigHexagonAndAlarmCallPointLEDs, NUM_LEDS_RGB1, CRGB::White);
        BlinkLeds(blink_speed);        // parameter: Blink speed
        RgbArrowMove(dir, move_speed); // 1st parameter: 1 for Right arrow, 0 for Left arrow 2nd parameter: move speed
    }
    else {
        FillSolidLeds(BigHexagonAndAlarmCallPointLEDs, NUM_LEDS_RGB1, CRGB::White);
        FillSolidLeds(SmallHexagonsAndFireLEDs, NUM_LEDS_RGB2, CRGB::White);
        FillSolidLeds(LeftArrowLEDs, NUM_LEDS_RGB3, CRGB::White);
        FillSolidLeds(RightArrowLEDs, NUM_LEDS_RGB4, CRGB::White);
        FillSolidLeds(SideLEDs, NUM_LEDS_RGB5, CRGB::White);
    }
}

void BlinkLeds(int duration) {
    if (millis() - millis_blink_rgb >= duration) {
        millis_blink_rgb = millis();
        prev_blink_color = !prev_blink_color;
        if (prev_blink_color) {
            FillSolidLeds(SideLEDs, NUM_LEDS_RGB5, CRGB::Red);
            FillSolidLeds(SmallHexagonsAndFireLEDs, NUM_LEDS_RGB2, CRGB::Red);
        }
        else {
            FillSolidLeds(SideLEDs, NUM_LEDS_RGB5, CRGB::Black);
            FillSolidLeds(SmallHexagonsAndFireLEDs, NUM_LEDS_RGB2, CRGB::Black);
        }
    }
}

void RgbArrowMove(bool dir, uint8_t speed) {
    if (millis() - millis_move_rgb > SECOND_TO_MILLIS / speed) {
        millis_move_rgb = millis();

        if (dir) { // 1 for Right arrow, 0 for Left arrow
            FillSolidLeds(LeftArrowLEDs, NUM_LEDS_RGB4, CRGB::White);
            RightArrowLEDs[move_id] = CRGB::Red;
        }
        else {
            FillSolidLeds(RightArrowLEDs, NUM_LEDS_RGB4, CRGB::White);
            LeftArrowLEDs[move_id] = CRGB::Red;
        }
        FastLED.show();
        // if (printDebug) Serial.println("move_id: " + String(move_id));
        move_id++;

        if (move_id == (NUM_LEDS_RGB4 + 1)) {
            move_id = 0;
            for (int i = 0; i < NUM_LEDS_RGB4; i++) { // Put this in void setup()
                if (dir) {// 1 for Right arrow, 0 for Left arrow
                    FillSolidLeds(LeftArrowLEDs, NUM_LEDS_RGB4, CRGB::White);
                    RightArrowLEDs[i] = CRGB::White;
                }
                else {
                    FillSolidLeds(RightArrowLEDs, NUM_LEDS_RGB4, CRGB::White);
                    LeftArrowLEDs[i] = CRGB::White;
                }
            }
            FastLED.show();
        }
    }
}

void initAudio() {
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(16); // default 0...21
}

void download_audio() {
    sd_card();
    // Download both files
    //   downloadFile(resourceURL, filename); // uncomment to download audio file 1
    //   downloadFile(resourceURL2, filename2); // uncomment to download audio file 2
}

void sd_card() {
    if (!SD.begin(5)) {
        if (printDebug) Serial.println("Card Mount Failed");
        return;
    }
}

// Function to download a file from a given URL and save it to the specified filename
void downloadFile(const char *resourceURL, const char *filename) {
    HTTPClient http;
    http.begin(resourceURL);
    int httpCode = http.GET();

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_FOUND || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            // Handle redirection
            String newURL = http.getLocation();
            http.end();            // Close the first connection
            http.begin(newURL);    // Open connection with new URL
            httpCode = http.GET(); // Repeat GET request
        }

        if (httpCode == HTTP_CODE_OK) {
            File file = SD.open(filename, FILE_WRITE);
            if (file) {
                http.writeToStream(&file);
                file.close();
                if (printDebug) Serial.println("File downloaded and saved to SD card");
            }
            else {
                if (printDebug) Serial.println("Failed to open file for writing");
            }
        }
        else {
            if (printDebug) Serial.printf("Failed to retrieve file. HTTP error: %s\n", http.errorToString(httpCode).c_str());
        }
    }
    else {
        if (printDebug) Serial.printf("Failed to retrieve file. HTTP error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

void buttonTask(void *parameter) {
    if (printDebug) Serial.println("buttonTask started");
    for(;;) {
        if (!evacuationActivefromLCD) {
            while (digitalRead(siteEvacuation_buttonPin) == LOW) {
                // send message to other nodes to start site evacuation
                sendActivationMessage();

                evacuationActivefromBTN = true;
                if (printDebug) Serial.println("Evacuation started from button");

                // send message to other nodes to start site evacuation
                sendActivationMessage();

                activateRGBflag = true;
                activateSoundflag = true;

                // To run leds in infinite loop upon activation
                xTaskCreate(rgbTask, "rgbTask", 10000, NULL, 9, &xHandleRGB);

                // To play audio and siren bell in infinite loop upon activation
                xTaskCreate(soundTask, "soundTask", 10000, NULL, 9, &xHandleSound);

                // send message to other nodes to start site evacuation
                sendActivationMessage();

                // To send activation SMS to 5 users
                xTaskCreate(sendNotificationSMS, 
                        "sendNotificationSMS", 
                        8192,
                        (void*)"ALERT! A Fyrebox unit has been activated. Please assess and follow the emergency procedure once verified.", // Parameter to pass
                        1, 
                        &xHandlesms);

                // Start slideShow
                slideShowFlag = true;
                slideShow_EvacuationDiagrams_forButton();
                delay(50);
            }
            evacuationActivefromBTN = false;
            delay(100);  // Delay to avoid overloading the server and to reset watchdog
        }

        delay(100);  // Delay to reset watchdog
    }
    if (printDebug) Serial.println("xHandleButton deleted");
    vTaskDelete(xHandleButton); // Delete the task
}

void RecvMessageTask(void *parameter) {
    if (printDebug) Serial.println("messageTask started");
    for(;;) {
        uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        uint8_t from;

        if (mesh->recvfromAckTimeout(buf, &len, 2000, &from)) {
            if (printDebug) {
                Serial.print("Received message from node ");
                Serial.print(from);
                Serial.print(": ");
                Serial.println((char *)buf);
            }

            String temp = (char *)buf;

            if(temp == "FRS-Act") {
                if (printDebug) Serial.println("messageTask received Activate");
                activatedByLoRa = true;
            }
            else if (temp == "Deact-FRS") {
                if (printDebug) Serial.println("messageTask received Deactivate");
                activatedByLoRa = false;
                deactivate = true;
            }
            else if (temp == "Present") {
                // Update node information or add new node
                updateNodeStatus(from);
            }
            else if (temp.startsWith("DATETIME:")) {
                updateDateTime((char *)buf + 9);
            }
        }
        delay(100);  // Delay to reset watchdog 
    }
    if (printDebug) Serial.println("RecvMessageTask deleted");
    vTaskDelete(xHandleRecmessage); // Delete the task
}

void sendActivationMessage() {
    deactivate = false;
  const char* activeMsg = "FRS-Act";
  uint8_t status = mesh->sendtoWait((uint8_t*)activeMsg, strlen(activeMsg) + 1, RH_BROADCAST_ADDRESS);
  if (status == RH_ROUTER_ERROR_NONE) {
      if (printDebug) Serial.println("Activation message sent successfully");
  } 
  else {
    if (printDebug) {
        Serial.print("Failed to send Activation message, error: ");
        Serial.println(status);
        Serial.println((const __FlashStringHelper*)getErrorString(status));
    }
  }
}

void sendDeactivationMessage() {
  const char* activeMsg = "Deact-FRS";
  uint8_t status = mesh->sendtoWait((uint8_t*)activeMsg, strlen(activeMsg) + 1, RH_BROADCAST_ADDRESS);
  if (status == RH_ROUTER_ERROR_NONE) {
      if (printDebug) Serial.println("Deactivation message sent successfully");
  } 
  else {
    if (printDebug) {
        Serial.print("Failed to send Deactivation message, error: ");
        Serial.println(status);
        Serial.println((const __FlashStringHelper*)getErrorString(status));
    }
  }
}

void rgbTask(void *parameter) {
    if (printDebug) Serial.println("rgbTask started");
    for(;;) {
        if(activateRGBflag) {
            // if (printDebug) Serial.println("rgbTask Running");
            ActivateRGBs(true, arrowDirection);
        }
        else if (!activateRGBflag) {
            ActivateRGBs(false);

            FillSolidLeds(BigHexagonAndAlarmCallPointLEDs, NUM_LEDS_RGB1, CRGB::White);
            FillSolidLeds(SmallHexagonsAndFireLEDs, NUM_LEDS_RGB2, CRGB::White);
            FillSolidLeds(LeftArrowLEDs, NUM_LEDS_RGB3, CRGB::White);
            FillSolidLeds(RightArrowLEDs, NUM_LEDS_RGB4, CRGB::White);
            FillSolidLeds(SideLEDs, NUM_LEDS_RGB5, CRGB::White);
            break;
        }

        delay(10);
    }
    if (printDebug) {
        Serial.println("rgbTask completed");
        Serial.println("rgbTask deleted");
    }
    vTaskDelete(xHandleRGB);
}

void soundTask(void *parameter) {
    if (printDebug) Serial.println("soundTask started");
    for(;;) {
        if(activateSoundflag) {
            // Check if audio is playing
            if (audio.isRunning()) {
                // if (printDebug) Serial.println("Audio is playing");
                delay(10);
            }
            else {
                digitalWrite(SirenPIN, HIGH);
                // delay(6000);
                digitalWrite(SirenPIN, LOW);
                if (printDebug) Serial.println("Audio has stopped, Restarting audio");
                audio.connecttoFS(SD, filename); // SD card file
                // audio.loop();
            }
            audio.loop();
        }
        else if (!activateSoundflag) {
            digitalWrite(SirenPIN, LOW); // stop siren
            if (audio.isRunning()) {
                audio.setVolume(0);
                audio.stopSong(); // stop audio
                audio.setVolume(16);
            }
            delay(10);
            break;
        }
    }
    if (printDebug) {
        Serial.println("soundTask completed");
        Serial.println("soundTask deleted");
    }
    vTaskDelete(xHandleSound);
}

// Sends activation and battery low notification to 5 users
void sendNotificationSMS(void *parameter) {
    if (printDebug) Serial.println("SMS task started");

    String message = (char*)parameter;
    int successCount = 0;
    int failureCount = 0;

    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;

        for (int i = 0; i < numRecipients; i++) {
            // Your Domain name with URL path or IP address with path
            http.begin(serverName);

            // If you need Node-RED/server authentication, insert user and password below
            //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

            String recv_token = "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJodHRwczovL2F1dGg6ODA4MC9hcGkvdjEvdXNlcnMvYXBpL2tleXMvZ2VuZXJhdGUiLCJpYXQiOjE3MTc1MTA4OTgsIm5iZiI6MTcxNzUxMDg5OCwianRpIjoiSWxhQTBsQUJkVGVQUlF4NiIsInN1YiI6NDU3ODYxLCJwcnYiOiIyM2JkNWM4OTQ5ZjYwMGFkYjM5ZTcwMWM0MDA4NzJkYjdhNTk3NmY3In0.KutM0gVPFasjWkOgsBhKU5jpklH_U8mhpEZ3qraCBHE";  // Complete Bearer token
            recv_token = "Bearer " + recv_token;    // Adding "Bearer " before token

            // If you need an HTTP request with a content type: application/json, use the following:
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Authorization", recv_token);
            String payload = "{\"message\":\"" + message + "\",\"to\":\"" + String(recipients[i]) + "\",\"bypass_optout\": true,\"sender_id\":\"Fyrebox\",\"callback_url\": \"\"}";
        
            int httpResponseCode = http.POST(payload);

            if (printDebug) {
                Serial.print("HTTP Response code for ");
                Serial.print(recipients[i]);
                Serial.print(": ");
                Serial.println(httpResponseCode);
            }

            if (httpResponseCode >= 200 && httpResponseCode < 300) {
                successCount++;
                String response = http.getString();
                if (printDebug) Serial.println(response);
            } else {
                failureCount++;
                if (printDebug) {
                    Serial.print("Error code: ");
                    Serial.println(httpResponseCode);
                }
            }

            http.end();

            delay(4000);  // Delay to avoid overloading the server and to reset watchdog
        }
        if (printDebug) {
            // Report overall status
            Serial.print("SMS sending completed. Success: ");
            Serial.print(successCount);
            Serial.print(", Failure: ");
            Serial.println(failureCount);
        }
    } 
    else {
        if (printDebug) Serial.println("WiFi Disconnected");
    }
    if (printDebug) Serial.println("SMS task deleted");
    vTaskDelete(xHandlesms); // Suspend the task once done
}

// FOTA functions
int FirmwareVersionCheck() {
    String payload;
    int httpCode;
    String fwurl = "";
    fwurl += URL_fw_Version;
    fwurl += "?";
    fwurl += String(rand());
    if (printDebug) Serial.println(fwurl);
    WiFiClientSecure *client = new WiFiClientSecure;

    if (client) {
        client->setCACert(OTA_CAcert);

        // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
        HTTPClient https;

        if (https.begin(*client, fwurl)) { // HTTPS
            if (printDebug) Serial.print("[HTTPS] GET...\n");
            // start connection and send HTTP header
            delay(100);
            httpCode = https.GET();
            delay(100);
            if (httpCode == HTTP_CODE_OK) { // if version received 
                payload = https.getString(); // save received version
            }
            else {
                if (printDebug) {
                    Serial.print("error in downloading version file:");
                    Serial.println(httpCode);
                }
            }
            https.end();
        }
        delete client;
    }

    if (httpCode == HTTP_CODE_OK) { // if version received
        payload.trim();
        if (printDebug) {
            Serial.print("File Version: ");
            Serial.println(payload);
        }
        if (payload.toFloat() < FirmwareVer.toFloat()) {
            if (printDebug) Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
            return 0;
        }
        else if (payload.equals(FirmwareVer)) {
            if (printDebug) Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
            return 0;
        }
        else {
            if (printDebug) {
                Serial.println(payload);
                Serial.println("New firmware detected");
            }
            return 1;
        }
    }
    return 0;
}

void firmwareUpdate() {
    WiFiClientSecure ota_client;
    ota_client.setCACert(OTA_CAcert);
    t_httpUpdate_return ret = httpUpdate.update(ota_client, URL_fw_Bin);

    switch (ret) {
    case HTTP_UPDATE_FAILED:
        if (printDebug) Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        if (printDebug) Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

    case HTTP_UPDATE_OK:
        if (printDebug) Serial.println("HTTP_UPDATE_OK");
        break;
    }
}

void OTA_repeatedCall() {
    unsigned long OTA_currentMillis = millis();
    if ((OTA_currentMillis - OTA_previousMillis) >= OTA_interval) {
        // save the last time you blinked the LED
        OTA_previousMillis = OTA_currentMillis;
        if (FirmwareVersionCheck()) {
            firmwareUpdate();
        }
    }
}

// Function to print task state
void printTaskState(TaskHandle_t taskHandle, const char *taskName) {
  eTaskState taskState = eTaskGetState(taskHandle);
  if (printDebug) {
    Serial.print(taskName);
    Serial.print(" State: ");
  }
  switch (taskState) {
    case eRunning:
      if (printDebug) Serial.println("Running");
      break;
    case eReady:
      if (printDebug) Serial.println("Ready");
      break;
    case eBlocked:
      if (printDebug) Serial.println("Blocked");
      break;
    case eSuspended:
      if (printDebug) Serial.println("Suspended");
      break;
    case eDeleted:
      if (printDebug) Serial.println("Deleted");
      break;
    default:
      if (printDebug) Serial.println("Unknown");
      break;
  }
}