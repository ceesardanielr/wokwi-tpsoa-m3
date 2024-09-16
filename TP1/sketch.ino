// ------------------------------------------------
// Librerías
// ------------------------------------------------
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

// ------------------------------------------------
// Etiquetas
// ------------------------------------------------
#define LOG // Comentar esta linea para desactivar logs

// ------------------------------------------------
// Constantes
// ------------------------------------------------
#define UMBRAL_SONIDO_BAJO 10
#define UMBRAL_SONIDO_MEDIO 100
#define UMBRAL_SONIDO_ALTO 200
//#define FUNCION_FULL 1
//#define FUNCION_SOLO_SONIDO 0


// ------------------------------------------------
// Pines sensores (A = analógico | D = Digital)
// ------------------------------------------------
#define PIN_D_SENSOR_DISTANCIA 15
#define PIN_A_SENSOR_SONIDO 34
#define PIN_D_PULSADOR_FUNCION 25

// ------------------------------------------------
// Pines actuadores (P = PWM | D = Digital)
// ------------------------------------------------
#define PIN_P_ACTUADOR_LED_SONIDO 2
#define PIN_D_ACTUADOR_LED_MOVIMIENTO 4


// Declaramos la intensidad del brillo
int BRILLO = 0;

//modo
int MODO = 0; //0 Normal 1 Full

//Características del PWM
const int frecuencia = 1000;
const int canal = 0;
const int resolucion = 10;

//Pulsador estados
int button_state;       // the current state of button
int last_button_state;  // the previous state of button

// PIR
int valorMovimiento = 0; // Leer el estado del pin

void start()
{
  // Display
  LCD.init();
  LCD.backlight();

  //Inicializamos las características del pwm
  ledcAttach(PIN_P_ACTUADOR_LED_SONIDO, frecuencia, resolucion);

  pinMode(PIN_D_ACTUADOR_LED_MOVIMIENTO, OUTPUT);
  pinMode(PIN_D_SENSOR_DISTANCIA, INPUT);
 // pinMode(PIN_D_PULSADOR_FUNCION, INPUT);

    //Inicializar variables pulsador
    pinMode(PIN_D_PULSADOR_FUNCION, INPUT_PULLUP);
    button_state = digitalRead(PIN_D_PULSADOR_FUNCION);
}

void fsm()
{
  //---Display---
  //LCD.clear();

  //---Potenciometro--
  //Obtenemos la señal del potenciometro
  BRILLO = analogRead(PIN_A_SENSOR_SONIDO);
  //desde 0 a 4095
  //Dividimos la señal en entre 16 para replicar el rango 0-255
  BRILLO = (BRILLO / 16);

  //Encendemos el led
  analogWrite(PIN_P_ACTUADOR_LED_SONIDO, BRILLO);

  
  
  LCD.setCursor(0, 0);
  // --Display--
  if(BRILLO > UMBRAL_SONIDO_ALTO)
  {
      LCD.println("SONIDO ALTO...");
  }

  if(BRILLO > UMBRAL_SONIDO_MEDIO && BRILLO < UMBRAL_SONIDO_ALTO)
  {
    LCD.println("SONIDO MEDIO...");
  }

  if(BRILLO > UMBRAL_SONIDO_BAJO && BRILLO < UMBRAL_SONIDO_MEDIO)
  {
    LCD.println("SONIDO BAJO...");
  }

  

  switch (MODO)
  {
    case 1 : 
    
             //---Pir---
             valorMovimiento = digitalRead(PIN_D_SENSOR_DISTANCIA);

              if (valorMovimiento == HIGH) 
              {
                digitalWrite(PIN_D_ACTUADOR_LED_MOVIMIENTO, HIGH);
              }
              else 
              {
                digitalWrite(PIN_D_ACTUADOR_LED_MOVIMIENTO, LOW);
              }

              LCD.setCursor(0, 0);
            if(BRILLO < UMBRAL_SONIDO_BAJO && valorMovimiento == HIGH)
             {
              LCD.println("MOVIMIENTO...");
             }

             LCD.setCursor(0, 1);
             LCD.println("Modo full");
             break;
    case 0 : 
             LCD.setCursor(0, 1);
             LCD.println("Modo Normal");
             break;
  }

}

void setup()
{
  start();
}

void loop()
{
    last_button_state = button_state;      // Estado pulsador anterior
    button_state = digitalRead(PIN_D_PULSADOR_FUNCION); // Estado actual pulsador
    
    if (last_button_state == HIGH && button_state == LOW) 
    {
      MODO = !MODO;
    }
    fsm();
  
}
