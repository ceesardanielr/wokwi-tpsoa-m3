int led = 2; //led conectado a D2

int pirData = 15; // PIR conectado a D15
int pirEstado = LOW; //Asumiendo no movimiento
int value = 0; // Leer el estado del pin



void setup() {
  // put your setup code here, to run once:
  pinMode(led, OUTPUT);
  pinMode(pirData, INPUT);

  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  value = digitalRead(pirData);

  if (value == HIGH) 
  {
    digitalWrite(led, HIGH);
    if (pirEstado == LOW) 
    {
      Serial.println("Movimiento detectado ... ");
      pirEstado = HIGH;
    }
    else 
    {
      digitalWrite(led, LOW);
      if (pirEstado == HIGH)
      {
        Serial.println("Movimiento no detectado ... ");
        pirEstado = LOW;
      }
    }

  }

  delay(10); // this speeds up the simulation
}
