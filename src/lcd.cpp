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

// RH_E32 driver(&Serial2, MOPIN, M1PIN, AUXPIN); //M0, M1, AUX
RH_E32 driver(&LoRaSerial, MOPIN, M1PIN, AUXPIN);
RHMesh mesh(driver, NODEID); // Node ID

std::vector<NodeStatus> nodeStatuses;

Audio audio;
AudioBuffer InBuff;
SemaphoreHandle_t audioSemaphore;

// 24 X LEDs for each of the sides - so 24x 2 (each side). Always on in white. Activation, flashing red. (48 total)

// 3  X LEDS for the big hexagon (always on in white) always on (3 total)

// 12 X LEDS small hexagon (always on, always white) activation flashing red, same as the sides. (12 total)

// 3 X LEDs per arrow (6 in total for both arrows) white normally then on activation the directional arrow turns red and runs (6 total)

// 18 X LEDs for the alarm call point sign - always white, always on (18 total)

// 24 X LEDs for the FIRE sign (always RED, always on. flashing red on activation) (24 total)

// LED configuration
#define DATA_PIN_RGB1 21
#define DATA_PIN_RGB2 26
#define DATA_PIN_RGB3 47
#define DATA_PIN_RGB4 33
#define DATA_PIN_RGB5 34
#define DATA_PIN_RGB6 20

#define NUM_LEDS_RGB6 48 // Left and Right side LEDs
#define NUM_LEDS_RGB5 3  // Right arrow
#define NUM_LEDS_RGB4 3  // Left arrow
#define NUM_LEDS_RGB3 36 // 5 Hexagons and FIRE sign
#define NUM_LEDS_RGB2 21 // Big hexagon and Alarm Call point

#define RGB_LED_BRIGHTNESS 10

CRGBArray<NUM_LEDS_RGB6> SideLEDs;
CRGBArray<NUM_LEDS_RGB5> RightArrowLEDs;
CRGBArray<NUM_LEDS_RGB4> LeftArrowLEDs;
CRGBArray<NUM_LEDS_RGB3> SmallHexagonsAndFireLEDs;
CRGBArray<NUM_LEDS_RGB2> BigHexagonAndAlarmCallPointLEDs;

// Dummy Function to Read Data from Lcd
void readData()
{
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
    Serial.println(data);
}

// Check Lcd Version
void checkVersion()
{
    byte version[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x0F, 0x01};
    Serial1.write(version, sizeof(version));
}

// Write Dataframe to Lcd at Specified VP Addresses
void sendReadCommand(uint16_t address, uint16_t data_length)
{
    byte frames[] = {0x5A, 0xA5, 0x04, 0x83, (byte)(address >> 8), (byte)(address & 0xFF), (byte)data_length};
    Serial1.write(frames, sizeof(frames));
}

void sendWriteCommand(uint16_t address, byte data)
{
    byte frame[] = {0x5A, 0xA5, 0x04, 0x82, (byte)(address >> 8), (byte)(address & 0xFF), data};
    Serial1.write(frame, sizeof(frame));
}

// Reset vp address
void resetVP(uint16_t address)
{
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
String readDataFromDisplay(uint16_t address, uint16_t totalLength, uint16_t maxChunk)
{
    const uint16_t maxChunkSize = maxChunk; // Maximum read length
    String completeData = "";

    while (totalLength > 0)
    {
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

bool readCheckboxState()
{
    sendReadCommand(checkboxVP, 0x01);
    delay(100); // Delay for the display to respond

    if (Serial1.available())
    {
        int available_bytes = Serial1.available();
        byte response[available_bytes];
        Serial1.readBytes(response, available_bytes);

        // Check if the response is for the correct VP address and extract the state
        if (response[3] == 0x83 && response[4] == (byte)(checkboxVP >> 8) && response[5] == (byte)(checkboxVP & 0xFF))
        {
            return response[7] == 1;
        }
    }
    return false;
}

String extractDataBetweenMarkers(String input, String startMarker, String endMarker)
{
    int startPos = input.indexOf(startMarker);
    if (startPos == -1)
    {
        // Start marker not found in the input
        return "";
    }

    int endPos = input.indexOf(endMarker, startPos + startMarker.length());
    if (endPos == -1)
    {
        // End marker not found in the input after the start marker
        return "";
    }

    // Extract the data between the markers
    String extractedData = input.substring(startPos + startMarker.length(), endPos);

    return extractedData;
}

String extractDataBeforeMarker(String input, String startMarker)
{
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

String tempreadResponse()
{
    String completeData = "";
    while (Serial1.available())
    {
        char a = Serial1.read();
        completeData += String(a, HEX);
    }
    return completeData;
}

bool containsPattern(const String &str, const String &pattern)
{
    return str.indexOf(pattern) != -1;
}

String extractDataBetweenPatterns(const String &input, const String &startPattern, const String &endPattern)
{
    int start = input.indexOf(startPattern);
    if (start == -1)
    {
        return ""; // Start pattern not found
    }
    start += startPattern.length(); // Move to the end of the start pattern

    int end = input.indexOf(endPattern, start);
    if (end == -1)
    {
        return ""; // End pattern not found
    }

    return input.substring(start, end); // Extract the data between the patterns
}

String dummyReadResponse()
{
    String completeData = "";
    int byteCount = 0;

    while (Serial1.available())
    {
        // Serial.print(Serial1.read(),HEX);
        char a = Serial1.read();
        // completeData += String(a, HEX);
        if (byteCount > 6)
        {
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

String extractVpAddress(const String &inputHex, const String &vpAddressPattern)
{
    int startPos = inputHex.indexOf(vpAddressPattern);
    if (startPos == -1)
    {
        // VP address pattern not found in the input
        return "";
    }

    // Extract the VP address
    String extractedVpAddress = inputHex.substring(startPos, startPos + vpAddressPattern.length());
    return extractedVpAddress;
}

String removeFirst6Bytes(const String &input)
{
    // Check if the string has at least 12 characters (6 bytes)
    if (input.length() < 12)
    {
        return ""; // Return an empty string if there are not enough characters
    }

    // Return the substring starting from the 13th character
    return input.substring(12);
}

String remove13Characters(const String &input)
{
    // Check if the string has at least 12 characters (6 bytes)
    if (input.length() < 13)
    {
        return ""; // Return an empty string if there are not enough characters
    }

    // Return the substring starting from the 13th character
    return input.substring(13);
}

String removeFirst7Bytes(const String &input)
{
    // Check if the string has at least 12 characters (6 bytes)
    if (input.length() < 14)
    {
        return ""; // Return an empty string if there are not enough characters
    }

    // Return the substring starting from the 13th character
    return input.substring(14);
}

String processFourthAndFifthBytes(const String &checkData)
{
    String vpAddress = extractDataBetweenPatterns(checkData, "a5", "83");
    int start_index;
    int end_index;
    // Ensure the string is long enough (at least 11 characters for 5.5 bytes)
    if (checkData.length() < 11)
    {
        return ""; // Not enough data to process
    }

    if (vpAddress.length() == 1)
    {
        start_index = 7;
        end_index = 11;
    }
    else
    {
        start_index = 8;
        end_index = 12;
    }
    // Extract 4th and 5th bytes (8th to 11th characters in the string)
    String fourthAndFifthBytes = checkData.substring(start_index, end_index);

    // You can now process this extracted data as needed
    // For example, convert it to a different format, interpret it, etc.

    return fourthAndFifthBytes; // Return the processed data
}

void startCheckingPassword(uint16_t passwordDisplay, uint16_t passwordIcon, const String &checkData)
{
    Serial.println("Data from password field: " + checkData);
    String dataLength = extractDataBetweenPatterns(checkData, "a5", "83");
    String removerHeaders;

    if (dataLength.length() == 1)
    {
        removerHeaders = removeFirst6Bytes(checkData);
    }

    else
    {
        removerHeaders = remove13Characters(checkData);
    }

    String passwordData = extractDataBeforeMarker(removerHeaders, "ffff");
    Serial.println(passwordData);
    String password = hexToString(passwordData);
    String hexPassword = toHexString(password); // Convert data to a hex string
    Serial.println("Hex Data :" + hexPassword);

    delay(100);
    sendReadCommand(passwordIcon, 0x1);
    delay(100);
    String iconRead = tempreadResponse();
    Serial.println("Read Icon :" + iconRead);
    if (checkLast3DigitsMatch(iconRead, showPassword))
    {
        // Serial.println("Show Password");
        delay(100);
        writeString(passwordDisplay, hexPassword);
    }
    else if (checkLast3DigitsMatch(iconRead, hidePassword))
    {
        // Serial.println("Hide Password");
        String hiddenPassword = "";
        for (int i = 0; i < password.length(); i++)
        {
            hiddenPassword += '*';
        }
        // Serial.println("Hidden Password: " + hiddenPassword);
        String hexData = toHexString(hiddenPassword);
        Serial.println("Hex Data: " + hexData);
        writeString(passwordDisplay, hexData);
    }
}

String processPasswordDisplay(uint16_t readPassword, uint16_t passwordDisplay, uint16_t passwordIcon)
{
    // Read password
    delay(100);
    sendReadCommand(readPassword, 0x28);
    delay(100);
    String passWord = tempreadResponse();
    Serial.println("Password Data :" + passWord);

    String removerHeaders = removeFirst7Bytes(passWord);
    String passwordData = "";

    if (containsPattern(passWord, "ffff"))
        passwordData = extractDataBeforeMarker(removerHeaders, "ffff");
    else
        passwordData = extractDataBeforeMarker(removerHeaders, "0000");

    Serial.println("Actual Data :" + passwordData);
    String password = hexToString(passwordData);
    delay(100);

    String hexPassword = toHexString(password); // Convert data to a hex string
    //   Serial.println("Hex Data :" + hexPassword);
    //   Serial.println("Password: " + password);

    // Read icon
    delay(100);
    sendReadCommand(passwordIcon, 0x1);
    delay(100);
    String iconRead = tempreadResponse();
    Serial.println("Read Icon Again :" + iconRead);

    if (checkLast3DigitsMatch(iconRead, showPassword))
    {
        Serial.println("Show Password");
        writeString(passwordDisplay, hexPassword);
    }
    else if (checkLast3DigitsMatch(iconRead, hidePassword))
    {
        Serial.println("Hide Password");
        String hiddenPassword = "";
        for (int i = 0; i < password.length(); i++)
        {
            hiddenPassword += '*';
        }
        // Serial.println("Hidden Password: " + hiddenPassword);
        String hexData = toHexString(hiddenPassword);
        // Serial.println("Hex Data: " + hexData);
        writeString(passwordDisplay, hexData);
    }
    return password;
}

// Read the Response from LCD
String readText()
{
    String completeData = "";

    while (Serial1.available())
    {
        char a = Serial1.read();
        completeData += String(a);
    }

    return completeData;
}

// Remove the zeros and Converted to String
String hexToStringRemovedZeros(const String &hex)
{
    String ascii = "";

    // Skip leading zeros
    unsigned int start = 0;
    while (start < hex.length() && hex.substring(start, start + 2) == "00")
    {
        start += 2;
    }

    // Process the string until trailing zeros
    for (unsigned int i = start; i < hex.length(); i += 2)
    {
        String part = hex.substring(i, i + 2);

        // Break if trailing zeros start
        if (part == "00")
        {
            break;
        }

        char ch = (char)strtol(part.c_str(), NULL, 16);
        ascii += ch;
    }

    return ascii;
}

// Convert Hexadecimal String to Text
String hexToString(const String &hex)
{
    String ascii = "";
    for (unsigned int i = 0; i < hex.length(); i += 2)
    {
        String part = hex.substring(i, i + 2);
        char ch = (char)strtol(part.c_str(), NULL, 16);
        ascii += ch;
    }
    return ascii;
}

// Check last four digits: return true if condition meet else false
bool checkLastFourDigitsMatch(const String &inputString, const String &targetDigits)
{
    // Ensure the input string is at least 4 characters long
    if (inputString.length() < 4)
    {
        return false;
    }

    // Extract the last 4 characters of the input string
    String lastFourDigits = inputString.substring(inputString.length() - 4);

    // Compare the last 4 digits with the target digits
    return lastFourDigits.equals(targetDigits);
}

bool checkLast3DigitsMatch(const String &inputString, const String &targetDigits)
{
    // Ensure the input string is at least 3 characters long
    if (inputString.length() < 3)
    {
        return false;
    }

    // Extract the last 3 characters of the input string
    String lastThreeDigits = inputString.substring(inputString.length() - 3);

    // Compare the last 3 digits with the target digits
    return lastThreeDigits.equals(targetDigits);
}

String readResponse()
{
    const int maxResponseLength = 8;
    char responseBytes[maxResponseLength]; // Array to store the bytes
    int availableBytes = Serial1.available();

    // Check if available bytes are less than 5, then keep reading
    while (availableBytes < 5)
    {
        delay(100);
        availableBytes = Serial1.available();
    }

    // Read bytes from Serial1, up to the maximum response length
    int bytesRead = 0;
    while (bytesRead < maxResponseLength && Serial1.available())
    {
        responseBytes[bytesRead] = Serial1.read();
        bytesRead++;
    }

    // Convert the byte array to a hexadecimal string
    String dataStr = "";
    for (int i = 0; i < bytesRead; i++)
    {
        if (responseBytes[i] < 0x10)
        { // Add leading zero for single digit hex values
            dataStr += "0";
        }
        dataStr += String(responseBytes[i], HEX);
    }

    // For debugging: Print the full response
    Serial.print("Full Response: ");
    Serial.println(dataStr);

    return dataStr + "," + String(availableBytes);
}

// Login Credentials for both Client and Admin Panel
Credentials retrieveCredentials(uint16_t ssidCommand, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon)
{
    // Read SSID
    Serial.println("In retrieveCredentials");
    Credentials creds;
    sendReadCommand(ssidCommand, 0x28);
    delay(100);
    String ssidData = dummyReadResponse();
    delay(100);
    String extractedSSID = extractDataBeforeMarker(ssidData, "ffff");
    creds.ssid = hexToString(extractedSSID);
    Serial.println("SSID: " + creds.ssid);

    // Read Password
    creds.password = processPasswordDisplay(passwordCommand, passwordDisplay, passwordIcon);
    Serial.println("Password: " + creds.password);

    return creds;
}

// Login Credentials for Client Panel
void processClientLogin(uint16_t username, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon)
{
    // Read Username
    Serial.println("In processClientLogin");

    sendReadCommand(username, 0x28);
    delay(100);
    String usernameData = dummyReadResponse();
    delay(100);
    String extractedusername = extractDataBeforeMarker(usernameData, "ffff");
    String saveusername = hexToString(extractedusername);

    if (saveusername != "")
    {
        preferences.putString("client_username", saveusername); // Save Username

        // Read Password
        String temppassword = processPasswordDisplay(passwordCommand, passwordDisplay, passwordIcon);

        preferences.putString("client_password", temppassword); // Save Password

        // Only for debugging

        String Username = preferences.getString("client_username", "");
        Serial.println("Username: " + Username);

        String password = preferences.getString("client_password", "");
        Serial.println("password: " + password);
        delay(100);
    }
}

// Login Credentials for Admin Panel
void processAdminLogin(uint16_t username, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon)
{
    // Read username
    Serial.println("In processAdmin_Login");

    sendReadCommand(username, 0x28);
    delay(100);
    String usernameData = dummyReadResponse();
    delay(100);
    String extractedusername = extractDataBeforeMarker(usernameData, "ffff");
    String saveusername = hexToString(extractedusername);

    if (saveusername != "")
    {
        preferences.putString("admin_username", saveusername); // Save username

        // Read Password
        String temppassword = processPasswordDisplay(passwordCommand, passwordDisplay, passwordIcon);

        preferences.putString("admin_password", temppassword); // Save Password

        // Only for debugging

        // String Username = preferences.getString("admin_username", "");
        // Serial.println("Username: " + Username);

        // String pass = preferences.getString("admin_password", "");
        // Serial.println("PASSWORD: " + pass);
    }
}

// Wi-Fi Credentials
void processWiFiCredentials(uint16_t ssid, uint16_t passwordCommand, uint16_t passwordDisplay, uint16_t passwordIcon)
{
    // Read ssid
    Serial.println("In processWiFiCredentials");

    sendReadCommand(ssid, 0x28);
    delay(100);
    String ssidData = dummyReadResponse();
    delay(100);
    String extractedssidData = extractDataBeforeMarker(ssidData, "ffff");
    String savessidData = hexToString(extractedssidData);

    // if(savessidData != "")
    // {
    preferences.putString("internetSSID", savessidData); // Save SSID

    // Read Password
    String temppassword = processPasswordDisplay(passwordCommand, passwordDisplay, passwordIcon);

    preferences.putString("internetPass", temppassword); // Save Password

    // Only for debugging

    // String ssid = preferences.getString("admin_username", "");
    // Serial.println("SSID: " + ssid);

    // String pass = preferences.getString("admin_password", "");
    // Serial.println("PASSWORD: " + pass);
    // }
}

String readOneData(uint16_t ssidCommand)
{
    sendReadCommand(ssidCommand, 0x28);
    delay(100);
    String Data = dummyReadResponse();
    delay(100);
    String extractedData = extractDataBeforeMarker(Data, "ffff");
    String readData = hexToString(extractedData);
    // Serial.println("Unique Data: " + readData);
    return readData;
}

void performLoginCheck(bool &clientLogin, bool &adminLogin)
{
    sendReadCommand(LOGIN, 0x01);
    delay(100);
    String loginData = tempreadResponse();

    if (checkLastFourDigitsMatch(loginData, clientPanelDigits))
    {
        Serial.println("Client Panel Login successful!");
        clientLogin = true;
        adminLogin = false;
    }
    else if (checkLastFourDigitsMatch(loginData, adminPanelDigits))
    {
        Serial.println("Admin Panel Login successful!");
        adminLogin = true;
        clientLogin = false;
    }
    else
    {
        Serial.println("Login failed. Last 4 digits do not match.");
        clientLogin = false;
        adminLogin = false;
    }
}

void readPage()
{
    byte readpage[] = {0x5A, 0xA5, 0x04, 0x83, 0x00, 0x14, 0x01};
    Serial1.write(readpage, sizeof(readpage));
}

// Page Switching
void pageSwitch(byte pageNo)
{
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

void DisplayDeactivateIcon()
{
    // 0x5A, 0xA5 Frame header
    // 0x0F Data length
    // 0x82 write instruction
    // 0x5440 VP
    // 0x30 icon library number 48
    // 0x07 icon write instruction
    // 0x0001 display an icon
    // the starting display coordinates of upper left corner
    // 0x001 icon no 1 in icon 48 lib
    // ff 00 terminator
    // byte iconCommand[] = {0x5A, 0xA5, 0x0F, 0x82, 0x54, 0x40, 0x30, 0x48, 0x00, 0x01, 0x01, 0x68, 0x01, 0x68, 0x00, 0x01, 0xFF, 0x00};
    byte iconCommand[] = {0x5A, 0xA5, 0x0F, 0x82, 0x62, 0x18, 0x32, 0x48, 0x00, 0x01, 0x01, 0x53, 0x01, 0xCA, 0x00, 0x00, 0xFF, 0x00};

    Serial1.write(iconCommand, sizeof(iconCommand));

    // byte icon[] = {0x5A, 0xA5, 0x0F, 0x82, 0x54, 0x40, 0x30, 0x07, 0x00, 0x01, 0x01, 0x68, 0x01, 0x68, 0x00, 0x01, 0xFF, 0x00};
    // Serial1.write(icon, sizeof(icon));
    // for (int i = 0; i < sizeof(icon); i++)
    // {
    //     Serial.print("Icon Byte ");
    //     Serial.print(i);
    //     Serial.print(": ");
    //     Serial.println(icon[i], HEX);
    // }
    delay(500);
}

// Lcd Reset
void systemReset()
{
    byte reset[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x04, 0x55, 0xAA, 0x5A, 0xA5};
    Serial1.write(reset, sizeof(reset));
}

// Compare String
bool compareCredentials(String ssid, String password)
{
    return ssid == predefinedusername && password == predefinedPassword;
}

// Compare Internet
bool compareInternetCredentials(String ssid, String password)
{
    return ssid == predefinedInternetSSID && password == predefinedInternetPassword;
}

String toHexString(const String &data)
{
    String hexString = "";
    for (char c : data)
    {
        char hex[3];
        sprintf(hex, "%02X", c); // Convert each character to a two-digit hexadecimal
        hexString += hex;
    }
    return hexString;
}

void sendDataToLcd(uint16_t vpAddress, const String &data)
{
    String hexData = toHexString(data);
    writeString(vpAddress, hexData);
}

// Function to convert two hexadecimal characters to one byte
byte hexCharToByte(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    return 0;
}

byte hexStringToByte(const String &hex)
{
    return (hexCharToByte(hex.charAt(0)) << 4) | hexCharToByte(hex.charAt(1));
}

// Helper function to convert two hexadecimal characters to one byte
byte hexToByte(const char *hex)
{
    return (byte)strtol(hex, NULL, 16);
}

void writeString(uint16_t address, const String &hexData)
{
    size_t dataLength = hexData.length() / 2; // Each byte is represented by two hex characters
    byte *charData = new byte[dataLength];

    // Convert hex string to bytes
    for (size_t i = 0; i < dataLength; i++)
    {
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

String ReturnJson(String url, DynamicJsonDocument &doc)
{
    String jsonResult = ""; // String to hold the JSON result

    HTTPClient http;
    delay(5);
    http.begin(url); // API URL
    Serial.println("URL " + url);
    int httpCode = http.GET();
    Serial.println(httpCode);

    if (httpCode > 0)
    { // Check for the returning code
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);

        // Parse JSON (optional, if you need to process it)
        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            // return;
        }

        jsonResult = payload; // Store the JSON string
    }
    else
    {
        Serial.println("Error on HTTP request");
        jsonResult = "Error"; // Error indication
    }

    http.end(); // Free resources

    return jsonResult; // Return the JSON string
}

bool processGPRMC(String gprmcString)
{
    Serial.println("INGPRMC");
    int timeIndex = gprmcString.indexOf(',');
    int validityIndex = gprmcString.indexOf(',', timeIndex + 1);
    int latitudeIndex = gprmcString.indexOf(',', validityIndex + 1);
    int northSouthIndex = gprmcString.indexOf(',', latitudeIndex + 1);
    int longitudeIndex = gprmcString.indexOf(',', northSouthIndex + 1);
    int eastWestIndex = gprmcString.indexOf(',', longitudeIndex + 1);
    int dateIndex = gprmcString.indexOf(',', eastWestIndex + 1);
    for (int i = 0; i < 2; ++i)
    { // Skipping two unused fields
        dateIndex = gprmcString.indexOf(',', dateIndex + 1);
    }

    // Extracting data
    String timeString = gprmcString.substring(timeIndex + 1, validityIndex);
    String validity = gprmcString.substring(validityIndex + 1, latitudeIndex);
    String latitudeString = gprmcString.substring(latitudeIndex + 1, northSouthIndex);
    String longitudeString = gprmcString.substring(longitudeIndex + 1, eastWestIndex);
    String dateString = gprmcString.substring(dateIndex + 1, gprmcString.indexOf(',', dateIndex + 1));

    // Check if data is valid
    if (validity != "A")
    {
        return false;
    }

    int hour = timeString.substring(0, 2).toInt();
    int minute = timeString.substring(2, 4).toInt();
    int second = timeString.substring(4, 6).toInt();
    int day = dateString.substring(0, 2).toInt();
    int month = dateString.substring(2, 4).toInt();
    int year = dateString.substring(4, 6).toInt() + 2000; // Adjust for century

    Serial.print("Validity: ");
    Serial.println(validity);
    Serial.print("Latitude: ");
    Serial.println(latitudeString);
    Serial.print("Longitude: ");
    Serial.println(longitudeString);
    Serial.print("Date: ");
    Serial.println(dateString);

    // Set the RTC with the GPS time
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    Serial.print("RTC set to GPS time: ");
    Serial.println(rtc.now().timestamp());
    String locationOfUnit = "Latitude: " + latitudeString + " Longitude: " + longitudeString;
    String locationBytes = toHexString(locationOfUnit);
    delay(10);
    writeString(VP_LOCATION_OF_UNIT, locationBytes);
    delay(10);

    return true;
}

void sendIconcommand(uint16_t pageVP, byte icon0, byte icon1, byte icon2, byte icon3, byte icon4, byte icon5)
{
    byte iconcommand[] = {0x5A, 0xA5, 0x2D, 0x82, (byte)(pageVP >> 8), (byte)(pageVP & 0xFF), 0x30, 0x07, 0x00, 0x06, 0x03, 0x3E, 0x00, 0xC8, 0x00, icon0,
                          0x03, 0x3E, 0x01, 0x04, 0x00, icon1,
                          0x03, 0x3E, 0x01, 0x40, 0x00, icon2,
                          0x03, 0x3E, 0x01, 0x7C, 0x00, icon3,
                          0x03, 0x3E, 0x01, 0xB8, 0x00, icon4,
                          0x03, 0x3E, 0x01, 0xF4, 0x00, icon5, 0xFF, 0x00};

    /*
    // Printing on serial monitor to check command
    for (int i = 0; i < sizeof(iconcommand); i++)
    {
        Serial.print("Icon Byte ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(iconcommand[i], HEX);
    }
    */
    Serial1.write(iconcommand, sizeof(iconcommand));
    Serial.println("Show icon command sent");
    delay(300);
}

String extractKeycode(const String &input)
{
    // Check if the string has at least 13 characters
    if (input.length() < 13 || input.length() > 17)
    {
        return ""; // Return an empty string
    }

    // Return the substring starting from the 12th character
    return input.substring(13);
}

String extractPageVP(const String &input, const String &vpAddressPattern)
{
    int startPos = input.indexOf(vpAddressPattern);
    // Check if the string has at least 8 characters
    if (input.length() < 8 || input.length() > 17 || startPos == -1)
    {
        return ""; // Return an empty string
    }

    // Extract the VP address
    String extractedVpAddress = input.substring(startPos, startPos + 4);
    return extractedVpAddress;
}

String concatinate_checkboxData()
{
    String checkboxData = controlFunction + speakerActivate + firemanActivateBox + bellRingSystemActivation + batteryHealth + ledLightOnWhite + ledRedActivation + smsReceivedFyreboxActivated + lcdScreenWork + systemActivateWeeklySelfTest + evacuatioDiagram + arrowWorking + permanentPower + illuminatedSignalsWorking + batteriesReplacement + flashSignPanel + unitSecured + faciaComponentSecured + evacuationDiagramUptodate + fyreboxFreeObstructions + LogbookUptodate + fyreboxUnitWipedCleaned + anyDamageBox + anyRustUnit;

    return checkboxData;
}

bool isActivityDetected()
{
    // Implement logic to check for activity (e.g., checkData is not empty)
    if (checkData != "")
        return true;

    else
        return false;
}

String devicesAvailable_DB(String url)
{
    HTTPClient http;

    // Send request
    http.begin(url);
    int httpResponseCode = http.GET(); // Use GET method for request

    String STATUS = "";

    if (httpResponseCode > 0)
    {
        //    Serial.print("Response code: ");
        //    Serial.println(httpResponseCode);
        String response = http.getString();

        // Parse JSON
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        STATUS = doc["STATUS"].as<String>();
        //    Serial.println("STATUS: " + STATUS);

        if (STATUS == "ERROR")
        {
            String ERROR_DESCRIPTION = doc["ERROR_DESCRIPTION"].as<String>();
            //      Serial.println("ERROR_DESCRIPTION: " + ERROR_DESCRIPTION);

            //   return ERROR_DESCRIPTION;
        }
        else if (STATUS == "SUCCESSFUL")
        {
            // return STATUS;
            //      Serial.print("Response: ");
            //      Serial.println(response);
        }
    }
    else
    {
        //    Serial.print("Error in request, error code: ");
        //    Serial.println(httpResponseCode);

        //    // Print HTTP error
        //    Serial.print("HTTP error: ");
        //    Serial.println(http.errorToString(httpResponseCode));
    }

    // Free resources
    http.end();

    return STATUS;
}

int getWeekNumberByMonth(int day, int month, int year)
{
    // Array to store the number of days in each month
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Check if the year is a leap year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
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

int getWeekNumberByYear(int day, int month, int year)
{
    // Array to store the number of days in each month
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Check if the year is a leap year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        daysInMonth[2] = 29; // Update February days for leap years
    }

    // Calculate the number of days passed since the beginning of the year
    int daysPassed = day;
    for (int i = 1; i < month; i++)
    {
        daysPassed += daysInMonth[i];
    }

    // Calculate the number of full weeks passed
    int weeksPassed = daysPassed / 7;

    return weeksPassed;
}

void checkGPSTask(void *parameter)
{
    Serial.println("GPS Task started, waiting for signal...");

    while (1)
    {
        if (getGPSTime())
        {
            Serial.println("GPS signal acquired.");
            break; // GPS time obtained, exit the loop
        }
        else
        {
            Serial.println("Waiting for GPS signal...");
            delay(1000);
        }
    }

    Serial.println("GPS Task completed.");
    vTaskDelete(xHandlegps); // Delete this task when done
}

bool getGPSTime()
{
    if (SerialGPS.available())
    {
        Serial.println("GPS Data Availabole");
        String gpsData = SerialGPS.readStringUntil('\n'); // Read a line of GPS data
        Serial.print("Raw GPS Data: ");
        Serial.println(gpsData);

        // Check if the line is a GPRMC string
        if (gpsData.startsWith("$GPRMC"))
        {
            Serial.println("GPRMC");

            return processGPRMC(gpsData);
        }
    }
    return false;
}

void dateTimeTask(void *parameter)
{
    for (;;)
    { // Infinite loop for the task
        DateTime now = rtc.now();

        DAY = String(now.day());
        MONTH = String(now.month());
        YEAR = String(now.year() % 100);

        HOUR = String(now.hour());
        MINUTE = String(now.minute());

        if (DAY.length() == 1)
        {
            DAY = "0" + DAY;
        }

        if (MONTH.length() == 1)
        {
            MONTH = "0" + MONTH;
        }

        if (HOUR.length() == 1)
        {
            HOUR = "0" + HOUR;
        }
        if (MINUTE.length() == 1)
        {
            MINUTE = "0" + MINUTE;
        }

        dateString = DAY + "/" + MONTH + "/" + YEAR;
        timeString = HOUR + ":" + MINUTE;

        // String dateString = String(now.day()) + "/" + String(now.month()) + "/" + String(now.year() % 100);
        // String timeString = String(now.hour()) + ":" + String(now.minute());

        // Serial.print("ESP32 RTC Date Time: ");
        // Serial.println(timeString);
        // Serial.println(dateString);

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

void loginTask(void *parameter)
{
    Serial.println("Login Task Started");

    while (true)
    {
        checkData = tempreadResponse();

        // Ack response from lcd
        if (checkData == "5aa53824f4b")
        {
            checkData = "";
        }

        // Serial.println("Data in loginTask:" +checkData);

        // Copyright accepted
        if (containsPattern(checkData, "2e70"))
        {
            // Get ssid and password of Wi-Fi
            internetSSID = preferences.getString("internetSSID", "");
            internetPassword = preferences.getString("internetPass", "");

            Serial.println("Saved client username is: " + internetSSID);
            Serial.println("Saved client password is: " + internetPassword);

            //  If ssid and password is available then auto connect
            if (internetSSID == predefinedInternetSSID && internetPassword == predefinedInternetPassword)
            {
                pageSwitch(NOTIFICATION_PAGE);

                // String message1 = "This is a Notification Screen";
                // showMessage(notificationStatus1, message1);
                // delay(100);

                String message1 = "Connecting to Wi-Fi";
                showMessage(notificationStatus1, message1);

                delay(5);
                connectInternet();
            }

            else
            {
                delay(500);
                pageSwitch(INTERNETPAGE);
                configureInternet();

                pageSwitch(NOTIFICATION_PAGE);
                delay(1000);
            }

            String message3 = "Logging In";
            showMessage(notificationStatus3, message3);
            delay(1000);

            rememberClient = true;

            if (rememberClient)
            {
                // Get client info for auto login
                String tempClientusername = preferences.getString("client_username", "");
                String tempClientpassword = preferences.getString("client_password", "");
                delay(100);

                Serial.println("Saved client username is: " + tempClientusername);
                Serial.println("Saved client password is: " + tempClientpassword);

                // If username and password is available then auto connect
                if (tempClientusername == predefinedusername && tempClientpassword == predefinedPassword)
                {
                    message3 = "Login Success";
                    showMessage(notificationStatus3, message3);
                    delay(1000);
                }
                else
                {
                    message3 = "Login Fail"; // Wait here and get login credentials again
                    showMessage(notificationStatus3, message3);
                    delay(1000);
                    pageSwitch(CLIENTPAGE);
                    configureLogin();

                    pageSwitch(NOTIFICATION_PAGE);
                    String LoginStatus = "Login Success";
                    showMessage(notificationStatus3, LoginStatus);
                    delay(1000);
                }
            }

            else
            {
                message3 = "Remember Client is not true";
                showMessage(notificationStatus3, message3);
                delay(1000);
            }

            delay(100);
            String uniqueData = readOneData(UNIQUE_KEY);
            delay(100);

            storedUniqueData = uniqueData;

            if (storedUniqueData == uniqueData)
            {
                String message3 = "Start Slide Show after unique key";
                showMessage(notificationStatus4, message3);
                delay(1000);
                // uniqueKeyFlag = true;
            }

            delay(1000);

            // Start slideShow
            slideShowFlag = true;
            slideShow();

            Serial.println("Start Home page Tasks");
            vTaskResume(xHandlehomepage); // Resume the next task before exit
            break;
        }
        /*
                // Switch user show admin
                else if(containsPattern(checkData, switchUser) && containsPattern(checkData, showAdmin))
                {
                    resetVP(ADMIN_SSID);
                    resetVP(ADMIN_PASSWORD);
                    resetVP(ADMIN_PASSWORD_DISPLAY);
                    resetVP(adminLoginStatus);
                    delay(100);

                    // Get Admin info
                    String tempAdminusername = preferences.getString("admin_username", "");
                    String tempAdminpassword = preferences.getString("admin_password", "");
                    delay(100);

                    Serial.println("Saved Admin username is: "+ tempAdminusername);
                    Serial.println("Saved Admin password is: "+ tempAdminpassword);
                    delay(100);

                    String hexdataAdminusername = toHexString(tempAdminusername);
                    writeString(ADMIN_SSID, hexdataAdminusername); // Display Admin's username
                    delay(100);

                    readeyeIcon(tempAdminpassword, ADMIN_PASSWORD, ADMIN_PASSWORD_ICON, ADMIN_PASSWORD_DISPLAY);

                    pageSwitch(ADMINPAGE);
                }

                // Switch user show client
                else if(containsPattern(checkData, switchUser) && containsPattern(checkData, showClient))
                {
                    resetVP(CLIENT_SSID);
                    resetVP(CLIENT_PASSWORD);
                    resetVP(CLIENT_PASSWORD_DISPLAY);
                    resetVP(clientLoginStatus);
                    delay(100);

                    // Get client info
                    String tempClientusername = preferences.getString("client_username", "");
                    String tempClientpassword = preferences.getString("client_password", "");
                    delay(100);

                    Serial.println("Saved client username is: "+ tempClientusername);
                    Serial.println("Saved client password is: "+ tempClientpassword);
                    delay(100);

                    String hexdataClientusername = toHexString(tempClientusername);
                    writeString(CLIENT_SSID, hexdataClientusername); // Display client's username
                    delay(100);

                    readeyeIcon(tempClientpassword, CLIENT_PASSWORD, CLIENT_PASSWORD_ICON, CLIENT_PASSWORD_DISPLAY);

                    pageSwitch(CLIENTPAGE);
                }

                // Client panel or admin panel
                else if (containsPattern(checkData, "ffff"))
                {
                    resetVP(CLIENT_PASSWORD_DISPLAY);
                    // resetVP(ADMIN_PASSWORD_DISPLAY);
                    delay(100);
                    String vpAddress = processFourthAndFifthBytes(checkData);
                    Serial.println("Vp Address :" + vpAddress);
                    if (vpAddress == "3164")
                    {
                        Serial.println("Client Panel");
                        delay(100);
                        startCheckingPassword(CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON, checkData);
                    }
                    else if (vpAddress == "332f")
                    {
                        Serial.println("Admin Panel");
                        delay(100);
                        startCheckingPassword(ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON, checkData);
                    }
                }

                // Client password show/hide icons
                else if (containsPattern(checkData, "31c8"))
                {
                    Serial.println("In client Icon");
                    delay(100);
                    processPasswordDisplay(CLIENT_PASSWORD, CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON);
                }

                // Admin password show/hide icons
                else if (containsPattern(checkData, "3393"))
                {
                    Serial.println("In admin Icon");
                    delay(100);
                    processPasswordDisplay(ADMIN_PASSWORD, ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON);
                }

                // Admin/Client login button
                else if (containsPattern(checkData, "31ca"))
                {
                    if (checkLastFourDigitsMatch(checkData, clientPanelDigits))
                    {
                        Serial.println("Data from client login button");
                        clientLogin = true;

                        if (RememberIcon(CLIENT_REMEMBER_LOGIN))
                            rememberClient = true;
                        else
                            rememberClient = false;
                    }
                    else if (checkLastFourDigitsMatch(checkData, adminPanelDigits))
                    {
                        Serial.println("Data from admin login button");
                        adminLogin = true;

                        if (RememberIcon(ADMIN_REMEMBER_LOGIN))
                            rememberAdmin = true;
                        else
                            rememberAdmin = false;
                    }
                }

                // Client Login Mode
                if (clientLogin && wifiConnectedFlag)
                {
                    Serial.println("Client Login Mode");
                    delay(100);

                    processClientLogin(CLIENT_SSID, CLIENT_PASSWORD, CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON);

                    String tempusernameClient = preferences.getString("client_username", "");
                    String temppasswordClient = preferences.getString("client_password", "");

                    Serial.println("Saved username for client login: "+ tempusernameClient);
                    Serial.println("Saved password for client login: "+ temppasswordClient);
                    delay(100);

                    if (compareCredentials(tempusernameClient, temppasswordClient))
                    {
                        if(rememberClient)
                        {
                            saveClientCredentials(tempusernameClient, temppasswordClient);
                        }
                        else if(!rememberClient)
                        {
                            removeClientCredentials();
                        }

                        String LoginStatus = "Login Success";
                        String LoginStatusBytes = toHexString(LoginStatus);
                        delay(100);
                        writeString(clientLoginStatus, LoginStatusBytes);
                        delay(1000);
                        resetVP(clientLoginStatus);

                        pageSwitch(UNIQUE_KEY_PAGE);

                        while (true)
                        {
                            delay(100);
                            String uniqueButton = tempreadResponse();
                            Serial.println("Data from uniqueButton: " + uniqueButton);

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
                        }
                    }

                    else
                    {
                        String LoginStatus = "Login Fail";
                        String LoginStatusBytes = toHexString(LoginStatus);
                        delay(100);
                        writeString(clientLoginStatus, LoginStatusBytes);
                    }

                    delay(10);
                    sendWriteCommand(LOGIN, RESET);
                    adminLogin = false;
                    clientLogin = false;
                }

                // Admin Login Mode
                else if (adminLogin && wifiConnectedFlag)
                {
                    Serial.println("Admin Login Mode");
                    delay(100);

                    processAdminLogin(ADMIN_SSID, ADMIN_PASSWORD, ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON);

                    String tempusernameAdmin = preferences.getString("admin_username", "");
                    String temppasswordAdmin = preferences.getString("admin_password", "");

                    Serial.println("Saved username for admin login: "+ tempusernameAdmin);
                    Serial.println("Saved password for admin login: "+ temppasswordAdmin);
                    delay(100);

                    if(compareCredentials(tempusernameAdmin, temppasswordAdmin))
                    {
                        if(rememberAdmin)
                        {
                            saveAdminCredentials(tempusernameAdmin, temppasswordAdmin);
                        }
                        else if(!rememberAdmin)
                        {
                            removeAdminCredentials();
                        }

                        String LoginStatus = "Login Success";
                        String LoginStatusBytes = toHexString(LoginStatus);
                        delay(100);
                        writeString(adminLoginStatus, LoginStatusBytes);
                        delay(1000);
                        resetVP(adminLoginStatus);

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
                    }

                    else
                    {
                        String LoginStatus = "Login Fail";
                        String LoginStatusBytes = toHexString(LoginStatus);
                        delay(100);
                        writeString(adminLoginStatus, LoginStatusBytes);
                    }

                    delay(10);
                    sendWriteCommand(LOGIN, RESET);
                    clientLogin = false;
                    adminLogin = false;
                }
        */
        if (uniqueKeyFlag)
        {
            Serial.println("Start Config Device");
            vTaskResume(xHandleconfigdevice); // Resume the next task before exit
            break;
        }

        delay(100);
    }

    Serial.println("Login Task completed");
    Serial.println("Login Task Suspended");
    vTaskSuspend(xHandlelogin); // Suspend the task
}

void readeyeIcon(String temppassword, uint16_t passwordvp, uint16_t passwordIcon, uint16_t passwordDisplay)
{
    // Important to write password first
    String hexdatapassword = toHexString(temppassword);
    writeString(passwordvp, hexdatapassword); // Display password

    // Read icon
    delay(100);
    sendReadCommand(passwordIcon, 0x1);
    delay(100);
    String iconRead = tempreadResponse();
    Serial.println("Read Icon Again :" + iconRead);

    if (checkLast3DigitsMatch(iconRead, showPassword))
    {
        Serial.println("Show Password");
        hexdatapassword = toHexString(temppassword);
        writeString(passwordDisplay, hexdatapassword); // Display password
    }
    else if (checkLast3DigitsMatch(iconRead, hidePassword))
    {
        Serial.println("Hide Password");
        String hiddenPassword = "";
        for (int i = 0; i < temppassword.length(); i++)
        {
            hiddenPassword += '*';
        }
        // Serial.println("Hidden Password: " + hiddenPassword);
        String hexData = toHexString(hiddenPassword);
        // Serial.println("Hex Data: " + hexData);
        writeString(passwordDisplay, hexData);
    }
}

void connectInternet()
{
    Serial.println("connectInternet Started");

    while (true)
    {
        String internetData = tempreadResponse();
        Serial.println("Data in connect internet:" + internetData);
        delay(100);

        /*Start of Auto connect*/
        internetSSID = preferences.getString("internetSSID", "");
        internetPassword = preferences.getString("internetPass", "");

        // if credentials are available
        if ((!internetSSID.isEmpty() && !internetPassword.isEmpty()))
        {
            WiFi.begin(internetSSID.c_str(), internetPassword.c_str());

            unsigned long startAttemptTime = millis();
            bool isConnected = false;

            while (millis() - startAttemptTime < 10000) // 30 seconds in milliseconds
            {
                if (WiFi.status() == WL_CONNECTED)
                {
                    isConnected = true;
                    Serial.println("WiFi Connected");
                    String InternetLoginStatus = "Wifi Connected";

                    showMessage(notificationStatus1, InternetLoginStatus);
                    wifiConnectedFlag = true;

                    break;
                }
            }

            if (wifiConnectedFlag)
            {
                String message2 = "Downloading Audio";
                Serial.println(message2);
                showMessage(notificationStatus2, message2);

                download_audio();

                message2 = "Audio Downloaded";
                Serial.println(message2);
                showMessage(notificationStatus2, message2);
                delay(100);

                break;
            }

            else if (!isConnected)
            {
                internetSSID = "";
                internetPassword = "";
                resetVP(INTERNET_PASSWORD);
                resetVP(INTERNET_PASSWORD_DISPLAY);
                // resetVP(INTERNET_PASSWORD_ICON);
                resetVP(INTERNET_CONNECT_BUTTON);

                String InternetLoginStatus = "Wifi Not Connected";
                Serial.println("Failed to connect to WiFi within 30 seconds.");

                showMessage(notificationStatus1, InternetLoginStatus);
            }
        }
    }
    Serial.println("connectInternet Completed");
}

void configureInternet()
{
    Serial.println("configureInternet Started");

    while (true)
    {
        String configure_internetData = tempreadResponse();
        Serial.println("Data in configure internet:" + configure_internetData);
        delay(100);

        if (containsPattern(configure_internetData, "ffff"))
        {
            Serial.println("Data VP Address :" + configure_internetData);
            resetVP(INTERNET_PASSWORD_DISPLAY);
            delay(100);

            String vpAddress = processFourthAndFifthBytes(configure_internetData);
            Serial.println("Vp Address Internet :" + vpAddress);

            if (vpAddress == "33fa")
            {
                delay(100);
                startCheckingPassword(INTERNET_PASSWORD_DISPLAY, INTERNET_PASSWORD_ICON, configure_internetData);
            }
        }

        //  Check Icon
        else if (containsPattern(configure_internetData, "345e"))
        {
            Serial.println("In Wi-Fi icon");
            delay(100);
            processPasswordDisplay(INTERNET_PASSWORD, INTERNET_PASSWORD_DISPLAY, INTERNET_PASSWORD_ICON);
        }

        else if (containsPattern(configure_internetData, "345f"))
        {
            delay(100);
            processWiFiCredentials(INTERNET_SSID, INTERNET_PASSWORD, INTERNET_PASSWORD_DISPLAY, INTERNET_PASSWORD_ICON);

            internetSSID = preferences.getString("internetSSID", "");
            internetPassword = preferences.getString("internetPass", "");

            Serial.println("Saved Internet SSID: " + internetSSID);
            Serial.println("Saved Internet Password: " + internetPassword);
            delay(100);

            if (!internetSSID.isEmpty() && !internetPassword.isEmpty())
            {
                Serial.println("Checking Wifi");
                WiFi.begin(internetSSID.c_str(), internetPassword.c_str());

                unsigned long startAttemptTime = millis();
                bool isConnected = false;

                while (millis() - startAttemptTime < 10000) // 30 seconds in milliseconds
                {
                    if (WiFi.status() == WL_CONNECTED)
                    {
                        isConnected = true;
                        String InternetLoginStatus = "Wifi Connected";
                        Serial.println(InternetLoginStatus);
                        resetVP(clientLoginStatus);

                        showMessage(clientLoginStatus, InternetLoginStatus);
                        delay(500);
                        showMessage(notificationStatus1, InternetLoginStatus);

                        wifiConnectedFlag = true;
                        break;
                    }
                    delay(500);
                    String InternetLoginStatus = "Connecting to WiFi...";
                    Serial.println(InternetLoginStatus);
                    resetVP(clientLoginStatus);

                    String InternetLoginStatusBytes = toHexString(InternetLoginStatus);
                    delay(100);
                    writeString(clientLoginStatus, InternetLoginStatusBytes);
                    delay(500);
                    resetVP(clientLoginStatus);
                }

                if (wifiConnectedFlag)
                {
                    saveInternetCredentials(internetSSID, internetPassword);

                    String message2 = "Downloading Audio";
                    Serial.println(message2);
                    showMessage(notificationStatus2, message2);

                    download_audio();

                    message2 = "Audio Downloaded";
                    Serial.println(message2);
                    showMessage(notificationStatus2, message2);
                    delay(100);

                    break;
                }

                else if (!isConnected)
                {
                    internetSSID = "";
                    internetPassword = "";
                    resetVP(INTERNET_PASSWORD);
                    resetVP(INTERNET_PASSWORD_DISPLAY);
                    // resetVP(INTERNET_PASSWORD_ICON);
                    resetVP(INTERNET_CONNECT_BUTTON);
                    Serial.println("Failed to connect to WiFi within 30 seconds.");
                    String InternetLoginStatus = "Wifi Not Connected";
                    resetVP(clientLoginStatus);
                    String InternetLoginStatusBytes = toHexString(InternetLoginStatus);
                    delay(100);
                    writeString(clientLoginStatus, InternetLoginStatusBytes);
                }
            }
        }
    }
    Serial.println("configureInternet Completed");
}

void configureLogin()
{

    Serial.println("configureLogin started");

    // resetVP(ADMIN_SSID);
    // resetVP(ADMIN_PASSWORD);
    // resetVP(ADMIN_PASSWORD_DISPLAY);
    // resetVP(adminLoginStatus);

    // resetVP(CLIENT_SSID);
    // resetVP(CLIENT_PASSWORD);
    // resetVP(CLIENT_PASSWORD_DISPLAY);
    // resetVP(clientLoginStatus);

    while (true)
    {

        checkData = tempreadResponse();

        // Ack response from lcd
        if (checkData == "5aa53824f4b")
        {
            checkData = "";
        }

        // Switch user show admin
        if (containsPattern(checkData, switchUser) && containsPattern(checkData, showAdmin))
        {
            resetVP(ADMIN_SSID);
            resetVP(ADMIN_PASSWORD);
            resetVP(ADMIN_PASSWORD_DISPLAY);
            resetVP(adminLoginStatus);
            delay(100);

            // Get Admin info
            String tempAdminusername = preferences.getString("admin_username", "");
            String tempAdminpassword = preferences.getString("admin_password", "");
            delay(100);

            Serial.println("Saved Admin username is: " + tempAdminusername);
            Serial.println("Saved Admin password is: " + tempAdminpassword);
            delay(100);

            String hexdataAdminusername = toHexString(tempAdminusername);
            writeString(ADMIN_SSID, hexdataAdminusername); // Display Admin's username
            delay(100);

            readeyeIcon(tempAdminpassword, ADMIN_PASSWORD, ADMIN_PASSWORD_ICON, ADMIN_PASSWORD_DISPLAY);

            pageSwitch(ADMINPAGE);
        }

        // Switch user show client
        else if (containsPattern(checkData, switchUser) && containsPattern(checkData, showClient))
        {
            resetVP(CLIENT_SSID);
            resetVP(CLIENT_PASSWORD);
            resetVP(CLIENT_PASSWORD_DISPLAY);
            resetVP(clientLoginStatus);
            delay(100);

            // Get client info
            String tempClientusername = preferences.getString("client_username", "");
            String tempClientpassword = preferences.getString("client_password", "");
            delay(100);

            Serial.println("Saved client username is: " + tempClientusername);
            Serial.println("Saved client password is: " + tempClientpassword);
            delay(100);

            String hexdataClientusername = toHexString(tempClientusername);
            writeString(CLIENT_SSID, hexdataClientusername); // Display client's username
            delay(100);

            readeyeIcon(tempClientpassword, CLIENT_PASSWORD, CLIENT_PASSWORD_ICON, CLIENT_PASSWORD_DISPLAY);

            pageSwitch(CLIENTPAGE);
        }

        // Client panel or admin panel
        else if (containsPattern(checkData, "ffff"))
        {
            resetVP(CLIENT_PASSWORD_DISPLAY);
            // resetVP(ADMIN_PASSWORD_DISPLAY);
            delay(100);
            String vpAddress = processFourthAndFifthBytes(checkData);
            Serial.println("Vp Address :" + vpAddress);
            if (vpAddress == "3164")
            {
                Serial.println("Client Panel");
                delay(100);
                startCheckingPassword(CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON, checkData);
            }
            else if (vpAddress == "332f")
            {
                Serial.println("Admin Panel");
                delay(100);
                startCheckingPassword(ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON, checkData);
            }
        }

        // Client password show/hide icons
        else if (containsPattern(checkData, "31c8"))
        {
            Serial.println("In client Icon");
            delay(100);
            processPasswordDisplay(CLIENT_PASSWORD, CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON);
        }

        // Admin password show/hide icons
        else if (containsPattern(checkData, "3393"))
        {
            Serial.println("In admin Icon");
            delay(100);
            processPasswordDisplay(ADMIN_PASSWORD, ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON);
        }

        // Admin/Client login button
        else if (containsPattern(checkData, "31ca"))
        {
            if (checkLastFourDigitsMatch(checkData, clientPanelDigits))
            {
                Serial.println("Data from client login button");
                clientLogin = true;

                if (RememberIcon(CLIENT_REMEMBER_LOGIN))
                    rememberClient = true;
                else
                    rememberClient = false;
            }
            else if (checkLastFourDigitsMatch(checkData, adminPanelDigits))
            {
                Serial.println("Data from admin login button");
                adminLogin = true;

                if (RememberIcon(ADMIN_REMEMBER_LOGIN))
                    rememberAdmin = true;
                else
                    rememberAdmin = false;
            }
        }

        // Client Login Mode
        if (clientLogin && wifiConnectedFlag)
        {
            Serial.println("Client Login Mode");
            delay(100);

            processClientLogin(CLIENT_SSID, CLIENT_PASSWORD, CLIENT_PASSWORD_DISPLAY, CLIENT_PASSWORD_ICON);

            String tempusernameClient = preferences.getString("client_username", "");
            String temppasswordClient = preferences.getString("client_password", "");

            Serial.println("Saved username for client login: " + tempusernameClient);
            Serial.println("Saved password for client login: " + temppasswordClient);
            delay(100);

            if (compareCredentials(tempusernameClient, temppasswordClient))
            {
                if (rememberClient)
                {
                    saveClientCredentials(tempusernameClient, temppasswordClient);
                }
                else if (!rememberClient)
                {
                    removeClientCredentials();
                }

                String LoginStatus = "Login Success";
                String LoginStatusBytes = toHexString(LoginStatus);
                writeString(clientLoginStatus, LoginStatusBytes);
                delay(1000);
                resetVP(clientLoginStatus);
                /*
                pageSwitch(UNIQUE_KEY_PAGE);

                while (true)
                {
                    delay(100);
                    String uniqueButton = tempreadResponse();
                    Serial.println("Data from uniqueButton: " + uniqueButton);

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
                }
                */
                break;
            }

            else
            {
                String LoginStatus = "Login Fail";
                String LoginStatusBytes = toHexString(LoginStatus);
                delay(100);
                writeString(clientLoginStatus, LoginStatusBytes);
            }

            delay(10);
            sendWriteCommand(LOGIN, RESET);
            adminLogin = false;
            clientLogin = false;
        }

        // Admin Login Mode
        else if (adminLogin && wifiConnectedFlag)
        {
            Serial.println("Admin Login Mode");
            delay(100);

            processAdminLogin(ADMIN_SSID, ADMIN_PASSWORD, ADMIN_PASSWORD_DISPLAY, ADMIN_PASSWORD_ICON);

            String tempusernameAdmin = preferences.getString("admin_username", "");
            String temppasswordAdmin = preferences.getString("admin_password", "");

            Serial.println("Saved username for admin login: " + tempusernameAdmin);
            Serial.println("Saved password for admin login: " + temppasswordAdmin);
            delay(100);

            if (compareCredentials(tempusernameAdmin, temppasswordAdmin))
            {
                if (rememberAdmin)
                {
                    saveAdminCredentials(tempusernameAdmin, temppasswordAdmin);
                }
                else if (!rememberAdmin)
                {
                    removeAdminCredentials();
                }

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

            else
            {
                String LoginStatus = "Login Fail";
                String LoginStatusBytes = toHexString(LoginStatus);
                delay(100);
                writeString(adminLoginStatus, LoginStatusBytes);
            }

            delay(10);
            sendWriteCommand(LOGIN, RESET);
            clientLogin = false;
            adminLogin = false;
        }
    }
    Serial.println("configureLogin completed");
}

void configuredeviceTask(void *parameter)
{
    Serial.println("configuredeviceTask Started");

    while (true)
    {
        DynamicJsonDocument doc(1024);
        // Serial.println("In Device Configuration");
        delay(100);

        String APIresponse = "";

        if (APIresponse == "" && !APIresponseFlag)
        {
            // Send http get request to check devices are avaiable
            APIresponse = devicesAvailable_DB(getOrgDetailsUrl);
            Serial.println(APIresponse);
        }

        if (APIresponse == "SUCCESSFUL")
        {
            vTaskResume(xHandledatetime); // Resume date time task

            APIresponseFlag = true;
            APIresponse = "";
            pageSwitch(COMPANY_DETAIL_AUTO_LOAD);
            delay(5);

            while (true)
            {
                sendReadCommand(VP_AUTO_UPLOAD_COMPANY_DETAILS, 0x1);
                delay(100);
                String autoLoad = tempreadResponse();
                Serial.println("Details auto load: " + autoLoad);

                // Autoload details
                if (containsPattern(autoLoad, companyDoneButtonYes))
                {
                    Serial.println("Will Work with Database API");

                    // Autoload getOrgDetails
                    String jsonOutput_getOrgDetails = ReturnJson(getOrgDetailsUrl, doc);
                    Serial.println("Json Output");
                    Serial.println(jsonOutput_getOrgDetails);
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
                    // Serial.println("Json Output jsonOutput_deviceDetails");
                    // Serial.println(jsonOutput_deviceDetails);
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

                    // Manually add remainig enteries
                    pageSwitch(COMPANY_DETAILS_PAGE1);
                    delay(100);
                    companyDetails();
                    delay(1000); // For device configuration picture

                    // Start slideShow
                    slideShowFlag = true;
                    slideShow();

                    ConfigureDeviceFlag = true;
                    break;
                }

                // Manually enter details
                else if (containsPattern(autoLoad, companyDoneButtonNo))
                {
                    pageSwitch(COMPANY_DETAILS_PAGE1);
                    delay(100);
                    companyDetails();
                    delay(1000); // For device configuration picture

                    // Start slideShow
                    slideShowFlag = true;
                    slideShow();

                    ConfigureDeviceFlag = true;
                    break;
                }
            }
        }

        // Manually enter details
        else
        {
            vTaskResume(xHandledatetime); // Resume date time task

            APIresponseFlag = true;
            APIresponse = "";
            resetVP(COMPANY_DONE_BUTTON_ADDRESS);
            delay(100);

            pageSwitch(COMPANY_DETAILS_PAGE1);
            delay(100);
            companyDetails();
            delay(1000); // For device configuration picture

            // Start slideShow
            slideShowFlag = true;
            slideShow();

            ConfigureDeviceFlag = true;
        }

        if (ConfigureDeviceFlag)
        {
            Serial.println("Start hompageTasks");
            vTaskResume(xHandlehomepage); // Resume the next task before exit
            break;
        }
        delay(100);
    }
    Serial.println("configuredeviceTask completed");
    Serial.println("configuredeviceTask deleted");
    vTaskDelete(xHandleconfigdevice); // Delete the task
}

void companyDetails()
{
    Serial.println("companyDetails Started");

    while (true)
    {
        resetVP(COMPANY_DONE_BUTTON_ADDRESS);
        delay(100);
        sendReadCommand(COMPANY_DONE_BUTTON_ADDRESS, 0x1);
        delay(100);
        String companyDetails = tempreadResponse();
        Serial.println("Company Details: " + companyDetails);

        if (containsPattern(companyDetails, companyDetails_page1_next))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            delay(100);
            sendReadCommand(VP_COMPANY_NAME, 0x28);
            delay(100);
            String tempcompanyName = tempreadResponse();
            delay(100);
            String cnremoverHeaders = removeFirst7Bytes(tempcompanyName);
            String cnextractedData = extractDataBeforeMarker(cnremoverHeaders, "ffff");
            companyName = hexToString(cnextractedData);
            Serial.println("Company Name: " + companyName);
            delay(100);

            sendReadCommand(VP_COMPANY_ADDRESS, 0x28);
            delay(100);
            String tempcompanyAddress = tempreadResponse();
            delay(100);
            String caremoverHeaders = removeFirst7Bytes(tempcompanyAddress);
            String caextractedData = extractDataBeforeMarker(caremoverHeaders, "ffff");
            companyAddress = hexToString(caextractedData);
            Serial.println("Company Address: " + companyAddress);
            delay(100);

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON_NAME, 0x28);
            delay(100);
            String tempkeyResponsiblePersonName = tempreadResponse();
            delay(100);
            String pnemoverHeaders = removeFirst7Bytes(tempkeyResponsiblePersonName);
            String pncaextractedData = extractDataBeforeMarker(pnemoverHeaders, "ffff");
            keyResponsiblePersonName = hexToString(pncaextractedData);
            Serial.println("Key Responsible Person Name: " + keyResponsiblePersonName);
            delay(100);

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON_CONTACT, 0x28);
            delay(100);
            String tempkeyResponsiblePersonContact = tempreadResponse();
            delay(100);
            String pcemoverHeaders = removeFirst7Bytes(tempkeyResponsiblePersonContact);
            String pcextractedData = extractDataBeforeMarker(pcemoverHeaders, "ffff");
            keyResponsiblePersonContact = hexToString(pcextractedData);
            Serial.println("Key Responsible Person Contact: " + keyResponsiblePersonContact);
            delay(100);

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON1_NAME, 0x28);
            delay(100);
            String tempkeyResponsiblePerson1Name = tempreadResponse();
            delay(100);
            String pn1emoverHeaders = removeFirst7Bytes(tempkeyResponsiblePerson1Name);
            String pn1extractedData = extractDataBeforeMarker(pn1emoverHeaders, "ffff");
            keyResponsiblePerson1Name = hexToString(pn1extractedData);
            Serial.println("Key Responsible Person1 Name: " + keyResponsiblePerson1Name);
            delay(100);

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON1_CONTACT, 0x28);
            delay(100);
            String tempkeyResponsiblePerson1Contact = tempreadResponse();
            delay(100);
            String pc1emoverHeaders = removeFirst7Bytes(tempkeyResponsiblePerson1Contact);
            String pc1extractedData = extractDataBeforeMarker(pc1emoverHeaders, "ffff");
            keyResponsiblePerson1Contact = hexToString(pc1extractedData);
            Serial.println("Key Responsible Person1 Contact: " + keyResponsiblePerson1Contact);
            delay(100);

            if ((companyName == "") || (companyAddress == "") || (keyResponsiblePersonName == "") || (keyResponsiblePersonContact == "") || (keyResponsiblePerson1Name == "") || (keyResponsiblePerson1Contact == ""))
            {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else
            {
                pageSwitch(COMPANY_DETAILS_PAGE2);
                Serial.println("Page Switched");
                delay(5);
            }
        }

        else if (containsPattern(companyDetails, companyDetails_page2_next))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON2_NAME, 0x28);
            delay(100);
            String tempkeyResponsiblePerson2Name = tempreadResponse();
            delay(100);
            String pn2emoverHeaders = removeFirst7Bytes(tempkeyResponsiblePerson2Name);
            String pn2extractedData = extractDataBeforeMarker(pn2emoverHeaders, "ffff");
            keyResponsiblePerson2Name = hexToString(pn2extractedData);
            Serial.println("Key Responsible Person2 Name: " + keyResponsiblePerson2Name);
            delay(100);

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON2_CONTACT, 0x28);
            delay(100);
            String tempkeyResponsiblePerson2Contact = tempreadResponse();
            delay(100);
            String pc2emoverHeaders = removeFirst7Bytes(tempkeyResponsiblePerson2Contact);
            String pc2extractedData = extractDataBeforeMarker(pc2emoverHeaders, "ffff");
            keyResponsiblePerson2Contact = hexToString(pc2extractedData);
            Serial.println("Key Responsible Person2 Contact: " + keyResponsiblePerson2Contact);
            delay(100);

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON3_NAME, 0x28);
            delay(100);
            String tempkeyResponsiblePerson3Name = tempreadResponse();
            delay(100);
            String pn3emoverHeaders = removeFirst7Bytes(tempkeyResponsiblePerson3Name);
            String pn3extractedData = extractDataBeforeMarker(pn3emoverHeaders, "ffff");
            keyResponsiblePerson3Name = hexToString(pn3extractedData);
            Serial.println("Key Responsible Person3 Name: " + keyResponsiblePerson3Name);
            delay(100);

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON3_CONTACT, 0x28);
            delay(100);
            String tempkeyResponsiblePerson3Contact = tempreadResponse();
            delay(100);
            String pc3emoverHeaders = removeFirst7Bytes(tempkeyResponsiblePerson3Contact);
            String pc3extractedData = extractDataBeforeMarker(pc3emoverHeaders, "ffff");
            keyResponsiblePerson3Contact = hexToString(pc3extractedData);
            Serial.println("Key Responsible Person3 Contact: " + keyResponsiblePerson3Contact);
            delay(100);

            if ((keyResponsiblePerson2Name == "") || (keyResponsiblePerson2Contact == "") || (keyResponsiblePerson3Name == "") || (keyResponsiblePerson3Contact == ""))
            {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else
            {
                pageSwitch(COMPANY_DETAILS_PAGE3);
                Serial.println("Page Switched");
                delay(5);
            }
        }

        else if (containsPattern(companyDetails, companyDetails_page2_back))
        {
            pageSwitch(COMPANY_DETAILS_PAGE1);
            Serial.println("Page Switched");
            delay(5);
        }

        else if (containsPattern(companyDetails, companyDetails_page3_next))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON4_NAME, 0x28);
            delay(100);
            String tempkeyResponsiblePerson4Name = tempreadResponse();
            delay(100);
            String pn4emoverHeaders = removeFirst7Bytes(tempkeyResponsiblePerson4Name);
            String pn4extractedData = extractDataBeforeMarker(pn4emoverHeaders, "ffff");
            keyResponsiblePerson4Name = hexToString(pn4extractedData);
            Serial.println("Key Responsible Person4 Name: " + keyResponsiblePerson4Name);
            delay(100);

            sendReadCommand(VP_KEY_RESPONSIBLE_PERSON4_CONTACT, 0x28);
            delay(100);
            String tempkeyResponsiblePerson4Contact = tempreadResponse();
            delay(100);
            String pc4emoverHeaders = removeFirst7Bytes(tempkeyResponsiblePerson4Contact);
            String pc4extractedData = extractDataBeforeMarker(pc4emoverHeaders, "ffff");
            keyResponsiblePerson4Contact = hexToString(pc4extractedData);
            Serial.println("Key Responsible Person4 Contact: " + keyResponsiblePerson4Contact);
            delay(100);

            sendReadCommand(VP_LOCAL_FIRE_DEPARTMENT_NAME, 0x28);
            delay(100);
            String templocalFireDepartmentName = tempreadResponse();
            delay(100);
            String lfnemoverHeaders = removeFirst7Bytes(templocalFireDepartmentName);
            String lfnextractedData = extractDataBeforeMarker(lfnemoverHeaders, "ffff");
            localFireDepartmentName = hexToString(lfnextractedData);
            Serial.println("Local Fire Department Name: " + localFireDepartmentName);
            delay(100);

            sendReadCommand(VP_LOCAL_FIRE_DEPARTMENT_CONTACT, 0x28);
            delay(100);
            String templocalFireDepartmentContact = tempreadResponse();
            delay(100);
            String lfcremoverHeaders = removeFirst7Bytes(templocalFireDepartmentContact);
            String lfcextractedData = extractDataBeforeMarker(lfcremoverHeaders, "ffff");
            localFireDepartmentContact = hexToString(lfcextractedData);
            Serial.println("Local Fire Department Contact: " + localFireDepartmentContact);
            delay(100);

            if ((keyResponsiblePerson4Name == "") || (keyResponsiblePerson4Contact == "") || (localFireDepartmentName == "") || (localFireDepartmentContact == ""))
            {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else
            {
                companyDetailsFlag = true;
            }
        }

        else if (containsPattern(companyDetails, companyDetails_page3_back))
        {
            pageSwitch(COMPANY_DETAILS_PAGE2);
            Serial.println("Page Switched");
            delay(5);
        }

        if (companyDetailsFlag)
            break;
    }
    if (companyDetailsFlag)
    {
        companyDetailsFlag = false;
        pageSwitch(COMPANY_MANUFACTURE_DETAILS);
        delay(5);
        manufactureDetails();
    }

    Serial.println("companyDetails completed");
}

void manufactureDetails()
{
    Serial.println("manufactureDetails Started");

    while (true)
    {
        resetVP(VP_MANUFACTURING_DETAILS);
        delay(100);
        sendReadCommand(VP_MANUFACTURING_DETAILS, 0x1);
        delay(100);
        String manufacturerDetails = tempreadResponse();
        Serial.println("Manufacturing Details :" + manufacturerDetails);

        if (containsPattern(manufacturerDetails, Manufacturing_Details_Upload))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            delay(100);
            sendReadCommand(VP_MANUFACTURE_NAME, 0x28);
            delay(100);
            String tempmanufacturerName = tempreadResponse();
            delay(100);
            String mnremoverHeaders = removeFirst7Bytes(tempmanufacturerName);
            String mncextractedData = extractDataBeforeMarker(mnremoverHeaders, "ffff");
            manufacturerName = hexToString(mncextractedData);
            Serial.println("Manufacturer Name: " + manufacturerName);

            delay(100);
            sendReadCommand(VP_MANUFACTURE_CONTACT, 0x28);
            delay(100);
            String tempmanufacturerContact = tempreadResponse();
            delay(100);
            String mcremoverHeaders = removeFirst7Bytes(tempmanufacturerContact);
            String mccextractedData = extractDataBeforeMarker(mcremoverHeaders, "ffff");
            manufacturerContact = hexToString(mccextractedData);
            Serial.println("Manufacturer Contact: " + manufacturerContact);

            delay(100);
            sendReadCommand(VP_MANUFACTURE_EMAIL, 0x28);
            delay(100);
            String tempmanufacturerEmail = tempreadResponse();
            delay(100);
            String meremoverHeaders = removeFirst7Bytes(tempmanufacturerEmail);
            String meextractedData = extractDataBeforeMarker(meremoverHeaders, "ffff");
            manufacturerEmail = hexToString(meextractedData);
            Serial.println("Manufacturer Email: " + manufacturerEmail);

            delay(100);
            sendReadCommand(VP_MANUFACTURE_DATE, 0x28);
            delay(100);
            String tempdateOfManufacture = tempreadResponse();
            delay(100);
            String dmremoverHeaders = removeFirst7Bytes(tempdateOfManufacture);
            String dmcextractedData = extractDataBeforeMarker(dmremoverHeaders, "ffff");
            dateOfManufacture = hexToString(dmcextractedData);
            Serial.println("Manufacturer Date: " + dateOfManufacture);

            delay(100);
            sendReadCommand(VP_MANUFACTURE_SERIAL_N0, 0x28);
            delay(100);
            String tempserialNumber = tempreadResponse();
            delay(100);
            String snremoverHeaders = removeFirst7Bytes(tempserialNumber);
            String sncextractedData = extractDataBeforeMarker(snremoverHeaders, "ffff");
            serialNumber = hexToString(sncextractedData);
            Serial.println("Manufacturer Serial No: " + serialNumber);

            if ((manufacturerName == "") || (manufacturerContact == "") || (manufacturerEmail == "") || (dateOfManufacture == "") || (serialNumber == ""))
            {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else
            {
                companyManufacturerDetails = true;
            }
        }

        if (companyManufacturerDetails)
            break;

        else if (containsPattern(manufacturerDetails, Manufacturing_Details_Back))
        {
            companyManufacturerDetailsBack = true;
            pageSwitch(COMPANY_DETAILS_PAGE3);
            Serial.println("Page Switched");
            delay(5);
            break;
        }
    }

    if (companyManufacturerDetailsBack)
    {
        companyManufacturerDetailsBack = false;
        companyDetails();
    }

    else if (companyManufacturerDetails)
    {
        companyManufacturerDetails = false;
        pageSwitch(COMPANY_UNIT_DETAILS);
        delay(5);
        unitDetails();
    }
    Serial.println("manufactureDetails completed");
}

void unitDetails()
{
    Serial.println("unitDetails Started");

    while (true)
    {
        resetVP(VP_UNIT_DONE);
        delay(100);
        sendReadCommand(VP_UNIT_DONE, 0x1);
        delay(100);
        String unitDetails = tempreadResponse();
        Serial.println("Unit Details :" + unitDetails);

        if (containsPattern(unitDetails, Unit_Details_Upload))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            delay(100);
            sendReadCommand(VP_LOCATION_OF_UNIT, 0x28);
            delay(100);
            String templocationOfUnit = tempreadResponse();
            delay(100);
            String luremoverHeaders = removeFirst7Bytes(templocationOfUnit);
            String lucextractedData = extractDataBeforeMarker(luremoverHeaders, "ffff");
            locationOfUnit = hexToString(lucextractedData);
            Serial.println("Location of Unit: " + locationOfUnit);

            delay(100);
            sendReadCommand(VP_ASSIGNED_UNIT_NUMBER, 0x28);
            delay(100);
            String tempassignedUnitNumber = tempreadResponse();
            delay(100);
            String auremoverHeaders = removeFirst7Bytes(tempassignedUnitNumber);
            String aucextractedData = extractDataBeforeMarker(auremoverHeaders, "ffff");
            assignedUnitNumber = hexToString(aucextractedData);
            Serial.println("Assigned Unit Number: " + assignedUnitNumber);

            delay(100);
            sendReadCommand(VP_DATE_OF_UNIT_INSTALLATION, 0x28);
            delay(100);
            String tempdateOfUnitInstallation = tempreadResponse();
            delay(100);
            String duiremoverHeaders = removeFirst7Bytes(tempdateOfUnitInstallation);
            String duicextractedData = extractDataBeforeMarker(duiremoverHeaders, "ffff");
            dateOfUnitInstallation = hexToString(duicextractedData);
            Serial.println("Unit Installation Date: " + dateOfUnitInstallation);

            delay(100);
            sendReadCommand(VP_UNIT_INSTALLER, 0x28);
            delay(100);
            String tempunitInstaller = tempreadResponse();
            delay(100);
            String uiremoverHeaders = removeFirst7Bytes(tempunitInstaller);
            String uicextractedData = extractDataBeforeMarker(uiremoverHeaders, "ffff");
            unitInstaller = hexToString(uicextractedData);
            Serial.println("Unit Installer: " + unitInstaller);

            delay(100);
            sendReadCommand(VP_UNIT_CONTACT_DETAILS, 0x28);
            delay(100);
            String tempunitContactDetails = tempreadResponse();
            delay(100);
            String ucdremoverHeaders = removeFirst7Bytes(tempunitContactDetails);
            String ucdcextractedData = extractDataBeforeMarker(ucdremoverHeaders, "ffff");
            unitContactDetails = hexToString(ucdcextractedData);
            Serial.println("Unit contact Details: " + unitContactDetails);

            delay(100);
            sendReadCommand(VP_UNIT_IP_ADDRESS, 0x28);
            delay(100);
            String tempipAddress = tempreadResponse();
            delay(100);
            String ipremoverHeaders = removeFirst7Bytes(tempipAddress);
            String ipdcextractedData = extractDataBeforeMarker(ipremoverHeaders, "ffff");
            ipAddress = hexToString(ipdcextractedData);
            Serial.println("Unit IP Address: " + ipAddress);

            if ((locationOfUnit == "") || (assignedUnitNumber == "") || (dateOfUnitInstallation == "") || (unitInstaller == "") || (unitContactDetails == "") || (ipAddress == ""))
            {
                String Status = "Please Enter Details";
                showMessage(clientLoginStatus, Status);
                delay(1000);
                resetVP(clientLoginStatus);

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched
            }
            else
            {
                UnitDetailsFlag = true;
            }
        }

        if (UnitDetailsFlag)
            break;

        else if (containsPattern(unitDetails, Unit_Details_Back))
        {
            unitDetailsBack = true;
            pageSwitch(COMPANY_MANUFACTURE_DETAILS);
            Serial.println("Page Switched");
            delay(5);
            break;
        }
    }

    if (unitDetailsBack)
    {
        unitDetailsBack = false;
        manufactureDetails();
    }

    else if (UnitDetailsFlag)
    {
        UnitDetailsFlag = false;
        pageSwitch(DEVICE_DIRECTION_DETAILS_PAGE);
        delay(5);
        devicesDirectionDetails();
    }
    Serial.println("unitDetails completed");
}

void devicesDirectionDetails()
{
    Serial.println("devicesDirectionDetails Started");

    while (true)
    {
        resetVP(VP_DEVICE_DRIVER_RETURN_KEY);
        delay(100);
        sendReadCommand(VP_DEVICE_DRIVER_RETURN_KEY, 0x1);
        delay(100);
        String arrowIndication = tempreadResponse();
        Serial.println("Arrow Details :" + arrowIndication);

        if (containsPattern(arrowIndication, Left_Arrow_Indication))
        {
            Serial.println("Left Side Move");
            arrowFlags = true;
        }

        else if (containsPattern(arrowIndication, Right_Arrow_Indication))
        {
            Serial.println("Right Side Move");
            arrowFlags = true;
        }

        else if (containsPattern(arrowIndication, Arrow_Details_Back))
        {
            ArrowDetailsBack = true;
            pageSwitch(COMPANY_UNIT_DETAILS);
            Serial.println("Page Switched");
            delay(5);
            break;
        }
        if (arrowFlags)
            break;
    }

    if (ArrowDetailsBack)
    {
        ArrowDetailsBack = false;
        unitDetails();
    }

    else if (arrowFlags)
    {
        arrowFlags = false;
        pageSwitch(DEVICE_CONFIGURED_SUCCESSFULLY);
    }
    Serial.println("devicesDirectionDetails completed");
}

void slideShow()
{
    Serial.println("slideShow Started");

    while (slideShowFlag)
    {
        vTaskSuspend(xHandledatetime); // Suspend date time task while slideShow

        String ack = tempreadResponse();

        pageSwitch(FYREBOXLOGO);
        // Record the start time
        unsigned long startTime = millis();
        // Check if 5 seconds have passed
        while (millis() - startTime < 5000)
        {

            ack = tempreadResponse();

            if (ack == "5aa53824f4b")
            {
                ack = "";
            }

            Serial.print("Acknowledgment from LCD: ");
            Serial.println(ack);

            if (checkLastFourDigitsMatch(ack, Home_Screen))
            {
                pageSwitch(HOME_PAGE);
                Serial.println("Page Switched");
                delay(5);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
            delay(100);
        }
        if (!slideShowFlag)
        {
            break;
        }

        pageSwitch(CLIENT_LOGO);

        // Record the start time
        startTime = millis();
        // Check if 10 seconds have passed
        while (millis() - startTime < 10000)
        {

            ack = tempreadResponse();

            if (ack == "5aa53824f4b")
            {
                ack = "";
            }

            Serial.print("Acknowledgment from LCD: ");
            Serial.println(ack);

            if (checkLastFourDigitsMatch(ack, Home_Screen))
            {
                pageSwitch(HOME_PAGE);
                Serial.println("Page Switched");
                delay(5);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
            delay(100);
        }
        if (!slideShowFlag)
        {
            break;
        }

        pageSwitch(EVACUATION_PROCEDURE_PAGE);

        // Record the start time
        startTime = millis();
        // Check if 15 seconds have passed
        while (millis() - startTime < 15000)
        {

            ack = tempreadResponse();

            if (ack == "5aa53824f4b")
            {
                ack = "";
            }

            Serial.print("Acknowledgment from LCD: ");
            Serial.println(ack);

            if (checkLastFourDigitsMatch(ack, Home_Screen))
            {
                pageSwitch(HOME_PAGE);
                Serial.println("Page Switched");
                delay(5);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
            delay(100);
        }
        if (!slideShowFlag)
        {
            break;
        }

        /*
        pageSwitch(0x0019);
        delay(1000);
        ack = tempreadResponse();

        if(ack == "5aa53824f4b")
        {
            ack = "";
        }

        Serial.print("Acknowledgment from LCD: ");
        Serial.println(ack);

        if (checkLastFourDigitsMatch(ack, Home_Screen))
        {
            pageSwitch(HOME_PAGE);
            Serial.println("Page Switched");
            delay(5);
            vTaskResume(xHandledatetime); // Resume if touch is detected
            break;
        }*/
    }
    Serial.println("slideShow completed");
}

void slideShow_EvacuationDiagrams()
{
    Serial.println("slideShow Started");

    while (slideShowFlag)
    {
        vTaskSuspend(xHandledatetime); // Suspend date time task while slideShow

        String ack = tempreadResponse();

        pageSwitch(SITEMAP_PAGE);
        // Record the start time
        unsigned long startTime = millis();
        // Check if 30 seconds have passed
        while (millis() - startTime < 30000)
        {

            ack = tempreadResponse();

            if (ack == "5aa53824f4b")
            {
                ack = "";
            }

            Serial.print("Acknowledgment from LCD: ");
            Serial.println(ack);

            if (checkLastFourDigitsMatch(ack, Home_Screen))
            {
                pageSwitch(HOME_PAGE);
                Serial.println("Page Switched");
                delay(5);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
            delay(100);
        }
        if (!slideShowFlag)
        {
            break;
        }

        pageSwitch(EVACUATION_PROCEDURE_PAGE);

        // Record the start time
        startTime = millis();
        // Check if 15 seconds have passed
        while (millis() - startTime < 15000)
        {

            ack = tempreadResponse();

            if (ack == "5aa53824f4b")
            {
                ack = "";
            }

            Serial.print("Acknowledgment from LCD: ");
            Serial.println(ack);

            if (checkLastFourDigitsMatch(ack, Home_Screen))
            {
                pageSwitch(HOME_PAGE);
                Serial.println("Page Switched");
                delay(5);
                slideShowFlag = false;
                vTaskResume(xHandledatetime); // Resume if touch is detected
                break;
            }
            delay(100);
        }
        if (!slideShowFlag)
        {
            break;
        }
    }
    Serial.println("slideShow completed");
}

void homepageTasks(void *parameter)
{
    Serial.println("homepageTasks Started");

    while (true)
    {
        // Reset the last activity time
        lastActivityTime = millis();

        checkData = tempreadResponse();

        // unwanted lcd response
        if (checkData == "5aa53824f4b")
        {
            checkData = "";
        }
        Serial.println("Data in homepageTasks:" + checkData);
        delay(100);

        // Handle site evacuation button on top priority
        // Read the state of the push button
        int reading = digitalRead(siteEvacuation_buttonPin);

        // If the switch changed, due to noise or pressing:
        if (reading != lastButtonState)
        {
            // reset the debouncing timer
            lastDebounceTime = millis();
        }

        if ((millis() - lastDebounceTime) > debounceDelay)
        {
            // Whatever the reading is at, it's been there for longer than the debounce
            // delay, so take it as the actual current state:
            if (reading != buttonState)
            {
                buttonState = reading;

                // Only toggle the evacuation state if the new button state is LOW
                if (buttonState == LOW)
                {
                    evacuationActive = !evacuationActive;

                    if (evacuationActive)
                    {
                        Serial.println("Start");
                        // start leds
                        FillSolidLeds(SideLEDs, NUM_LEDS_RGB2, CRGB::Red); // Red when Activation
                        delay(1000);
                        FillSolidLeds(SideLEDs, NUM_LEDS_RGB2, CRGB::Black);

                        startSiren();
                        audio.connecttoFS(SD, filename); // SD card file
                        audio.setFileLoop(true);         // Set file to loop
                        // send message to other nodes to activate

                        // Start slideShow
                        slideShowFlag = true;
                        slideShow_EvacuationDiagrams();
                    }
                    else
                    {
                        Serial.println("Stop evacuation");
                        stopSiren(); // Stop siren
                        audio.stopSong();
                        audio.setFileLoop(false); // Reset file to loop
                        // stop leds
                        // send message to other nodes to stop
                    }
                }
            }
        }

        // Save the reading. Next time through the loop, it'll be the lastButtonState:
        lastButtonState = reading;

        // Small delay to stabilize the loop
        delay(10);

        // Show/Hide Menu & select between Menu functions
        if (containsPattern(checkData, "6211"))
        {
            Serial.println("Data from home screen menu");

            if (containsPattern(checkData, Home_Screen_Menu))
            {
                pageSwitch(MENU_PAGE);
                Serial.println("Page Switched");
                delay(5);
            }

            else if (containsPattern(checkData, Menu_Home_Screen))
            {
                pageSwitch(HOME_PAGE);
                Serial.println("Page Switched");
                delay(5);
            }

            else if (containsPattern(checkData, batteryCalc))
            {
                // pageSwitch(HOME_PAGE);
                // Serial.println("Page Switched");
                delay(5);
            }

            // Handle Logout
            else if (containsPattern(checkData, logout))
            {
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
                Serial.println("Page Switched");
                delay(5);

                Serial.println("Suspend date time Task");
                vTaskSuspend(xHandledatetime); // Not needed while login
                delay(100);

                configureLogin();

                Serial.println("Resume date time Task");
                vTaskResume(xHandledatetime); // Login Completed
                delay(100);

                pageSwitch(HOME_PAGE);
            }
        }

        // local map
        else if (containsPattern(checkData, "6212"))
        {
            pageSwitch(LOCALMAP_PAGE);
            Serial.println("local map");
            delay(5);
        }

        // Site map
        else if (containsPattern(checkData, "6213"))
        {
            // pageSwitch(SITEMAP_PAGE);
            pageSwitch(LOCALMAP_PAGE); // only for testing
            Serial.println("Site map");
            delay(5);
        }

        // Show Units lists
        else if (containsPattern(checkData, "6214"))
        {
            pageSwitch(UNITSLISTS_PAGE);
            Serial.println("Page Switched");
            delay(5);

            displayFyreBoxUnitList();

            slideShowFlag = false;
            FyreBoxUnitListFlag = true;

            FyreBoxUnitList();
        }

        // Show Report
        else if (containsPattern(checkData, "6215"))
        {
            Serial.println("Data from home screen report");

            if (containsPattern(checkData, Home_Screen_Report))
            {
                pageSwitch(SHOW_REPORT_PAGE);
                Serial.println("Page Switched");
                delay(5);

                slideShowFlag = false;
                displayIconsFlag = true;

                displayIcons();
            }
        }

        // Self Test
        else if (containsPattern(checkData, "6216"))
        {
            Serial.println("Self Test");
            audio.connecttoFS(SD, filename2); // SD card file
            audio.loop();
            delay(1000);
        }

        // Show Checklist
        else if (containsPattern(checkData, "6217") && !dataEnteredtoday)
        {
            Serial.println("Data from home screen checklist");

            lastDataEntryEEPROM = EEPROM.read(EEPROMAddress);
            Serial.print("Last data entry week number: ");
            Serial.println(lastDataEntryEEPROM);

            if (weekByYear == lastDataEntryEEPROM)
            {
                dataEnteredtoday = false;
                Serial.println("Data Entered today");
            }

            else if (weekByYear > lastDataEntryEEPROM)
            {
                weekElapsed = true;
                Serial.println("Week Elapsed");
            }

            else
            {
                Serial.println("Next data will be received in next week");
            }

            if (containsPattern(checkData, Home_Screen_Checklist))
            {
                pageSwitch(CHECKLISTPAGE1);
                Serial.println("Page Switched");
                delay(5);

                slideShowFlag = false;
                checkBoxFlag = true;

                CheckBoxes();
            }
        }

        // Checklist Data is Received
        else if (containsPattern(checkData, "6217") && dataEnteredtoday)
        {
            Serial.println("Data is Received, next data will be received in next week");
        }

        // handle site evacuation button on LCD
        else if (containsPattern(checkData, "6218"))
        {
            if (containsPattern(checkData, "100"))
            {
                Serial.println("activate site evacuation");

                // start leds
                FillSolidLeds(SideLEDs, NUM_LEDS_RGB2, CRGB::Red); // Red when Activation
                delay(1000);
                FillSolidLeds(SideLEDs, NUM_LEDS_RGB2, CRGB::Black);

                startSiren();
                audio.connecttoFS(SD, filename); // SD card file
                audio.setFileLoop(true);         // Set file to loop
                // send message to other nodes to activate

                // Start slideShow
                slideShowFlag = true;
                slideShow_EvacuationDiagrams();
            }

            else if (containsPattern(checkData, "101"))
            {
                Serial.println("Stop evacuation");
                stopSiren(); // Stop siren
                audio.stopSong();
                audio.setFileLoop(false); // Reset file to loop
                // stop leds
                // send message to other nodes to stop
            }
        }

        // Start Slide show
        else if (containsPattern(checkData, "6219"))
        {
            Serial.println("Start slideShow");

            if (containsPattern(checkData, Home_Screen_Back))
            {
                // Start slideShow
                slideShowFlag = true;
                slideShow();
            }
        }

        // return keycode on site map
        else if (containsPattern(checkData, "6220"))
        {
            // currently page switching
        }

        // Check if the idle timeout has elapsed
        else if (millis() - lastActivityTime >= idleTimeout)
        {
            Serial.println("Idle timeout has elapsed");
            // Start slideShow
            slideShowFlag = true;
            slideShow();

            // Reset the last activity time
            lastActivityTime = millis();
        }
    }

    Serial.println("homepageTasks completed");
    Serial.println("homepageTasks Suspended");
    vTaskSuspend(xHandlehomepage); // Suspend the task
}

void CheckBoxes()
{
    Serial.println("CheckBoxes Started");

    while (checkBoxFlag)
    {
        delay(100);
        String checkBoxesData = tempreadResponse();

        if (checkBoxesData == "5aa53824f4b")
        {
            checkBoxesData = "";
        }

        Serial.println("Data in checkbox: " + checkBoxesData);

        if (checkLastFourDigitsMatch(checkBoxesData, geticonPage1))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            // String message = "Please wait while fetching Data";
            // showMessage(clientLoginStatus, message);

            sendReadCommand(CONTROL_FUNCTION, 0x1);
            delay(100);
            String tempControlFunction = tempreadResponse();
            delay(100);
            if (tempControlFunction == "5aa53824f4b")
                tempControlFunction = "";
            controlFunction = removeFirst6Bytes(tempControlFunction);
            Serial.println("Company Function: " + controlFunction);
            delay(100);

            sendReadCommand(SPEAKER_ACTIVATE, 0x1);
            delay(100);
            String tempSpeakerActivate = tempreadResponse();
            delay(100);
            if (tempSpeakerActivate == "5aa53824f4b")
                tempSpeakerActivate = "";
            speakerActivate = removeFirst6Bytes(tempSpeakerActivate);
            Serial.println("Speaker Activate: " + speakerActivate);
            delay(100);

            sendReadCommand(FIREMAN_ACTIVATE_BOX, 0x1);
            delay(100);
            String tempfiremanActivateBox = tempreadResponse();
            delay(100);
            if (tempfiremanActivateBox == "5aa53824f4b")
                tempfiremanActivateBox = "";
            firemanActivateBox = removeFirst6Bytes(tempfiremanActivateBox);
            Serial.println("Fireman Activate Box: " + firemanActivateBox);
            delay(100);

            sendReadCommand(BELL_RING_SYSTEM_ACTIVATION, 0x1);
            delay(100);
            String tempbellRingSystemActivation = tempreadResponse();
            delay(100);
            if (tempbellRingSystemActivation == "5aa53824f4b")
                tempbellRingSystemActivation = "";
            bellRingSystemActivation = removeFirst6Bytes(tempbellRingSystemActivation);
            Serial.println("Bell Ring System Activation: " + bellRingSystemActivation);
            delay(100);

            sendReadCommand(BATTERY_HEALTH, 0x1);
            delay(100);
            String tempbatteryHealth = tempreadResponse();
            delay(100);
            if (tempbatteryHealth == "5aa53824f4b")
                tempbatteryHealth = "";
            batteryHealth = removeFirst6Bytes(tempbatteryHealth);
            Serial.println("Battery Health: " + batteryHealth);
            delay(100);

            sendReadCommand(LED_LIGHT_ON_WHITE, 0x1);
            delay(100);
            String templedLightOnWhite = tempreadResponse();
            delay(100);
            if (templedLightOnWhite == "5aa53824f4b")
                templedLightOnWhite = "";
            ledLightOnWhite = removeFirst6Bytes(templedLightOnWhite);
            Serial.println("Led Light ON (White): " + ledLightOnWhite);
            delay(100);

            if ((controlFunction != "0") && (speakerActivate != "0") && (firemanActivateBox != "0") && (bellRingSystemActivation != "0") && (batteryHealth != "0") && (ledLightOnWhite != "0"))
            {
                vTaskResume(xHandledatetime); // Resume date time task after data is fetched

                pageSwitch(CHECKLISTPAGE2);
                Serial.println("Page Switched");
                delay(5);
            }
            else
            {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage1))
        {
            pageSwitch(HOME_PAGE);
            Serial.println("Page Switched");
            delay(5);
            Serial.println("Exit command called");
            break;
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, geticonPage2))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            // String message = "Please wait while fetching Data";
            // showMessage(clientLoginStatus, message);

            delay(100);
            sendReadCommand(LED_RED_ACTIVATION, 0x1);
            delay(100);
            String templedRedActivation = tempreadResponse();
            delay(100);
            if (templedRedActivation == "5aa53824f4b")
                templedRedActivation = "";
            ledRedActivation = removeFirst6Bytes(templedRedActivation);
            Serial.println("Led Red Activation: " + ledRedActivation);
            delay(100);

            sendReadCommand(SMS_RECEIVED_FYREBOX_ACTIVATED, 0x1);
            delay(100);
            String tempsmsReceivedFyreboxActivated = tempreadResponse();
            delay(100);
            if (tempsmsReceivedFyreboxActivated == "5aa53824f4b")
                tempsmsReceivedFyreboxActivated = "";
            smsReceivedFyreboxActivated = removeFirst6Bytes(tempsmsReceivedFyreboxActivated);
            Serial.println("SMS Received: " + smsReceivedFyreboxActivated);
            delay(100);

            sendReadCommand(LCD_SCREEN_WORK, 0x1);
            delay(100);
            String templcdScreenWork = tempreadResponse();
            delay(100);
            if (templcdScreenWork == "5aa53824f4b")
                templcdScreenWork = "";
            lcdScreenWork = removeFirst6Bytes(templcdScreenWork);
            Serial.println("LCD Screen Work: " + lcdScreenWork);
            delay(100);

            sendReadCommand(SYSTEM_ACTIVATE_WEEKLY_SELF_TEST, 0x1);
            delay(100);
            String tempsystemActivateWeeklySelfTest = tempreadResponse();
            delay(100);
            if (tempsystemActivateWeeklySelfTest == "5aa53824f4b")
                tempsystemActivateWeeklySelfTest = "";
            systemActivateWeeklySelfTest = removeFirst6Bytes(tempsystemActivateWeeklySelfTest);
            Serial.println("System Activate Weekly Self Test: " + systemActivateWeeklySelfTest);
            delay(100);

            sendReadCommand(EVACUATION_DIAGRAM, 0x1);
            delay(100);
            String tempevacuatioDiagram = tempreadResponse();
            delay(100);
            if (tempevacuatioDiagram == "5aa53824f4b")
                tempevacuatioDiagram = "";
            evacuatioDiagram = removeFirst6Bytes(tempevacuatioDiagram);
            Serial.println("Evacuation Diagram: " + evacuatioDiagram);
            delay(100);

            sendReadCommand(ARROW_WORKING, 0x1);
            delay(100);
            String temparrowWorking = tempreadResponse();
            delay(100);
            if (temparrowWorking == "5aa53824f4b")
                temparrowWorking = "";
            arrowWorking = removeFirst6Bytes(temparrowWorking);
            Serial.println("Arrow Working: " + arrowWorking);
            delay(100);

            if ((ledRedActivation != "0") && (smsReceivedFyreboxActivated != "0") && (lcdScreenWork != "0") && (systemActivateWeeklySelfTest != "0") && (evacuatioDiagram != "0") && (arrowWorking != "0"))
            {
                vTaskResume(xHandledatetime); // Resume date time task after data is fetched

                pageSwitch(CHECKLISTPAGE3);
                Serial.println("Page Switched");
                delay(5);
            }
            else
            {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage2))
        {
            pageSwitch(CHECKLISTPAGE1);
            Serial.println("Page Switched");
            delay(5);
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, geticonPage3))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            // String message = "Please wait while fetching Data";
            // showMessage(clientLoginStatus, message);

            sendReadCommand(PERMANENT_POWER, 0x1);
            delay(100);
            String temppermanentPower = tempreadResponse();
            delay(100);
            if (temppermanentPower == "5aa53824f4b")
                temppermanentPower = "";
            permanentPower = removeFirst6Bytes(temppermanentPower);
            Serial.println("Permanenet Power: " + permanentPower);
            delay(100);

            sendReadCommand(ILLUMINATED_SIGNALS_WORKING, 0x1);
            delay(100);
            String tempilluminatedSignalsWorking = tempreadResponse();
            delay(100);
            if (tempilluminatedSignalsWorking == "5aa53824f4b")
                tempilluminatedSignalsWorking = "";
            illuminatedSignalsWorking = removeFirst6Bytes(tempilluminatedSignalsWorking);
            Serial.println("Illuminated Signal Working: " + illuminatedSignalsWorking);
            delay(100);

            sendReadCommand(BATTERIES_REPLACEMENT, 0x1);
            delay(100);
            String tempbatteriesReplacement = tempreadResponse();
            delay(100);
            if (tempbatteriesReplacement == "5aa53824f4b")
                tempbatteriesReplacement = "";
            batteriesReplacement = removeFirst6Bytes(tempbatteriesReplacement);
            Serial.println("Batteries Replacement: " + batteriesReplacement);
            delay(100);

            sendReadCommand(FLASH_SIGN_PANELS, 0x1);
            delay(100);
            String tempflashSignPanel = tempreadResponse();
            delay(100);
            if (tempflashSignPanel == "5aa53824f4b")
                tempflashSignPanel = "";
            flashSignPanel = removeFirst6Bytes(tempflashSignPanel);
            Serial.println("Flash Sign Panel: " + flashSignPanel);
            delay(100);

            if ((permanentPower != "0") && (illuminatedSignalsWorking != "0") && (batteriesReplacement != "0") && (flashSignPanel != "0"))
            {
                vTaskResume(xHandledatetime); // Resume date time task after data is fetched

                pageSwitch(CHECKLISTPAGE4);
                Serial.println("Page Switched");
                delay(5);
            }
            else
            {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage3))
        {
            pageSwitch(CHECKLISTPAGE2);
            Serial.println("Page Switched");
            delay(5);
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, geticonPage4))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            // String message = "Please wait while fetching Data";
            // showMessage(clientLoginStatus, message);

            sendReadCommand(UNIT_SECURED, 0x1);
            delay(100);
            String tempunitSecured = tempreadResponse();
            delay(100);
            if (tempunitSecured == "5aa53824f4b")
                tempunitSecured = "";
            unitSecured = removeFirst6Bytes(tempunitSecured);
            Serial.println("Unit Secured: " + unitSecured);
            delay(100);

            sendReadCommand(FACIA_COMPONENT_SECURED, 0x1);
            delay(100);
            String tempfaciaComponentSecured = tempreadResponse();
            delay(100);
            if (tempfaciaComponentSecured == "5aa53824f4b")
                tempfaciaComponentSecured = "";
            faciaComponentSecured = removeFirst6Bytes(tempfaciaComponentSecured);
            Serial.println("Facia Component Secured: " + faciaComponentSecured);
            delay(100);

            sendReadCommand(EVACUATION_DIAGRAM_UPTODATE, 0x1);
            delay(100);
            String tempevacuationDiagramUptodate = tempreadResponse();
            delay(100);
            if (tempevacuationDiagramUptodate == "5aa53824f4b")
                tempevacuationDiagramUptodate = "";
            evacuationDiagramUptodate = removeFirst6Bytes(tempevacuationDiagramUptodate);
            Serial.println("Evacuation Diagram Up to Date: " + evacuationDiagramUptodate);
            delay(100);

            sendReadCommand(FYREBOX_FREE_OBSTRUCTIONS, 0x1);
            delay(100);
            String tempfyreboxFreeObstructions = tempreadResponse();
            delay(100);
            if (tempfyreboxFreeObstructions == "5aa53824f4b")
                tempfyreboxFreeObstructions = "";
            fyreboxFreeObstructions = removeFirst6Bytes(tempfyreboxFreeObstructions);
            Serial.println("Fyre Box Free Obstructions: " + fyreboxFreeObstructions);
            delay(100);

            sendReadCommand(LOGBOOK_UPTODATE, 0x1);
            delay(100);
            String tempLogbookUptodate = tempreadResponse();
            delay(100);
            if (tempLogbookUptodate == "5aa53824f4b")
                tempLogbookUptodate = "";
            LogbookUptodate = removeFirst6Bytes(tempLogbookUptodate);
            Serial.println("LogBook Up to Date: " + LogbookUptodate);
            delay(100);

            if ((unitSecured != "00") && (faciaComponentSecured != "00") && (evacuationDiagramUptodate != "00") && (fyreboxFreeObstructions != "00") && (LogbookUptodate != "00"))
            {
                vTaskResume(xHandledatetime); // Resume date time task after data is fetched

                pageSwitch(CHECKLISTPAGE5);
                Serial.println("Page Switched");
                delay(5);
            }
            else
            {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage4))
        {
            pageSwitch(CHECKLISTPAGE3);
            Serial.println("Page Switched");
            delay(5);
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, Checklist_Done))
        {
            vTaskSuspend(xHandledatetime); // Suspend date time task while fetching data

            // String message = "Please wait while fetching Data";
            // showMessage(clientLoginStatus, message);

            sendReadCommand(FYREBOX_UNIT_WIPED_CLEAN, 0x1);
            delay(100);
            String tempfyreboxUnitWipedCleaned = tempreadResponse();
            delay(100);
            if (tempfyreboxUnitWipedCleaned == "5aa53824f4b")
                tempfyreboxUnitWipedCleaned = "";
            fyreboxUnitWipedCleaned = removeFirst6Bytes(tempfyreboxUnitWipedCleaned);
            Serial.println("Fyre Box Unit Wiped and Clean: " + fyreboxUnitWipedCleaned);
            delay(100);

            sendReadCommand(ANY_DAMAGE_BOX, 0x1);
            delay(100);
            String tempanyDamageBox = tempreadResponse();
            delay(100);
            if (tempanyDamageBox == "5aa53824f4b")
                tempanyDamageBox = "";
            anyDamageBox = removeFirst6Bytes(tempanyDamageBox);
            Serial.println("Any Damage Box: " + anyDamageBox);
            delay(100);

            sendReadCommand(ANY_RUST_UNIT, 0x1);
            delay(100);
            String tempanyRustUnit = tempreadResponse();
            delay(100);
            if (tempanyRustUnit == "5aa53824f4b")
                tempanyRustUnit = "";
            anyRustUnit = removeFirst6Bytes(tempanyRustUnit);
            Serial.println("Any Rust Unit: " + anyRustUnit);
            delay(100);

            if ((fyreboxUnitWipedCleaned != "00") && (anyDamageBox != "00") && (anyRustUnit != "00"))
            {
                checkBoxFlag = false;
                Serial.println("checkBoxFlag is false ");

                Serial.println("Data entered for this week.");
                dataEnteredtoday = true;
                weekElapsed = false;
                lastDataEntryweekbyYear = weekByYear; // Store the day of the week when data is entered
                delay(100);
                Serial.print("last data entry week: ");
                Serial.println(lastDataEntryweekbyYear);
                EEPROM.write(EEPROMAddress, lastDataEntryweekbyYear);
                EEPROM.commit(); // Commit changes
                /*
                // Only for testing of setDeviceChecklists
                // DynamicJsonDocument doc(1024);
                Serial.println("Inserting checklists in Database ");

                // Device Checklists Parameters
                // controlFunction + speakerActivate + firemanActivateBox + bellRingSystemActivation + batteryHealth + ledLightOnWhite + ledRedActivation
                // + smsReceivedFyreboxActivated + lcdScreenWork + systemActivateWeeklySelfTest + evacuatioDiagram + arrowWorking + permanentPower + illuminatedSignalsWorking +
                // batteriesReplacement + flashSignPanel + unitSecured + faciaComponentSecured + evacuationDiagramUptodate + fyreboxFreeObstructions
                // + LogbookUptodate + fyreboxUnitWipedCleaned + anyDamageBox + anyRustUnit;

                devCheck_deviceId = "1";
                siren = "1";
                switchDevice = "1";
                voltIndicator = "24";
                ledWhite = "1";
                ledRed = "1";
                activationSms = "1";
                signage = "1";
                autoTest = "1";
                testSms = "1";
                power = "1";
                voltLevel = "12";
                battExpiery = "01-01-25";
                deviceInstall = "01-03-24";
                boxState = "Active";
                diagUpdate = "01-02-24";
                deviceObstacle = "Null";
                lastUpdate = "01-02-24";
                deviceClean = "1";
                status = "0";

                String setDeviceChecklists = manufacturerBaseUrl + "operation=" + devCheck_operation + "&device_id=" + devCheck_deviceId + "&siren=" + siren + "&switch=" + switchDevice + "&volt_indicator=" + voltIndicator + "&led_white=" + ledWhite + "&led_red=" + ledRed + "&activation_sms=" + activationSms + "&signage=" + signage + "&auto_test=" + autoTest + "&test_sms=" + testSms + "&power=" + power + "&volt_level=" + voltLevel + "&batt_expiry=" + battExpiery + "&devi_install=" + deviceInstall + "&box_state=" + boxState + "&diag_update=" + diagUpdate + "&devi_obstacle=" + deviceObstacle + "&last_update=" + lastUpdate + "&devi_clean=" + deviceClean + "&status=" + status;

                HTTPClient http;
                delay(5);
                http.begin(setDeviceChecklists); // API URL
                Serial.println("URL " + setDeviceChecklists);
                int httpCode = http.GET();
                Serial.println(httpCode);

                if (httpCode > 0)
                { // Check for the returning code
                    String payload = http.getString();
                    Serial.println(httpCode);
                    Serial.println(payload);
                }
                else
                {
                    Serial.println("Error on HTTP request");
                }
                http.end(); // Free resources
                */

                vTaskResume(xHandledatetime); // Resume date time task after data is fetched

                pageSwitch(HOME_PAGE);
                Serial.println("Page Switched");
                delay(5);
            }
            else
            {
                String checkboxStatus = "Please check all boxes";
                showMessage(clientLoginStatus, checkboxStatus);
                delay(1000);
                resetVP(clientLoginStatus);
            }
        }

        else if (checkLastFourDigitsMatch(checkBoxesData, previousONpage5))
        {
            pageSwitch(CHECKLISTPAGE4);
            Serial.println("Page Switched");
            delay(5);
        }
    }
    Serial.println("CheckBoxes completed");
}

void displayIcons()
{
    Serial.println("displayIcons Started");

    while (displayIconsFlag)
    {
        String LCD_RESPONSE = tempreadResponse();

        if (containsPattern(LCD_RESPONSE, Report_Home_Screen))
        {
            displayIconsFlag = false;
            Serial.println("displayIconsFlag is false ");

            pageSwitch(HOME_PAGE);
            Serial.println("Page Switched");
            delay(5);
        }

        String keyValue = extractKeycode(LCD_RESPONSE);
        // Serial.print("Returned key value in string: "); Serial.println(keyValue);

        int keycode = 0;
        keycode = keyValue.toInt();
        // Serial.print("Returned key value in int: ");Serial.println(keycode);

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
        // Serial.print("VP address in string: "); Serial.println(vp);
        int vpINT = 0;
        vpINT = vp.toInt();
        // Serial.print("VP address in int: "); Serial.println(vpINT);

        if (vpINT == showWeek_page1_VP) // for page 1
        {
            if (keycode == Next_week)
            {
                resetVP(basicGraphic_page1);
                delay(100);
                // sendIconcommand(basicGraphic_page1, icon_5, icon_4, icon_3, icon_2, icon_1, icon_0);
                delay(5);
            }
            else if (keycode == Prev_week)
            {
                resetVP(basicGraphic_page1);
                delay(100);
                sendIconcommand(basicGraphic_page1, icon_0, icon_1, icon_2, icon_3, icon_4, icon_5);
                delay(5);
            }
        }

        else if (vpINT == showWeek_page2_VP) // for page 2
        {
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

        else if (vpINT == showWeek_page3_VP) // for page 3
        {
            if (keycode == Next_week)
            {
                resetVP(basicGraphic_page3);
                delay(100);
                // sendIconcommand(basicGraphic_page3, icon_15, icon_14, icon_13, icon_12, 0x04, 0x04);
                delay(5);
            }
            else if (keycode == Prev_week)
            {
                resetVP(basicGraphic_page3);
                delay(100);
                sendIconcommand(basicGraphic_page3, icon_12, icon_13, icon_14, icon_15, 0x04, 0x04);
                delay(5);
            }
        }

        else if (vpINT == showWeek_page4_VP) // for page 4
        {
            if (keycode == Next_week)
            {
                resetVP(basicGraphic_page4);
                delay(100);
                // sendIconcommand(basicGraphic_page4, icon_21, icon_20, icon_19, icon_18, icon_17, icon_16);
                delay(5);
            }
            else if (keycode == Prev_week)
            {
                resetVP(basicGraphic_page4);
                delay(100);
                sendIconcommand(basicGraphic_page4, icon_16, icon_17, icon_18, icon_19, icon_20, icon_21);
                delay(5);
            }
        }

        else if (vpINT == showWeek_page5_VP) // for page 5
        {
            if (keycode == Next_week)
            {
                resetVP(basicGraphic_page5);
                delay(100);
                // sendIconcommand(basicGraphic_page5, icon_24, icon_23, icon_22, 0x04, 0x04, 0x04);
                delay(5);
            }
            else if (keycode == Prev_week)
            {
                resetVP(basicGraphic_page5);
                delay(100);
                sendIconcommand(basicGraphic_page5, icon_22, icon_23, icon_24, 0x04, 0x04, 0x04);
                delay(5);
            }
        }

        switch (keycode)
        {
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
            Serial.println("No previous pages available");
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
    Serial.println("displayIcons completed");
}

void saveClientCredentials(const String &username, const String &password)
{
    preferences.putString("client_username", username);
    preferences.putString("client_password", password);
}

void removeClientCredentials()
{
    preferences.putString("client_username", " ");
    preferences.putString("client_password", " ");
}

void saveAdminCredentials(const String &username, const String &password)
{
    preferences.putString("admin_username", username);
    preferences.putString("admin_password", password);
}

void removeAdminCredentials()
{
    preferences.putString("admin_username", " ");
    preferences.putString("admin_password", " ");
}

void saveInternetCredentials(const String &ssid, const String &password)
{
    preferences.putString("internetSSID", ssid);
    preferences.putString("internetPass", password);
}

bool RememberIcon(uint16_t rememberLogin)
{
    delay(100);
    sendReadCommand(rememberLogin, 0x1);
    delay(100);
    String iconRead = tempreadResponse();
    Serial.println("Read remember Icon:" + iconRead);

    if (checkLast3DigitsMatch(iconRead, "101"))
    {
        Serial.println("Remember me is true");
        return true;
    }
    else
    {
        Serial.println("Remember me is false");
        return false;
    }
}

void showMessage(uint16_t VP_ADDRESS, String displaymessage)
{
    resetVP(VP_ADDRESS);
    Serial.println(displaymessage);
    String StatusBytes = toHexString(displaymessage);
    delay(100);
    writeString(VP_ADDRESS, StatusBytes);
}

void displayFyreBoxUnitList()
{

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
    for (const auto &status : nodeStatuses)
    {
        // Convert nodeId to string and then to hexadecimal
        String nodeIdString = String(status.nodeId);
        String message = "Node ID " + nodeIdString;
        String nodeBytes = toHexString(message);
        delay(10);

        if (status.isActive)
        {
            // Write to the active device address for this node
            if (activeIndex < Text_Active_Devices_Size)
            {
                writeString(Text_Active_Devices[activeIndex], nodeBytes);
                activeIndex++;
            }
        }
        else
        {
            // Write to the inactive device address for this node
            if (inactiveIndex < Text_Inactive_Devices_Size)
            {
                writeString(Text_Inactive_Devices[inactiveIndex], nodeBytes);
                inactiveIndex++;
            }
        }
    }
}

void FyreBoxUnitList()
{

    Serial.println("FyreBoxUnitsLists started");

    while (FyreBoxUnitListFlag)
    {

        String LCD_RESPONSE = tempreadResponse();

        if (containsPattern(LCD_RESPONSE, Report_Home_Screen))
        {
            FyreBoxUnitListFlag = false;
            Serial.println("FyreBoxUnitListFlag is false ");

            pageSwitch(HOME_PAGE);
            Serial.println("Page Switched");
            delay(5);
        }

        else if (containsPattern(LCD_RESPONSE, VP_ReturnKeyCode))
        {
            while (true)
            {
                if (containsPattern(LCD_RESPONSE, ReturnKeyCode_Active_Next))
                {
                    Serial.println("ReturnKeyCode_Active_Next");
                    break;
                }
                else if (containsPattern(LCD_RESPONSE, ReturnKeyCode_Active_Prev))
                {
                    Serial.println("ReturnKeyCode_Active_Prev");
                    break;
                }
                else if (containsPattern(LCD_RESPONSE, ReturnKeyCode_Inactive_Next))
                {
                    Serial.println("ReturnKeyCode_Inactive_Next");
                    break;
                }
                else if (containsPattern(LCD_RESPONSE, ReturnKeyCode_Inactive_Prev))
                {
                    Serial.println("ReturnKeyCode_Inactive_Prev");
                    break;
                }
            }
        }
        // static unsigned long lastUpdateTime = 0;
        // unsigned long currentMillis = millis();

        // if(currentMillis - lastUpdateTime > 60000)
        // {
        //     displayFyreBoxUnitList();
        //     lastUpdateTime = currentMillis;
        // }
    }
    Serial.println("FyreBoxUnitsLists completed");
}

// For LoRa Mesh
void LoRatask(void *parameter)
{
    Serial.println("LoRatask Started");
    for (;;)
    {
        static unsigned long lastBroadcastTime = 0;
        static unsigned long lastCheckTime = 0;
        static unsigned long lastStatusPrintTime = 0;
        unsigned long currentMillis = millis();

        if (currentMillis - lastBroadcastTime > 30000)
        { // Every 30 seconds
            broadcastPresence();
            lastBroadcastTime = currentMillis;
        }

        listenForNodes();

        if (currentMillis - lastCheckTime > 10000)
        { // Every 10 seconds
            checkNodeActivity();
            lastCheckTime = currentMillis;
        }

        if (currentMillis - lastStatusPrintTime > 60000)
        { // Every 60 seconds
            // printNodeStatuses();  // Print the statuses of all nodes
            printNetworkStats();
            displayFyreBoxUnitList();
            lastStatusPrintTime = currentMillis;
        }
    }
    Serial.println("LoRatask completed");
    Serial.println("LoRatask Suspended");
    vTaskSuspend(xHandleLoRa); // Suspend the task
}

const __FlashStringHelper *getErrorString(uint8_t error)
{
    switch (error)
    {
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

bool initializeMESH()
{
    if (!mesh.init())
    {
        // Serial.println("Mesh initialization failed");
        return false;
    }
    return true;
}

void broadcastPresence()
{
    const char *presenceMsg = "Node Present";
    uint8_t status = mesh.sendtoWait((uint8_t *)presenceMsg, strlen(presenceMsg) + 1, RH_BROADCAST_ADDRESS);
    if (status == RH_ROUTER_ERROR_NONE)
    {
        Serial.println("Message sent successfully");
    }
    else
    {
        Serial.print("Failed to send message, error: ");
        Serial.println(status);
        Serial.println((const __FlashStringHelper *)getErrorString(status));
    }
}

// Function to listen for other nodes and update their status
void listenForNodes()
{
    uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;

    if (mesh.recvfromAckTimeout(buf, &len, 2000, &from))
    {
        Serial.print("Received message from node ");
        Serial.print(from);
        Serial.print(": ");
        Serial.println((char *)buf);

        // Update node information or add new node
        updateNodeStatus(from);
    }
}

// Function to update or add a node status in the list
void updateNodeStatus(uint8_t nodeId)
{
    bool nodeFound = false;
    unsigned long currentTime = millis();
    for (auto &status : nodeStatuses)
    {
        if (status.nodeId == nodeId)
        {
            status.lastSeen = currentTime;
            status.isActive = true;
            nodeFound = true;
            break;
        }
    }
    if (!nodeFound)
    {
        nodeStatuses.push_back({nodeId, currentTime, true});
    }
}

// Function to check and update the activity status of nodes
void checkNodeActivity()
{
    unsigned long currentTime = millis();
    const unsigned long timeout = 60000; // 1 minute timeout to consider a node as dead
    for (auto &status : nodeStatuses)
    {
        if (status.isActive && (currentTime - status.lastSeen > timeout))
        {
            status.isActive = false;
            Serial.print("Node ");
            Serial.print(status.nodeId);
            Serial.println(" is now considered dead.");
        }
    }
}

// Function to get the total number of known nodes
size_t getTotalNodes()
{
    return nodeStatuses.size();
}

// Function to print the status of all nodes known to this node
void printNodeStatuses()
{
    size_t totalNodes = getTotalNodes();
    Serial.print("Total Nodes: ");
    Serial.println(totalNodes);

    for (const auto &status : nodeStatuses)
    { // range based loop
        Serial.print("Node ");
        Serial.print(status.nodeId);
        Serial.print(": ");
        Serial.println(status.isActive ? "Active" : "Dead");
    }
}

void printNetworkStats()
{
    // int totalNodes = nodeStatuses.size();  // Total number of nodes is the size of the vector
    // int activeNodes = 0;
    // int deadNodes = 0;

    totalNodes = 0;
    activeNodes = 0;
    deadNodes = 0;

    // Count active and dead nodes
    for (const auto &status : nodeStatuses)
    {
        Serial.print("Node ");
        Serial.print(status.nodeId);
        Serial.print(": ");
        Serial.println(status.isActive ? "Active" : "Dead");

        if (status.isActive)
        {
            activeNodes++; // Increment active node count
        }
        else
        {
            deadNodes++; // Increment dead node count
        }
    }

    // Total number of nodes is the size of the vector
    totalNodes = nodeStatuses.size();

    // Print the results
    Serial.print("Total Nodes: ");
    Serial.println(totalNodes);
    Serial.print("Active Nodes: ");
    Serial.println(activeNodes);
    Serial.print("Dead Nodes: ");
    Serial.println(deadNodes);
}

// Led functions
void setupLeds()
{
    //   FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB1>(leds, NUM_LEDS);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB6>(SideLEDs, NUM_LEDS_RGB6);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB5>(RightArrowLEDs, NUM_LEDS_RGB5);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB4>(LeftArrowLEDs, NUM_LEDS_RGB4);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB3>(SmallHexagonsAndFireLEDs, NUM_LEDS_RGB3);
    FastLED.addLeds<NEOPIXEL, DATA_PIN_RGB2>(BigHexagonAndAlarmCallPointLEDs, NUM_LEDS_RGB2);
}

// void setAllLedsRed() {
//   led_blink(RGB_LED_BRIGHTNESS, 0, 0);
//   FastLED.show();
// }

// void setAllLedsBlue() {
//   led_blink(0, 0, RGB_LED_BRIGHTNESS);
//   FastLED.show();
// }

// void setAllLedsGreen() {
//   led_blink(0, RGB_LED_BRIGHTNESS, 0);
//   FastLED.show();
// }

// void setAllLedYellow() {
//   led_blink(RGB_LED_BRIGHTNESS, RGB_LED_BRIGHTNESS, 0);
//   FastLED.show();
// }

// void setAllLedWhite() {
//   led_blink(RGB_LED_BRIGHTNESS, RGB_LED_BRIGHTNESS, RGB_LED_BRIGHTNESS);
//   FastLED.show();
// }

// void turnOffAllLeds(struct CRGB * targetArray, int numToFill) {
//   fill_solid(targetArray, numToFill, CRGB::Black);
//   FastLED.show();
// }

void stableWhite(struct CRGB *targetArray, int numToFill)
{
    fill_solid(targetArray, numToFill, CRGB::White);
    FastLED.show();

    //   FillSolidLeds(SideLEDs, NUM_LEDS_RGB2, CRGB::White);    // White when power on
    //   delay(1000);
    //   FillSolidLeds(SideLEDs, NUM_LEDS_RGB2, CRGB::Black);
}

void FillSolidLeds(struct CRGB *targetArray, int numToFill, const struct CRGB &color)
{
    fill_solid(targetArray, numToFill, color);
    FastLED.show();
}

// void stableRed() {
//   fill_solid(leds, NUM_LEDS, CRGB::Red);
//   FastLED.show();
// }

// void stableGreen() {
//   fill_solid(leds, NUM_LEDS, CRGB::Green);
//   FastLED.show();
// }

// void stableBlue() {
//   fill_solid(leds, NUM_LEDS, CRGB::Blue);
//   FastLED.show();
// }

// void stableYellow() {
//   fill_solid(leds, NUM_LEDS, CRGB::Yellow);
//   FastLED.show();
// }

// void led_blink(uint8_t red_intensity, uint8_t green_intensity, uint8_t blue_intensity)
// {

//   for (int i = 0; i < NUM_LEDS / 2; i++) {
//     // fade everything out
//     leds.fadeToBlackBy(40);

//     // set an LED value with user-specified intensities
//     leds[i] = CRGB(red_intensity, green_intensity, blue_intensity);

//     // mirror the first 20 leds to the last 20 leds
//     leds(NUM_LEDS / 2, NUM_LEDS - 1) = leds(NUM_LEDS / 2 - 1, 0);

//     FastLED.show();
//     FastLED.delay(33);
//   }
// }

void initAudio()
{
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21); // default 0...21
}

void download_audio()
{
    sd_card();
    // Download both files
    //   downloadFile(resourceURL, filename); // uncomment to download audio file 1
    //   downloadFile(resourceURL2, filename2); // uncomment to download audio file 2
}

void sd_card()
{
    if (!SD.begin(5))
    {
        Serial.println("Card Mount Failed");
        return;
    }
}

// Function to download a file from a given URL and save it to the specified filename
void downloadFile(const char *resourceURL, const char *filename)
{
    HTTPClient http;
    http.begin(resourceURL);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_FOUND || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
            // Handle redirection
            String newURL = http.getLocation();
            http.end();            // Close the first connection
            http.begin(newURL);    // Open connection with new URL
            httpCode = http.GET(); // Repeat GET request
        }

        if (httpCode == HTTP_CODE_OK)
        {
            File file = SD.open(filename, FILE_WRITE);
            if (file)
            {
                http.writeToStream(&file);
                file.close();
                Serial.println("File downloaded and saved to SD card");
            }
            else
            {
                Serial.println("Failed to open file for writing");
            }
        }
        else
        {
            Serial.printf("Failed to retrieve file. HTTP error: %s\n", http.errorToString(httpCode).c_str());
        }
    }
    else
    {
        Serial.printf("Failed to retrieve file. HTTP error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

void audioTask(void *parameter)
{

    // audio.connecttoFS(SD, "/Fyrebox_-_Fyrebox_Alarm_with_Siren.mp3"); // SD card file

    for (;;)
    {
        // if (buttonPressed) {
        audio.loop();
        // }
        delay(10); // Small delay to prevent watchdog timer issues
    }
}

void startSiren()
{
    digitalWrite(SirenPIN, HIGH);
}

void stopSiren()
{
    digitalWrite(SirenPIN, LOW);
}

void palyAudio()
{
    vTaskResume(xHandleAudio);
    audio.loop();
}

// FOTA functions
int FirmwareVersionCheck()
{
    String payload;
    int httpCode;
    String fwurl = "";
    fwurl += URL_fw_Version;
    fwurl += "?";
    fwurl += String(rand());
    Serial.println(fwurl);
    WiFiClientSecure *client = new WiFiClientSecure;

    if (client)
    {
        client->setCACert(OTA_CAcert);

        // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
        HTTPClient https;

        if (https.begin(*client, fwurl))
        { // HTTPS
            Serial.print("[HTTPS] GET...\n");
            // start connection and send HTTP header
            delay(100);
            httpCode = https.GET();
            delay(100);
            if (httpCode == HTTP_CODE_OK) // if version received
            {
                payload = https.getString(); // save received version
            }
            else
            {
                Serial.print("error in downloading version file:");
                Serial.println(httpCode);
            }
            https.end();
        }
        delete client;
    }

    if (httpCode == HTTP_CODE_OK) // if version received
    {
        payload.trim();
        Serial.print("File Version: ");
        Serial.println(payload);
        if (payload.toFloat() > FirmwareVer.toFloat())
        {
            Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
            return 0;
        }
        else if (payload.equals(FirmwareVer))
        {
            Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
            return 0;
        }
        else
        {
            Serial.println(payload);
            Serial.println("New firmware detected");
            return 1;
        }
    }
    return 0;
}

void firmwareUpdate()
{
    WiFiClientSecure ota_client;
    ota_client.setCACert(OTA_CAcert);
    t_httpUpdate_return ret = httpUpdate.update(ota_client, URL_fw_Bin);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
}

void OTA_repeatedCall()
{
    unsigned long OTA_currentMillis = millis();
    if ((OTA_currentMillis - OTA_previousMillis) >= OTA_interval)
    {
        // save the last time you blinked the LED
        OTA_previousMillis = OTA_currentMillis;
        if (FirmwareVersionCheck())
        {
            firmwareUpdate();
        }
    }
}