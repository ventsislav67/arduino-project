#include <LiquidCrystal.h>

// LCD пинове (RS->A0, E->A1, D4->A2, D5->A3, D6->A4, D7->A5)
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

// RGB диоди
const int RED1 = 3;
const int RED2 = 7;
const int GREEN1 = 6;
const int GREEN2 = 9;
const int BLUE1 = 5;
const int BLUE2 = 8;

// HC-SR04
const int trigPin = 10;
const int echoPin = 11;

// Пищялка
const int buzzerPin = 2;

long duration;
float distance;

unsigned long lastBlinkTime = 0;
bool blinkState = false;
int blinkInterval = 300;

// За мигане при 0-5 см
bool stopBlinking = false;

// За зелено премигване при изключване
bool greenBlinking = false;
unsigned long greenBlinkStart = 0;
int greenBlinkCount = 0;

// Ново: режим само часовник
bool showingClockOnly = false;

unsigned long previousMillis = 0;
int seconds = 0;
int minutes = 0;
int hours = 12;  // начало 12:00:00

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(RED1, OUTPUT);
  pinMode(RED2, OUTPUT);
  pinMode(GREEN1, OUTPUT);
  pinMode(GREEN2, OUTPUT);
  pinMode(BLUE1, OUTPUT);
  pinMode(BLUE2, OUTPUT);

  pinMode(buzzerPin, OUTPUT);
  noTone(buzzerPin);

  setColor(0, 0, 0);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Distance: -- cm");
  lcd.setCursor(0, 1);
  lcd.print("Color: -----");
}

void loop() {
  unsigned long currentMillis = millis();

  // Обновяване на времето
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) hours = 0;
      }
    }
  }

  // Винаги мерим дистанцията
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 20000);
  distance = duration * 0.034 / 2.0;
  if (duration == 0 || distance < 0.5) distance = 0.5;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Ако сме в режим само часовник и дистанцията <= 45 см, излизаме от него
  if (showingClockOnly && distance <= 45) {
    showingClockOnly = false;
    greenBlinking = false;
  }

  // Ако сме в режим само часовник, показваме само часовника
  if (showingClockOnly) {
    showClock();
    return;
  }

  // Ако сме в зелено мигане (преди да влезем в режим часовник)
  if (greenBlinking) {
    greenBlinkLoop();
    return;
  }

  // Ако дистанцията > 45 и не сме в зелено мигане, започваме зелено мигане
  if (distance > 45 && !greenBlinking) {
    greenBlinking = true;
    greenBlinkStart = millis();
    greenBlinkCount = 0;
    return;
  }

  // 0-5 см: мигащо червено + "STOP" + пищялка
  if (distance <= 5) {
    stopBlinking = true;
    blinkRedWithBuzzer();
    return;
  } else {
    stopBlinking = false;
  }

  // 5-10 см: червено преливане (от 5 най-ярко, към 10 по-слабо)
  if (distance > 5 && distance <= 10) {
    noTone(buzzerPin);
    float t = (distance - 5) / 5.0; // 0..1 от 5 до 10
    int redVal = int(255 * (1 - t)); // от 255 към 0
    setColor(redVal, 0, 0);
    updateLCD(distance, "RED");
    return;
  }

  // 10-20 см: ЖЪЛТО (червено + зелено), плавен преход
  if (distance > 10 && distance <= 20) {
    noTone(buzzerPin);
    float t = (distance - 10) / 10.0; // 0..1 от 10 до 20
    int redVal = 255;
    int greenVal = int(255 * t);
    setColor(redVal, greenVal, 0);
    updateLCD(distance, "YELLOW");
    return;
  }

  // 20-35 см: ЗЕЛЕНО (преливане)
  if (distance > 20 && distance <= 35) {
    noTone(buzzerPin);
    float t = (distance - 20) / 15.0; // 0..1
    int redVal = int(255 * (1 - t));
    int greenVal = 255;
    setColor(redVal, greenVal, 0);
    updateLCD(distance, "GREEN");
    return;
  }

  // 35-45 см: СИНЬО (преливане)
  if (distance > 35 && distance <= 45) {
    noTone(buzzerPin);
    float t = (distance - 35) / 10.0; // 0..1
    int greenVal = int(255 * (1 - t));
    int blueVal = int(255 * t);
    setColor(0, greenVal, blueVal);
    updateLCD(distance, "BLUE");
    return;
  }

  delay(15);
}

// Мигащо червено със звук и STOP надпис при 0-5см
void blinkRedWithBuzzer() {
  unsigned long now = millis();
  if (now - lastBlinkTime >= blinkInterval) {
    blinkState = !blinkState;
    lastBlinkTime = now;
  }

  if (blinkState) {
    setColor(255, 0, 0);
    tone(buzzerPin, 1000);
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("STOP");
    lcd.setCursor(0, 1);
    lcd.print("Distance:");
    lcd.setCursor(9, 1);
    lcd.print(distance, 1);
    lcd.print("cm");
  } else {
    setColor(0, 0, 0);
    noTone(buzzerPin);
    lcd.clear();
  }
}

// Двукратно зелено премигване по 300 ms преди режим часовник
void greenBlinkLoop() {
  unsigned long now = millis();
  if (!blinkState) {
    setColor(0, 255, 0);
    blinkState = true;
    lastBlinkTime = now;
  } else if (now - lastBlinkTime >= 300) {
    setColor(0, 0, 0);
    greenBlinking = false;
    showingClockOnly = true; // влизаме в режим само часовник
    showClock();
  }
}

void setColor(int red, int green, int blue) {
  analogWriteSafe(RED1, red);
  analogWriteSafe(GREEN1, green);
  analogWriteSafe(BLUE1, blue);

  digitalWrite(RED2, red > 10 ? HIGH : LOW);
  analogWriteSafe(GREEN2, green);
  digitalWrite(BLUE2, blue > 10 ? HIGH : LOW);
}

void analogWriteSafe(int pin, int value) {
  if (pin == 3 || pin == 5 || pin == 6 || pin == 9 || pin == 10 || pin == 11) {
    analogWrite(pin, value);
  } else {
    digitalWrite(pin, value > 10 ? HIGH : LOW);
  }
}

void updateLCD(float dist, String colorName) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  lcd.setCursor(10, 0);
  if (dist < 1 || dist > 400) {
    lcd.print("- cm");
  } else {
    lcd.print(dist, 1);
    lcd.print("cm");
  }
  lcd.setCursor(0, 1);
  lcd.print("Color: ");
  lcd.print(colorName);
}

void showClock() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (hours < 10) lcd.print("0");
  lcd.print(hours);
  lcd.print(":");
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);

  lcd.setCursor(0, 1);
  lcd.print("Ventsi Project");
}







