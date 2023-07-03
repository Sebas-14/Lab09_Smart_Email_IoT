/* 
           UNIVERSIDAD TECNICA DEL NORTE
      FACULTAD DE INGENIERIA EN CIENCIAS APLICADAS
           CARRERA DE TELECOMUNICACIONES

  INTEGRANTES:
  - Donoso Fabricio
  - Farinango Wilmer
  - Muñoz Pablo
  - Quelal Rony
  - Tobar Anahi
  - Yepez Jhon

*/ 

// Importacion de librerias
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "CTBot.h"

// Declaración de constantes y variables
const char* ssid = "BROKEN"; // Nombre de la red Wi-Fi a la que se conectará el ESP8266
const char* password = "@broken@"; // Contraseña de la red Wi-Fi

DHT dht(4, DHT11); // Instancia del sensor DHT11 conectado al pin 4

CTBot myBot; // Objeto para manejar el bot de Telegram
TBMessage msg; // Objeto para almacenar los mensajes de Telegram

String token = "6390106186:AAG_prho45RKEGzbS-HdDKW9DQEeBYApICw";
long id = 5307090358;

unsigned long CurrentTime = 0;
unsigned long SensorTime = 0;
unsigned long TelegramTime = 0;

# define SensorRefresh 1000 // Intervalo de tiempo para refrescar la lectura del sensor (en milisegundos)
# define TelegramRefresh 1000 // Intervalo de tiempo para enviar mensajes a Telegram (en milisegundos)
# define Change 1

bool llave = false;

#define DHTTYPE    DHT22 // Tipo de sensor DHT a utilizar (DHT22 en este caso)

float h, t; // Variables para almacenar la humedad y la temperatura

AsyncWebServer server(80); // Crear un servidor web en el puerto 80

unsigned long previousMillis = 0; // Almacenar el tiempo anterior en milisegundos

const long interval = 10000; // Intervalo de tiempo para actualizar las lecturas del sensor (10 segundos)

// Pagina web
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Montserrat;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP8266 DHT Server - Equipo 1</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperatura: </span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humedad: </span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
    <img src="https://wstemproject.eu/files/2019/06/logo-UTN-2017-300x300.png" alt="Logo UTN">
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var) {
  //Serial.println(var);
  if (var == "TEMPERATURE") {
    return String(t);
  }
  else if (var == "HUMIDITY") {
    return String(h);
  }
  return String();
}

void setup() {
  Serial.begin(115200); // Inicializa la comunicación serial a una velocidad de 115200 baudios
  dht.begin(); // Inicializa el sensor DHT

  WiFi.begin(ssid, password); // Conecta el ESP8266 a la red Wi-Fi especificada
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Servidor web: ");
  Serial.println(WiFi.localIP());

  // Configuración de rutas del servidor web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(h).c_str());
  });

  server.begin(); // Inicia el servidor web

  Serial.println("Iniciando TelegramBot...");

  // Conexión del bot de Telegram a la red Wi-Fi
  myBot.wifiConnect(ssid, password);

  myBot.setTelegramToken(token); // Establece el token del bot de Telegram
  if (myBot.testConnection()) {
    Serial.println("----- Conexion OK ----");
  } else {
    Serial.println("----- Conexion NOK ----");
  }
}

// Calcula y devuelve una medida
float GetMeasure() {
  return h * Change;  // Calcula y devuelve una medida basada en la humedad multiplicada por el valor de Change
}

void sensor() {
  if (CTBotMessageText == myBot.getNewMessage(msg)) {  // Verifica si hay un nuevo mensaje de Telegram
    if (msg.text.equalsIgnoreCase("Status")) {  // Comprueba si el texto del mensaje es "Status" (ignorando mayúsculas y minúsculas)
      myBot.sendMessage(msg.sender.id, "Hola " + msg.sender.firstName
                        + "\nt: " + String(t)
                        + "\nh: " + String(h)
                        + "\nValores normales");  // Envía un mensaje de respuesta con la temperatura, humedad y un mensaje indicando que los valores son normales
      Serial.println("\nh: " + String(h) + "\nMensaje Enviado");
    } else {
      String reply = msg.sender.firstName + (String)" no reconozco lo que enviaste, por favor intenta ingresando Status.";
      myBot.sendMessage(msg.sender.id, reply);  // Envía un mensaje de respuesta indicando que el comando no es reconocido y sugiere ingresar "Status"
      Serial.println("Mensaje no reconocido");
    }
    delay(500);  // Retraso de 500 milisegundos
  }

  if (h > 60) {  // Verifica si la humedad es mayor que 60
    if (llave == false) {  // Verifica si la variable llave es falsa
      llave = true;  // Cambia el valor de llave a verdadero
      myBot.sendMessage(msg.sender.id, "¡Precaución! " + msg.sender.firstName
                        + "\nTemperatura: " + String(t)
                        + "\nHumedad: " + String(h)
                        + "\nValores alarmantes");  // Envía un mensaje de precaución indicando la temperatura, humedad y que los valores son alarmantes
      Serial.println("Valor alarmante " + String(h) + "\nAlerta Enviada");
    }
  }

  if (h <= 60) {  // Verifica si la humedad es menor o igual a 60
    if (llave == true) {  // Verifica si la variable llave es verdadera
      llave = false;  // Cambia el valor de llave a falso
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();  // Obtiene el tiempo actual en milisegundos
  if (currentMillis - previousMillis >= interval) {  // Verifica si ha pasado el intervalo de tiempo establecido

    previousMillis = currentMillis;  // Actualiza el valor de previousMillis al tiempo actual

    float newT = dht.readTemperature();  // Lee la temperatura del sensor DHT

    if (isnan(newT)) {  // Verifica si la lectura de temperatura es inválida
      Serial.println("Failed to read from DHT sensor!");  // Imprime un mensaje de error en el puerto serie
    }
    else {
      t = newT;  // Asigna el nuevo valor de temperatura a la variable t
      Serial.println("Temperatura: " + String(t)); // Imprime la temperatura en el puerto serie
    }
    // Leer la humedad
    float newH = dht.readHumidity();  // Lee la humedad del sensor DHT

    if (isnan(newH)) {  // Verifica si la lectura de humedad es inválida
      Serial.println("Failed to read from DHT sensor!");  // Imprime un mensaje de error en el puerto serie
    }
    else {
      h = newH;  // Asigna el nuevo valor de humedad a la variable h
      Serial.println("Humedad: " + String(h)); // Imprime la humedad en el puerto serie
    }
  }
  CurrentTime = millis();  // Actualiza el valor de CurrentTime con el tiempo actual en milisegundos

  if ((CurrentTime - SensorTime) >= SensorRefresh)  // Verifica si ha pasado el tiempo de actualización del sensor
  {
    h = GetMeasure();  // Actualiza la variable h con una nueva medida calculada
    SensorTime = CurrentTime;  // Actualiza el valor de SensorTime al tiempo actual
  }

  if ((CurrentTime - TelegramTime) >= TelegramRefresh)  // Verifica si ha pasado el tiempo de actualización de Telegram
  {
    sensor();  // Llama a la función sensor() para procesar los mensajes y enviar alertas si es necesario
    TelegramTime = CurrentTime;  // Actualiza el valor de TelegramTime al tiempo actual
  }
}
