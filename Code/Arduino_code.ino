#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#include <Keypad.h>

// --- Pin Definitions ---
#define DIR_PIN 19             // Direction pin (stepper motor)
#define STEP_PIN 18            // Step pin (stepper motor)
#define SOLENOID_PIN 12        // Solenoid control pin
#define LED_STRIP_PIN 13       // LED strip control pin
#define BUZZER_PIN 14          // Piezo buzzer control pin
#define PIR_SENSOR_PIN 15      // PIR motion sensor pin (HC-SR502)
#define SERVO_PIN 16           // Servo control pin

// --- System Settings ---
#define STEP_DELAY 500         // Delay between steps in microseconds
#define PIR_TIMEOUT_MS 30000   // Motion detection timeout (30 seconds)
#define WAIT_AFTER_PASS_MS 5000 // Wait time after a pass (5 seconds)

// --- Objects ---
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD address 0x27, 16 characters, 2 rows
Servo servo;


// --- Keypad Configuration ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6}; // Подключение строк
byte colPins[COLS] = {5, 4, 3, 2}; // Подключение столбцов
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- Password Configuration ---
const String correctPassword = "1234"; // Установите правильный пароль
String enteredPassword = "";           // Для ввода пользователя


// --- Global Variables ---
int state;                      // Current system state
unsigned long lastActionTime = 0; // Tracks the last action's timestamp
char receivedUID[20]; // Buffer for UID received from the reader
bool pirTriggered = false;        // PIR sensor state

// --- States ---
#define PREPARATION_ST 0
#define GETTING_DATA_ST 1
#define PSWD_CHECK_ST 2
#define HUMGATE_UNAVAILABLE_ST 3
#define HUMGATE_AVAILABLE_ST 4
#define SERVER_WAIT_STATUS_ST 5     // Waiting for server response
#define CARGATE_UNAVAILABLE_ST 6   // Car gate access denied
#define CARGATE_AVAILABLE_ST 7     // Car gate access granted

// --- Utility Functions ---
void logMessage(const char* message) {
    Serial.println(message);
}

// --- System Initialization ---
void setup() {
    Serial.begin(115200);
    logMessage("System initializing...");

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");

    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(SOLENOID_PIN, OUTPUT);
    pinMode(LED_STRIP_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(PIR_SENSOR_PIN, INPUT);

    servo.attach(SERVO_PIN);
    servo.write(0); // Set servo to initial position

    delay(1000);
    lcd.clear();
    state = PREPARATION_ST;
}

// --- System Check ---
void systemCheck() {
    lcd.clear();
    lcd.print("System Check...");
    delay(1000);
    lcd.clear();
    lcd.print("Swipe RFID Card");
    Serial.println("Waiting for RFID...");
    digitalWrite(SOLENOID_PIN, LOW); // Disable solenoid
    digitalWrite(LED_STRIP_PIN, LOW); // Turn off LED strip
    noTone(BUZZER_PIN);               // Turn off buzzer
    servo.write(0);                   // Set servo to closed position
    lcd.clear();
    lcd.setCursor(0, 0);
}

// --- Stepper Motor Control ---
void stepMotor(int steps, bool direction) {
    digitalWrite(DIR_PIN, direction);
    for (int i = 0; i < steps; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(STEP_DELAY);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(STEP_DELAY);
    }
}

// --- PIR Sensor Check ---
bool checkPIR() {
    return digitalRead(PIR_SENSOR_PIN) == HIGH;
}

// --- State Handlers ---
void preparationStateHandler() {
    logMessage("Entering preparation state...");
    systemCheck();
    state = GETTING_DATA_ST;
}
gettingDataStateHandler

//функция каторая отслеживает ввод пароля либо приложение rfid карты на проезд
void gettingDataStateHandler() {
    logMessage("Entering data input state...");
    lcd.setCursor(0, 0);
    lcd.print("Enter password or");
    lcd.setCursor(0, 1);
    lcd.print("Swipe RFID Card");

    while (true) {
        // Проверка на ввод пароля
        char key = keypad.getKey();
        if (key) {
            enteredPassword += key;
            lcd.clear();
            lcd.print("Password:");
            lcd.setCursor(0, 1);
            lcd.print(enteredPassword);

            // Если пользователь нажал "#" для подтверждения
            if (key == '#') {
                state = PSWD_CHECK_ST;
                return;
            }

            // Если пользователь нажал "*" для сброса
            if (key == '*') {
                enteredPassword = "";
                lcd.clear();
                lcd.print("Enter password or");
                lcd.setCursor(0, 1);
                lcd.print("Swipe RFID Card");
            }
        }

        // Проверка на получение UID через RFID
        if (Serial.available() > 0) {
            int len = Serial.readBytesUntil('\n', receivedUID, sizeof(receivedUID) - 1);
            receivedUID[len] = '\0'; // Завершающий символ строки
            lcd.clear();
            lcd.print("UID Received");
            delay(1000);
            state = SERVER_WAIT_STATUS_ST;
            return;
        }
    }
}







void getPasswordInput(){
    char key = keypad.getKey();
    if (key) {
        if (key == '#') { // Подтверждение пароля
            if (enteredPassword == correctPassword) {
                lcd.clear();
                lcd.print("Access Granted");
                delay(1000);
                state = HUMGATE_AVAILABLE_ST;
            } else {
                lcd.clear();
                lcd.print("Access Denied");
                tone(BUZZER_PIN, 1000, 500);
                delay(1000);
                state = HUMGATE_UNAVAILABLE_ST;
            }
            enteredPassword = ""; // Сброс введенного пароля
        } else if (key == '*') { // Сброс пароля
            enteredPassword = "";
            lcd.clear();
            lcd.print("Enter Password");
        } else { // Добавление цифры
            enteredPassword += key;
            lcd.setCursor(0, 1);
            lcd.print(enteredPassword);
        }
    }
}



#define OPEN_SOLENOID       0
#define OPEN_GATE           1
#define WAIT_PEDESTRIAN     2
#define PEDESTRIAN_PASS     3
#define WAIT_AFTER_PASS     4
#define CLOSE_GATE          5
#define CLOSE_SOLENOID      6


void cargateUnavailableHandler(){
  lcd.print("access is denied");
}



void openSolenoid() {
    logMessage("Opening solenoid...");
    digitalWrite(SOLENOID_PIN, HIGH);
    gateState = S1_OPEN_GATE;
}


void cargateAvailableHandler() {
    logMessage("Opening gate...");
    servo.write(90);
    delay(1000);
    gateState = S2_WAIT_PEDESTRIAN;
}


void waitForPedestrian() {
    logMessage("Waiting for pedestrian...");
    unsigned long startTime = millis();
    while (millis() - startTime < PIR_TIMEOUT_MS) {
        if (digitalRead(PIR_SENSOR_PIN) == HIGH) {
            logMessage("Pedestrian detected!");
            gateState = S3_PEDESTRIAN_PASS;
            return;
        }
    }
    gateState = S5_CLOSE_GATE;
}


void pedestrianPass() {
    logMessage("Pedestrian passing...");
    while (digitalRead(PIR_SENSOR_PIN) == HIGH);
    delay(1000);
    gateState = S4_WAIT_AFTER_PASS;
}


void waitAfterPass() {
    logMessage("Waiting after pass...");

    unsigned long startTime = millis();
    while (millis() - startTime < WAIT_AFTER_PASS_MS) {
        if (checkPIR()) { // Проверяем наличие пешехода
            logMessage("Pedestrian detected during wait. Transitioning to pedestrian pass...");
            gateState = PEDESTRIAN_PASS;
            return;
        }
        delay(100); // Небольшая задержка для предотвращения перегрузки процессора
    }

    gateState = CLOSE_GATE;
}


void closeGate() {
    logMessage("Closing gate...");
    
    unsigned long startTime = millis();
    servo.write(0); // Начинаем закрытие калитки

    while (millis() - startTime < 1000) { // Проверяем пешехода в течение закрытия (1 сек)
        if (checkPIR()) { // Если обнаружен пешеход
            logMessage("Pedestrian detected during gate closing. Reopening gate...");
            servo.write(90); // Открываем калитку обратно
            gateState = S5_PEDESTRIAN_PASS; // Переход в состояние прохода пешехода
            return;
        }
        delay(100); // Небольшая задержка, чтобы не перегружать процессор
    }

    gateState = S6_CLOSE_SOLENOID; // Если пешеход не обнаружен, продолжаем закрытие
}


void closeSolenoid() {
    logMessage("Closing solenoid...");
    digitalWrite(SOLENOID_PIN, LOW);
}


void handleGateLogic() {
    switch (gateState) {
        case OPEN_SOLENOID: openSolenoid(); break;
        case OPEN_GATE: openGate(); break;
        case WAIT_PEDESTRIAN: waitForPedestrian(); break;
        case PEDESTRIAN_PASS: pedestrianPass(); break;
        case WAIT_AFTER_PASS: waitAfterPass(); break;
        case CLOSE_GATE: closeGate(); break;
        case CLOSE_SOLENOID: closeSolenoid(); break;
    }
}






//---------------------------

void systemCheck() {
    lcd.clear();
    lcd.print("System Check...");
    delay(1000);
    lcd.clear();
    lcd.print("Swipe RFID Card");
    Serial.println("Waiting for RFID...");
}

void waitForUID() {
    if (Serial.available() > 0) {
        int len = Serial.readBytesUntil('\n', receivedUID, sizeof(receivedUID) - 1);
        receivedUID[len] = '\0';
        uidReceived = true;
        lcd.clear();
        lcd.print("UID Received");
        delay(1000);
        state = SERVER_WAIT_STATUS_ST;
    }
}

void serverWaitStatusHandler() {
    lcd.clear();
    lcd.print("Checking UID...");
    Serial.println(receivedUID); // Send UID to Raspberry Pi
    delay(100); // Wait for response

    unsigned long startTime = millis();
  while (millis() - startTime < 5000) { // Ждать до 5 секунд
      if (Serial.available() > 0) {
        String response = Serial.readStringUntil('\n');
          if (response == "GRANTED") {
            lcd.print("Access Granted");
            state = CARGATE_AVAILABLE_ST;
            return;
          } else if (response == "DENIED") {
            lcd.print("Access Denied");
            state = CARGATE_UNAVAILABLE_ST;
            return;
        }
    }
  }
  lcd.print("No Response");
  state = PREPARATION_ST;
}


#define OPEN_CAR_GATE           0
#define WAIT_CAR                1
#define WAIT_CAR_PASS           2
#define WAIT_AFTER_CAR_PASS     3
#define CLOSE_CAR_GATE          4


void stepMotor(int steps, bool direction) {
    digitalWrite(DIR_PIN, direction);
    for (int i = 0; i < steps; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(STEP_DELAY);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(STEP_DELAY);
    }
}



bool checkPIR() {
    return digitalRead(PIR_SENSOR_PIN) == HIGH;
}


void open_car_Gate() {
    logMessage("Opening gate...");
    stepMotor(STEPS_PER_REV, HIGH);
    gateState = WAIT_PEDESTRIAN;
}


void waitForCar() {
    logMessage("Waiting for pedestrian...");
    unsigned long startTime = millis();
    while (millis() - startTime < PIR_TIMEOUT_MS) {
        if (checkPIR()) {
            gateState = WAIT_CAR_PASS;
            return;
        }
    }
    gateState = CLOSE_CAR_GATE;
}


void CarPass() {
    while (checkPIR());
    delay(1000);
    gateState = WAIT_AFTER_CAR_PASS;
}


void waitAfterCarPass() {
    unsigned long startTime = millis();
    while (millis() - startTime < WAIT_AFTER_PASS_MS) {
        if (checkPIR()) {
            gateState = PEDESTRIAN_PASS;
            return;
        }
    }
    gateState = CLOSE_GATE;
}

void closeCarGate() {
    logMessage("Closing gate...");
    unsigned long startTime = millis();
    stepMotor(STEPS_PER_REV, LOW);
    while (millis() - startTime < 1000) {
        if (checkPIR()) {
            gateState = WAIT_CAR_PASS;
            return;
        }
    }
}


void handleCarGateLogic() {
    switch (gateState) {
        case OPEN_CAR_GATE: open_car_Gate(); break;
        case WAIT_CAR: waitForCar(); break;
        case WAIT_CAR_PASS: CarPass(); break;
        case WAIT_AFTER_CAR_PASS: waitAfterCarPass(); break;
        case CLOSE_CAR_GATE: closeCarGate(); break;
    }
}





// --- Main Loop ---
void loop() {
    switch (state) {
        case PREPARATION_ST:
            preparationStateHandler();
            break;
        case GETTING_DATA_ST:
            gettingDataStateHandler();
            break;
        case PSWD_CHECK_ST:
            getPasswordInput();
            break;
        case HUMGATE_UNAVAILABLE_ST:
            humgateUnavailableStateHandler();
            break;
        case HUMGATE_AVAILABLE_ST:
            humgateAvailableStateHandler();
            break;
        case SERVER_WAIT_STATUS_ST:
            serverWaitStatusHandler();
            break;
        case CARGATE_UNAVAILABLE_ST:
            cargateUnavailableHandler();
            break;
        case CARGATE_AVAILABLE_ST:
            cargateAvailableHandler();
            break;
        default:
            break;
    }
}