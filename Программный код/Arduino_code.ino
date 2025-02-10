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
#define CARGATE_SWITCH_PIN 17    // Пин концевого выключателя ворот
#define LIMIT_SWITCH_PIN 21
#define LIMIT_SWITCH_CLOSE_PIN 22

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

// --- Мелодия для пьезопищалки ---
int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
int noteDurations[] = {200, 200, 200, 200, 200, 200, 200, 400};

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

void playSuccessTone() {
    for (int i = 0; i < 8; i++) {
        tone(BUZZER_PIN, melody[i]);
        delay(noteDurations[i]);
        noTone(BUZZER_PIN);
        delay(50);
    }
}

void playErrorTone() {
    for (int i = 7; i >= 0; i--) {
        tone(BUZZER_PIN, melody[i]);
        delay(noteDurations[i]);
        noTone(BUZZER_PIN);
        delay(50);
    }
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
                playSuccessTone();
                delay(1000);
                state = HUMGATE_AVAILABLE_ST;
              
            } else {
                
                playErrorTone();
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

int gateState = OPEN_SOLENOID;

void humgateUnavailableStateHandler() {
    delay(2000);
    state = GETTING_DATA_ST; // Возвращаемся в режим ожидания
}

void humgateAvailableStateHandler() {
    delay(2000);
    state = HUMGATE_AVAILABLE_ST;
}


void openSolenoid() {
    
    digitalWrite(SOLENOID_PIN, HIGH);
    gateState = OPEN_GATE;
}


void cargateAvailableHandler() {
    lcd.print("Opening gate...");
    servo.write(90);
    delay(1000);
    gateState = WAIT_PEDESTRIAN;
}


void waitForPedestrian() {
    lcd.print("Waiting for pedestrian...");
    unsigned long startTime = millis();
    while (millis() - startTime < PIR_TIMEOUT_MS) {
        if (digitalRead(PIR_SENSOR_PIN) == HIGH) {
            logMessage("Pedestrian detected!");
            gateState = PEDESTRIAN_PASS;
            return;
        }
    }
    gateState = CLOSE_GATE;
}


void pedestrianPass() {
    lcd.print("Pedestrian passing...");
    while (digitalRead(PIR_SENSOR_PIN) == HIGH);
    delay(1000);
    gateState = WAIT_AFTER_PASS;
}


void waitAfterPass() {
    lcd.print("Waiting after pass...");

    unsigned long startTime = millis();
    while (millis() - startTime < WAIT_AFTER_PASS_MS) {
        if (checkPIR()) { // Проверяем наличие пешехода
            lcd.print("Pedestrian detected during wait. Transitioning to pedestrian pass...");
            gateState = PEDESTRIAN_PASS;
            return;
        }
        delay(100); // Небольшая задержка для предотвращения перегрузки процессора
    }

    gateState = CLOSE_GATE;
}


void closeGate() {
    lcd.print("Closing gate...");
    
    unsigned long startTime = millis();
    servo.write(0); // Начинаем закрытие калитки

    while (millis() - startTime < 1000) { // Проверяем пешехода в течение закрытия (1 сек)
        if (checkPIR()) { // Если обнаружен пешеход
            lcd.print("Pedestrian detected during gate closing. Reopening gate...");
            servo.write(90); // Открываем калитку обратно
            gateState = PEDESTRIAN_PASS; // Переход в состояние прохода пешехода
            return;
        }
        delay(100); // Небольшая задержка, чтобы не перегружать процессор
    }

    gateState = CLOSE_SOLENOID; // Если пешеход не обнаружен, продолжаем закрытие
}





void closeSolenoid() {
    lcd.print("Closing solenoid...");
    digitalWrite(SOLENOID_PIN, LOW);
}


void handleGateLogic() {
    switch (gateState) {
        case OPEN_SOLENOID: openSolenoid(); break;
        case OPEN_GATE: cargateAvailableHandler(); break;
        case WAIT_PEDESTRIAN: waitForPedestrian(); break;
        case PEDESTRIAN_PASS: pedestrianPass(); break;
        case WAIT_AFTER_PASS: waitAfterPass(); break;
        case CLOSE_GATE: closeGate(); break;
        case CLOSE_SOLENOID: closeSolenoid(); break;
    }
}






//---------------------------

bool uidReceived = false;

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
    
    Serial.print("Sending UID: "); Serial.println(receivedUID); // Логируем UID
    Serial.println(receivedUID); // Отправляем UID на сервер
    
    delay(100); // Даем время серверу ответить

    unsigned long startTime = millis();
    while (millis() - startTime < 5000) { // Ждать до 5 секунд
        if (Serial.available() > 0) {
            String response = Serial.readStringUntil('\n');
            response.trim(); // Удаляем пробелы и символы новой строки

            if (response.equalsIgnoreCase("GRANTED")) {
                lcd.print("Access Granted");
                logMessage("Access granted by server.");
                state = CARGATE_AVAILABLE_ST;
                return;
            } else if (response.equalsIgnoreCase("DENIED")) {
                lcd.print("Access Denied");
                logMessage("Access denied by server.");
                state = CARGATE_UNAVAILABLE_ST;
                return;
            } else {
              logMessage(("Unexpected response from server: " + response).c_str());
            }
        }
    }
    
    lcd.print("No Response");
    logMessage("Server did not respond in time.");
    memset(receivedUID, 0, sizeof(receivedUID)); // Очистка UID
    
  }


#define OPEN_CAR_GATE           0
#define WAIT_CAR                1
#define WAIT_CAR_PASS           2
#define WAIT_AFTER_CAR_PASS     3
#define CLOSE_CAR_GATE          4


void cargateUnavailableHandler() {
    for (int i = 0; i < 10; i++) {
        digitalWrite(LED_STRIP_PIN, HIGH);
        delay(200);
        digitalWrite(LED_STRIP_PIN, LOW);
        delay(200);
    }
    lcd.println("Доступ запрещён");
    state = GETTING_DATA_ST;
}



void stepMotor(int steps, bool direction) {
    digitalWrite(DIR_PIN, direction);
    for (int i = 0; i < steps; i++) {
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(STEP_DELAY);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(STEP_DELAY);
    }
}



void open_car_Gate() {
    lcd.print("Opening gate...");
    pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP); // Включаем подтяжку

    while (digitalRead(LIMIT_SWITCH_PIN) == HIGH) { // Пока концевик не сработает
        stepMotor(1, HIGH); // Двигаем мотор на один шаг вперед
        delayMicroseconds(STEP_DELAY); // Задержка между шагами
    }
    state = WAIT_CAR; // Переход в следующее состояние
}


void waitForCar() {
    lcd.print("Waiting for car...");
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
    lcd.print("Closing gate...");
    pinMode(CARGATE_SWITCH_PIN, INPUT_PULLUP);
    while (digitalRead(LIMIT_SWITCH_CLOSE_PIN) == HIGH) { // Пока ворота не закроются
      stepMotor(1, LOW); // Двигаем мотор по одному шагу назад
      delayMicroseconds(STEP_DELAY); // Задержка между шагами

      if (checkPIR()) { // Если пешеход обнаружен во время закрытия
          logMessage("Pedestrian detected! Reopening gate...");
          gateState = WAIT_CAR_PASS;
          return; // Прекращаем закрытие
      }
  }
  state = GETTING_DATA_ST;
  
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
