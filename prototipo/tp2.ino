// ------------------------------------------------
// Librerías
// ------------------------------------------------
#include <Wire.h>
#include "rgb_lcd.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "PubSubClient.h"

// ------------------------------------------------
// Constantes
// ------------------------------------------------
#define UMBRAL_SONIDO_BAJO 250
#define UMBRAL_SONIDO_MEDIO 350
#define UMBRAL_SONIDO_ALTO 450
// ------------------------------------------------
// Pines sensores (A = analógico | D = Digital)
// ------------------------------------------------
#define PIN_D_SENSOR_DISTANCIA 15
#define PIN_A_SENSOR_SONIDO 34
#define PIN_D_PULSADOR_FUNCION 27
// ------------------------------------------------
// Pines actuadores (P = PWM | D = Digital)
// ------------------------------------------------
#define PIN_P_ACTUADOR_LED_SONIDO 32
#define PIN_D_ACTUADOR_LED_MOVIMIENTO 4
// ------------------------------------------------
// Parametros MQTT
// ------------------------------------------------
#define DESACTIVATE '0' 
#define ACTIVATE    '1'
// Dividimos la señal entre 16 canales para replicar el rango 0-255
#define CANAL 16
// ------------------------------------------------
// TEMPORIZADORES
// ------------------------------------------------
#define TIME_ACTIVE 3000
//Cantidad de funciones de verificación de sensores
#define CANT_FUNCIONES 3
//Posiciones del Display 16x2, filas y columnas
#define COL_0 0
#define ROW_0 0
#define ROW_1 1
// Constante para debounce
#define DEBOUNCE_DELAY 50  // 50 milisegundos de debounce
//
#define ACTIVO true

rgb_lcd lcd;

//Configuraciones de tópicos
const char* ssid        = "SO Avanzados";
const char* password    = "SOA.2019";
const char* mqttServer  = "broker.emqx.io";
const char* user_name   = "";
const char* user_pass   = "";
const char * topic_mov = "/notif/movimiento";
int port = 1883;
char clientId[50];

WiFiClient espClient;
PubSubClient client(espClient);

//Características del PWM
const int frecuencia = 1000;
const int resolucion = 10;
//timer
unsigned int time_before;

//Pulsador estados
int button_state = 0;       // the current state of button
int last_button_state = 0;  // the previous state of button

// Agregar una variable para almacenar el tiempo de la última lectura del pulsador.
unsigned long last_debounce_time = 0; 

//Mensajes tópicos
unsigned long lastPublishTime = 0;  // Almacena el tiempo de la última publicación
unsigned long publishInterval = 5000;  // Intervalo de 5 segundos entre publicaciones

WiFiUDP ntpUDP;
//Configuración de Tiempo actual en argentina.
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);  // UTC-3 para Buenos Aires

// Estados
enum e_estados {
  ESTADO_ESCUCHANDO_FULL,
  ESTADO_ESCUCHANDO_SONIDO,
  ESTADO_ACTIVO
};

// Eventos
enum e_eventos {
  EVENTO_RUIDO_BAJO,
  EVENTO_RUIDO_MEDIO,
  EVENTO_RUIDO_FUERTE,
  EVENTO_MOVIMIENTO,
  EVENTO_PULSADOR,
  EVENTO_TIMER,
  EVENTO_NULO
};

typedef struct evento_s
{
  e_eventos tipo;
  int valor;
} evento_t;

typedef struct sensor_pir
{
  int valorMovimiento;
  boolean activo;
} s_pir;

typedef struct sensor_sonido
{
  int valorSonido = 0;
} s_sonido;

e_estados estado_actual;
evento_t evento;
//Estructura del sensor de movimiento
s_pir sensorMov;
//Estructura del sensor de sonido
s_sonido sensorSonido;
//Índice de funciones
int indice = 0;
//Último valor del potenciómetro
int last_value_potentiometer = 0;
//Detector de movimiento activo
int pir_activo = 0;

void start()
{
  Serial.begin(9600);
  // Display
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setRGB(30, 125, 100);
  lcd.setCursor(0, 0);
  //Setup Wifi
  wifiConnect();

  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

  client.setServer(mqttServer, port);
  client.setCallback(callback);

  mostrar_por_pantalla("ESCUCHANDO FULL", COL_0, ROW_1);
  //Inicializamos las características del pwm
  ledcAttach(PIN_P_ACTUADOR_LED_SONIDO, frecuencia, resolucion);
  //Led Movimiento
  pinMode(PIN_D_ACTUADOR_LED_MOVIMIENTO, OUTPUT);
  //PIR
  pinMode(PIN_D_SENSOR_DISTANCIA, INPUT);
  //Pulsador
  pinMode(PIN_D_PULSADOR_FUNCION, INPUT_PULLUP);
  button_state = digitalRead(PIN_D_PULSADOR_FUNCION);

  estado_actual = ESTADO_ESCUCHANDO_FULL;
  evento.tipo = EVENTO_NULO;
  sensorMov.activo = ACTIVO;
  time_before = millis();
  //Inicialización cliente NTP  
  timeClient.begin();
}

/*
* Setup de configuración del WiFi
*/
void wifiConnect()  
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
}

//Funcion Callback que recibe los mensajes enviados por lo dispositivos
void callback(char* topic, byte* message, unsigned int length) 
{
  Serial.print("Se recibio mensaje en el topico: ");
  Serial.println(topic);
  Serial.print("Mensaje Recibido: ");

  String stMessage;
  
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char) message[i]);
    stMessage += (char) message[i];
  }

  Serial.println(stMessage);
}

void mqttReconnect() 
{
  if(!client.connected())
  {
    Serial.print("Intentando conexión MQTT...");
    long r = random(1000);
    sprintf(clientId, "clientId-%ld", r);
    if (client.connect(clientId,user_name,user_pass)) 
    {
      Serial.print(clientId);
      Serial.println(" conectado");
      Serial.println("envio");
      client.subscribe(topic_mov);
    } 
    else 
    {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.println("intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

/*
* Leer el sensor PIR
*/
int leer_sensor_movimiento()
{
  return digitalRead(PIN_D_SENSOR_DISTANCIA);
}

/*
* Leer el valor del potenciometro
*/
int leer_sensor_sonido()
{
  return analogRead(PIN_A_SENSOR_SONIDO);
}

/*
* Actualizar los eventos segun el valor del potenciómetro
*/
void actualizar_evento_potenciometro(e_eventos tipo, int value)
{
  evento.tipo = tipo;
  last_value_potentiometer = value;
  time_before = millis();
}

/*
* Leer el registro del pulsador.
*/
int leer_pulsador()
{
  return digitalRead(PIN_D_PULSADOR_FUNCION);
}

/*
* Verificar la pulsación.
*/
void verificar_pulsador()
{
  int lectura_actual = leer_pulsador(); // Leer el estado actual del botón
  
  // Si el estado del botón cambió
  if (lectura_actual != last_button_state) {
    // Reiniciar el tiempo de debounce
    last_debounce_time = millis();
  }

  // Verificar si ha pasado el tiempo de debounce
  if ((millis() - last_debounce_time) > DEBOUNCE_DELAY) 
  {
    // Solo cambiar el estado si el botón cambió de estado
    if (lectura_actual != button_state) 
    {
      button_state = lectura_actual;
      // Si el estado del botón es LOW, significa que fue presionado
      if (button_state == LOW) 
      {
        sensorMov.activo = !sensorMov.activo;
        evento.tipo = EVENTO_PULSADOR;
      }
    }
  }
  // Guardar el estado actual del botón para la próxima lectura
  last_button_state = lectura_actual;
}

/*
* Verificar la activación del potenciómetro
*/
void verificar_sensor_sonido()
{
  sensorSonido.valorSonido = leer_sensor_sonido();

  int brillo = sensorSonido.valorSonido / CANAL;

  if (brillo != last_value_potentiometer) 
  {
    if (brillo >= UMBRAL_SONIDO_ALTO) 
    {
      actualizar_evento_potenciometro(EVENTO_RUIDO_FUERTE, brillo);
    } 
    else if (brillo >= UMBRAL_SONIDO_MEDIO && brillo < UMBRAL_SONIDO_ALTO) 
    {
      actualizar_evento_potenciometro(EVENTO_RUIDO_MEDIO, brillo);
    } 
    else if (brillo >= UMBRAL_SONIDO_BAJO && brillo < UMBRAL_SONIDO_MEDIO) 
    {
      actualizar_evento_potenciometro(EVENTO_RUIDO_BAJO, brillo);
    }
  }
}

/*
* Verificar la activación del sensor de movimiento PIR
*/
void verificar_sensor_movimiento()
{
  sensorMov.valorMovimiento = leer_sensor_movimiento();
  unsigned long currentMillis = millis();
  if (sensorMov.valorMovimiento == HIGH)
  {
    evento.tipo = EVENTO_MOVIMIENTO;
    if (currentMillis - lastPublishTime >= publishInterval) {
      String message = "Movimiento detectado a las " + timeClient.getFormattedTime();
      char msg[100];
      message.toCharArray(msg, 100);
      client.publish(topic_mov, msg);
      lastPublishTime = currentMillis;
    }
    time_before = millis();
  }
}

/*
* Endender los leds según el evento registrado.
*/
void encender_led(e_eventos evento)
{
  switch (evento)
  {
    case EVENTO_MOVIMIENTO:
      digitalWrite(PIN_D_ACTUADOR_LED_MOVIMIENTO, HIGH);
      break;
    case EVENTO_NULO:
      analogWrite(PIN_P_ACTUADOR_LED_SONIDO, LOW);
      break;
    case EVENTO_TIMER:
      digitalWrite(PIN_D_ACTUADOR_LED_MOVIMIENTO, LOW);
      analogWrite(PIN_P_ACTUADOR_LED_SONIDO, 0);
      break;
    default:
      analogWrite(PIN_P_ACTUADOR_LED_SONIDO, sensorSonido.valorSonido / CANAL);
      break;
  }
}

/*
* Mostrar en el display el evento registrado.
*/
void mostrar_por_pantalla(String mensaje, int colLCD, int rowLCD)
{
  lcd.setCursor(colLCD, rowLCD);
  lcd.println(mensaje);
}

//Array de funciones de verificación
void (*verificar_sensor[CANT_FUNCIONES])() = {verificar_sensor_movimiento, verificar_sensor_sonido, verificar_pulsador};

/*
* Obtener el evento activado.
*/
void tomar_evento()
{
  unsigned int time_after = millis();
  if ( (time_after - time_before) > TIME_ACTIVE && estado_actual == 2)
  {
    evento.tipo = EVENTO_TIMER;
    time_before = time_after;
  }
  verificar_sensor[indice]();
  indice = ++indice % CANT_FUNCIONES;
}

/*
* Transición de estados utilizando el patrón de diseño 
* de Máquina de estados
*/
void fsm()
{
  tomar_evento();

  switch (estado_actual)
  {
    case ESTADO_ESCUCHANDO_FULL:
      switch (evento.tipo)
      {
        case EVENTO_RUIDO_BAJO:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO BAJO...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_RUIDO_MEDIO:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO MEDIO...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_RUIDO_FUERTE:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO FUERTE...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_MOVIMIENTO:
          pir_activo = 1;
          encender_led(evento.tipo);
          mostrar_por_pantalla("MOVIMIENTO...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_PULSADOR:
          if(sensorMov.activo)
          {
            mostrar_por_pantalla("ESCUCHANDO FULL", COL_0, ROW_1);
            estado_actual = ESTADO_ESCUCHANDO_FULL;
          }
          else
          {
            mostrar_por_pantalla("MOVIMIENTO DESACTIVADO", COL_0, ROW_1);
            estado_actual = ESTADO_ESCUCHANDO_SONIDO;
          }
          break;
        case EVENTO_NULO:
          encender_led(evento.tipo);
          estado_actual = ESTADO_ESCUCHANDO_FULL;
          break;
      }
      break;
    case ESTADO_ESCUCHANDO_SONIDO:
      switch (evento.tipo)
      {
        case EVENTO_RUIDO_BAJO:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO BAJO...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_RUIDO_MEDIO:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO MEDIO...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
          case EVENTO_RUIDO_FUERTE:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO FUERTE...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_PULSADOR:
          if(sensorMov.activo)
          {
            mostrar_por_pantalla("ESCUCHANDO FULL", COL_0, ROW_1);
            estado_actual = ESTADO_ESCUCHANDO_FULL;
          }
          else
          {
            mostrar_por_pantalla("MOVIMIENTO DESACTIVADO", COL_0, ROW_1);
            estado_actual = ESTADO_ESCUCHANDO_SONIDO;
          }
          break;
      }
      break;
    case ESTADO_ACTIVO:
      switch (evento.tipo)
      {
        case EVENTO_RUIDO_BAJO:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO BAJO...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_RUIDO_MEDIO:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO MEDIO...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_RUIDO_FUERTE:
          encender_led(evento.tipo);
          mostrar_por_pantalla("SONIDO FUERTE...", COL_0, ROW_0);
          estado_actual = ESTADO_ACTIVO;
          break;
        case EVENTO_MOVIMIENTO:
          if (sensorMov.activo && !pir_activo) 
          {
            pir_activo = 1;
            encender_led(evento.tipo);
            mostrar_por_pantalla("MOVIMIENTO...", COL_0, ROW_0);
            estado_actual = ESTADO_ESCUCHANDO_FULL;
          }
          break;
        case EVENTO_TIMER:
          pir_activo = 0;
          mostrar_por_pantalla("                ", COL_0, ROW_0);
          encender_led(evento.tipo);
          if(sensorMov.activo)
          {
            estado_actual = ESTADO_ESCUCHANDO_FULL;
          }
          else
          {
            estado_actual = ESTADO_ESCUCHANDO_SONIDO;
          }
          break;
        case EVENTO_PULSADOR:
          if(sensorMov.activo)
          {
            mostrar_por_pantalla("ESCUCHANDO FULL", COL_0, ROW_1);
            estado_actual = ESTADO_ESCUCHANDO_FULL;
          }
          else
          {
            mostrar_por_pantalla("MOVIMIENTO DESACTIVADO", COL_0, ROW_1);
            estado_actual = ESTADO_ESCUCHANDO_SONIDO;
          }
          break;
      }
      break;
  }
}

void setup()
{
  start();
}

void loop()
{
  timeClient.update();
  //Reconexión mqtt
  mqttReconnect();

  fsm();
}
