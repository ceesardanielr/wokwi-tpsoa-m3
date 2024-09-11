// Declaramos la intensidad del brillo
int BRILLO = 0;
// Pin de entrada del potenciometro
int pinPot=34;
// Pin de salida al led
int pinLed=2;
//Características del PWM
const int frecuencia = 1000;
const int canal = 0;
const int resolucion = 10;
void setup()
{
  Serial.begin(9600);
  //Inicializamos las características del pwm
  ledcAttach(pinLed, frecuencia, resolucion);
  delay(1000);
}
void loop()
{
  if (analogRead(pinPot) > 1000) {
    analogWrite(pinLed, 255);
  } else {
    analogWrite(pinLed, 0);
  };
  //Obtenemos la señal del potenciometro
  //BRILLO = analogRead(pinPot);
  //Mostramos la señal del potenciometro
  //Serial.println(BRILLO);
  //desde 0 a 4095
  //Dividimos la señal en entre 16
  //BRILLO = (BRILLO / 16.2);
  //Serial.println(BRILLO);
  //Encendemos el led
  //analogWrite(pinLed, BRILLO);
  delay(100);
}
