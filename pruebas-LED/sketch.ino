//Declaramos el pin que encendera
int pin_dos = 2;
//Iniciamos los pines del ESP32
void setup() {
  //Declaramos que el pin del led es de tipo salida, osea que la señal va salir
  pinMode(pin_dos, OUTPUT);
}
//Iniciamos la funcion bucle que se repetira indefinidamente
void loop() {
  //Encendemos el led
  digitalWrite(pin_dos, HIGH);
  //Esperamos un segundo
  delay(1000);
  //Apagamos el led
  digitalWrite(pin_dos, LOW);
  //Esperamos un segundo
  delay(500);
}
