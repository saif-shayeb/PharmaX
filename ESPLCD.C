#include <WiFi.h>
#include <HTTPClient.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "PharmacySystem";
const char* password = "12345678";
const char* serverIP = "192.168.4.1";

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {14,27,26,25};
byte colPins[COLS] = {33,32, 19, 18};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

struct Medicine {
  int id;
  String name;
  String command;
};

Medicine medicines[12] = {
  {1, "Paracetamol 500mg", "S0-0"},
  {2, "Ibuprofen 400mg", "S0-1"},
  {3, "Amoxicillin 250mg", "S0-2"},
  {4, "Omeprazole 20mg", "S0-3"},
  {5, "Aspirin 100mg", "S1-0"},
  {6, "Cetirizine 10mg", "S1-1"},
  {7, "Metformin 500mg", "S1-2"},
  {8, "Atorvastatin 20mg", "S1-3"},
  {9, "Salbutamol Inhaler", "S2-0"},
  {10, "Loratadine 10mg", "S2-1"},
  {11, "Diazepam 5mg", "S2-2"},
  {12, "Ciprofloxacin 500mg", "S2-3"}
};

String inputNumber = "";
unsigned long lastKeyTime = 0;
const unsigned long resetTime = 5000;
bool wifiConnected = false;

enum SystemState {
  STATE_MAIN_MENU,
  STATE_SHOWING_MEDICINES,
  STATE_WAITING_INPUT,
  STATE_CONFIRMING,
  STATE_SHOWING_MESSAGE,
  STATE_SYSTEM_STATUS,
  STATE_TESTING_CONNECTION
};

SystemState currentState = STATE_MAIN_MENU;
unsigned long stateStartTime = 0;
int medicineDisplayPage = 0;
int selectedMedicineId = 0;
String messageLine1 = "";
String messageLine2 = "";
unsigned long messageDuration = 0;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    lcd.clear();
    lcd.print("WiFi Connected");
    delay(1000);
  } else {
    lcd.clear();
    lcd.print("WiFi Failed");
    lcd.setCursor(0, 1);
    lcd.print("Keypad Only Mode");
    delay(2000); 
  }
  
  changeState(STATE_MAIN_MENU);
  lastKeyTime = millis();
}

void loop() {
  char key = keypad.getKey();
  
  if (key) {
    lastKeyTime = millis();
    handleKeyPress(key);
  }
  
  handleState();
  
  if (millis() - lastKeyTime > resetTime) {
    resetSystem();
  }
}

void changeState(SystemState newState) {
  currentState = newState;
  stateStartTime = millis();
  
  switch (currentState) {
    case STATE_MAIN_MENU:
      lcd.clear();
      lcd.print("Select Medicine:");
      lcd.setCursor(0, 1);
      lcd.print("Enter 1-12");
      medicineDisplayPage = 0;
      break;
      
    case STATE_SHOWING_MEDICINES:
      medicineDisplayPage = 0;
      showMedicinePage(0);
      break;
      
    case STATE_WAITING_INPUT:
      lcd.clear();
      lcd.print("Enter number:");
      lcd.setCursor(0, 1);
      lcd.print("1-12 then #");
      break;
      
    case STATE_CONFIRMING:
      lcd.clear();
      lcd.print("Confirm:");
      lcd.setCursor(0, 1);
      lcd.print(medicines[selectedMedicineId-1].name.substring(0, 16));
      break;
      
    case STATE_SHOWING_MESSAGE:
      lcd.clear();
      lcd.print(messageLine1);
      lcd.setCursor(0, 1);
      lcd.print(messageLine2);
      break;
      
    case STATE_SYSTEM_STATUS:
      lcd.clear();
      lcd.print("System Status:");
      lcd.setCursor(0, 1);
      lcd.print(wifiConnected ? "WiFi Connected" : "WiFi Disconnected");
      break;
      
    case STATE_TESTING_CONNECTION:
      lcd.clear();
      lcd.print("Testing...");
      break;
  }
}

void handleState() {
  switch (currentState) {
    case STATE_SHOWING_MEDICINES:
      if (millis() - stateStartTime > 2000) {
        medicineDisplayPage++;
        if (medicineDisplayPage >= 3) {
          changeState(STATE_WAITING_INPUT);
        } else {
          showMedicinePage(medicineDisplayPage);
          stateStartTime = millis();
        }
      }
      break;
      
    case STATE_CONFIRMING:
      if (millis() - stateStartTime > 3000) {
        resetSystem();
      }
      break;
      
    case STATE_SHOWING_MESSAGE:
      if (millis() - stateStartTime > messageDuration) {
        resetSystem();
      }
      break;
      
    case STATE_SYSTEM_STATUS:
      if (millis() - stateStartTime > 2000) {
        resetSystem();
      }
      break;
  }
}

void showMedicinePage(int page) {
  lcd.clear();
  switch (page) {
    case 0:
      lcd.print("1:Parcet 2:Ibu");
      lcd.setCursor(0, 1);
      lcd.print("3:Amoxi 4:Omep");
      break;
    case 1:
      lcd.print("5:Aspir 6:Ceti");
      lcd.setCursor(0, 1);
      lcd.print("7:Metfo 8:Ator");
      break;
    case 2:
      lcd.print("9:Salb 10:Lora");
      lcd.setCursor(0, 1);
      lcd.print("11:Diaz 12:Cipro");
      break;
  }
}

void handleKeyPress(char key) {
  switch (currentState) {
    case STATE_MAIN_MENU:
      if (key == '#' || key == '*') {
        changeState(STATE_SHOWING_MEDICINES);
      }
      break;
      
    case STATE_SHOWING_MEDICINES:
      if (key) {
        medicineDisplayPage++;
        if (medicineDisplayPage >= 3) {
          changeState(STATE_WAITING_INPUT);
        } else {
          showMedicinePage(medicineDisplayPage);
          stateStartTime = millis();
        }
      }
      break;
      
    case STATE_WAITING_INPUT:
      if (key >= '0' && key <= '9') {
        if (inputNumber.length() < 2) {
          inputNumber += key;
          lcd.clear();
          lcd.print("Selected:");
          lcd.setCursor(0, 1);
          lcd.print(inputNumber);
        }
      } 
      else if (key == '#') {
        if (inputNumber.length() > 0) {
          selectedMedicineId = inputNumber.toInt();
          if (selectedMedicineId >= 1 && selectedMedicineId <= 12) {
            changeState(STATE_CONFIRMING);
          } else {
            showMessage("Invalid number", "Enter 1-12", 1500);
            inputNumber = "";
            changeState(STATE_MAIN_MENU);
          }
        }
      }
      else if (key == '*') {
        resetSystem();
      }
      else if (key == 'A') {
        sendCommand("HOME");
        showMessage("Going Home...", "Please wait", 1500);
      }
      else if (key == 'B') {
        sendCommand("PARK");
        showMessage("Parking...", "Please wait", 1500);
      }
      else if (key == 'C') {
        changeState(STATE_SYSTEM_STATUS);
      }
      else if (key == 'D') {
        changeState(STATE_TESTING_CONNECTION);
        testConnection();
      }
      break;
      
    case STATE_CONFIRMING:
      if (key == '#') {
        sendCommand(medicines[selectedMedicineId-1].command);
        showMessage("Request sent:", medicines[selectedMedicineId-1].name, 2000);
      }
      else if (key == '*') {
        resetSystem();
      }
      break;
      
    case STATE_SHOWING_MESSAGE:
      resetSystem();
      break;
      
    case STATE_SYSTEM_STATUS:
      resetSystem();
      break;
  }
}

void showMessage(String line1, String line2, unsigned long duration) {
  messageLine1 = line1;
  messageLine2 = line2;
  messageDuration = duration;
  changeState(STATE_SHOWING_MESSAGE);
}

void sendCommand(String command) {
  if (wifiConnected) {
    HTTPClient http;
    String url = "http://" + String(serverIP) + "/cmd?command=" + command;
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Response: " + payload);
    } else {
      Serial.println("Error sending command");
    }
    
    http.end();
  } else {
    showMessage("No WiFi Connection", "Using Keypad Only", 1500);
  }
}

void testConnection() {
  if (wifiConnected) {
    HTTPClient http;
    String url = "http://" + String(serverIP) + "/";
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      showMessage("Server Online", "Code: " + String(httpCode), 1500);
    } else {
      showMessage("Server Offline", "Check Connection", 1500);
    }
    
    http.end();
  } else {
    showMessage("WiFi Disconnected", "Cannot test", 1500);
  }
}

void resetSystem() {
  inputNumber = "";
  changeState(STATE_MAIN_MENU);
  lastKeyTime = millis();
}