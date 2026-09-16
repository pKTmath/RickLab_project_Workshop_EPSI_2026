#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);
Servo monServo;

// Tableau contenant l'ID autorisé
byte nuidPICC[4];
byte badge[4] = {0x43, 0x4B, 0x51, 0x07};

// Pins des LED (sur broches analogiques)
const int LED_ROUGE  = A0;
const int LED_JAUNE  = A1;
const int LED_BLANC  = A2;
const int LED_VERT   = A3;
const int LED_BLEU   = A4;

// Pins des autres périphériques
const int SERVO_PIN = 3;
const int buzzer    = 5;
const int trig      = 7;
const int echo      = 8;

const int DISTANCE_SEUIL = 30; 

// Positions du servomoteur
const int POS_FERME = 10;
const int POS_OUVERT = 100;

// Variable d'état de la porte
bool isDoorOpen = false; 

// Fonction pour éteindre toutes les LED
void resetLeds()
{
  digitalWrite(LED_ROUGE, LOW);
  digitalWrite(LED_JAUNE, LOW);
  digitalWrite(LED_BLANC, LOW);
  digitalWrite(LED_VERT, LOW);
  digitalWrite(LED_BLEU, LOW);
}

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  
  Serial.println("--- Initialisation du système ---");
  
  SPI.begin();
  rfid.PCD_Init();
  
  pinMode(LED_ROUGE, OUTPUT);
  pinMode(LED_JAUNE, OUTPUT);
  pinMode(LED_BLANC, OUTPUT);
  pinMode(LED_VERT, OUTPUT);
  pinMode(LED_BLEU, OUTPUT);

  pinMode(buzzer, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  monServo.attach(SERVO_PIN);
  monServo.write(POS_FERME);
  delay(500);
  monServo.detach();

  resetLeds();
  digitalWrite(LED_BLEU, HIGH);
  digitalWrite(LED_ROUGE, HIGH);

  Serial.println("--- Système prêt ---");
}

void loop()
{
  int distance = getDistance();

  if (distance > 0 && distance <= DISTANCE_SEUIL)
  {
    Serial.print("[DEBUG] Objet proche (");
    Serial.print(distance);
    Serial.println(" cm). Scanning RFID...");

    resetLeds();
    digitalWrite(LED_JAUNE, HIGH);

    unsigned long startTime = millis();
    unsigned long lastBipTime = 0;
    bool cardFound = false;

    // Scan pendant 1,5s avec des bips d'attente
    while (millis() - startTime < 1500 && !cardFound)
    {
      // Émission d'un bip discret d'attente toutes les 400 ms
      if (millis() - lastBipTime >= 400)
      {
        lastBipTime = millis();
        if(!isDoorOpen)tone(buzzer, 500, 700); // si verrou fermé, son pendant la possibilité de scanner le tag rfid
      }

      rfid.PCD_Init(); 
      delay(50);

      if (rfid.PICC_IsNewCardPresent())
      {
        if (rfid.PICC_ReadCardSerial())
        {
          cardFound = true;

          // Clignotement de confirmation LED Blanche
          resetLeds();
          for (int i = 0; i < 3; i++) {
            digitalWrite(LED_BLANC, HIGH);
            delay(50);
            digitalWrite(LED_BLANC, LOW);
            delay(50);
          }

          bool isRightBadge = true;
          Serial.print("UID : ");
          for (byte i = 0; i < 4; i++)
          {
            nuidPICC[i] = rfid.uid.uidByte[i];
            Serial.print(nuidPICC[i], HEX);
            Serial.print(" ");
            if (nuidPICC[i] != badge[i]) isRightBadge = false;
          }
          Serial.println();

          if (isRightBadge)
          {
            isDoorOpen = !isDoorOpen;
            monServo.attach(SERVO_PIN);

            if (isDoorOpen)
            {
              Serial.println("--> ACCÈS AUTORISÉ : Ouverture (100°)");
              resetLeds();
              digitalWrite(LED_VERT, HIGH);

              // Son d'ouverture
              tone(buzzer, 1318, 80); delay(90);
              tone(buzzer, 1568, 80); delay(90);
              tone(buzzer, 2093, 100); delay(110);
              tone(buzzer, 2637, 80); delay(90);
              tone(buzzer, 2093, 80); delay(90);
              tone(buzzer, 2637, 200);
              
              monServo.write(POS_OUVERT);
            }
            else
            {
              Serial.println("--> ACCÈS AUTORISÉ : Fermeture (10°)");
              resetLeds();
              digitalWrite(LED_BLEU, HIGH);
              digitalWrite(LED_ROUGE, HIGH);

              // Son de fermeture
              tone(buzzer, 2093, 80); delay(90);
              tone(buzzer, 1568, 80); delay(90);
              tone(buzzer, 1047, 180);
              
              monServo.write(POS_FERME);
            }

            delay(600);
            monServo.detach();
          }
          else
          {
            Serial.println("--> ACCÈS REFUSÉ !");
            resetLeds();
            digitalWrite(LED_ROUGE, HIGH);

            // Son Troll / Refus
            tone(buzzer, 466, 200); delay(250);
            tone(buzzer, 440, 200); delay(250);
            tone(buzzer, 415, 200); delay(250);
            tone(buzzer, 392, 600);
          }

          rfid.PICC_HaltA();
          rfid.PCD_StopCrypto1();

          delay(2000); 
        }
      }
    }

    // Réinitialisation de l'affichage après la boucle
    resetLeds();
    if (isDoorOpen) {
      digitalWrite(LED_VERT, HIGH);
    } else {
      digitalWrite(LED_BLEU, HIGH);
      digitalWrite(LED_ROUGE, HIGH);
    }
  }

  delay(200);
}

// Fonction de mesure de la distance (HC-SR04)
int getDistance()
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 25000);
  if (duration == 0) return 999;

  return duration * 0.034 / 2;
}