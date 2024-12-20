#include <ESP8266WiFi.h>        // Librería para Wi-Fi
#include <ESP8266HTTPClient.h> // Librería para enviar datos HTTP
#include <DHT.h>               // Librería para el sensor DHT11

// Credenciales Wi-Fi
const char* ssid = "Fenix";
const char* password = "rainbow6";

// Configuración de ThingSpeak
const char* serverName = "http://api.thingspeak.com/update";
const char* apiKey = "NFBU6FHGH62L3HV3"; // Reemplaza con tu API Key de ThingSpeak

WiFiServer server(80); // Configurar un servidor web en el puerto 80

// Configuración del DHT11
#define DHTPIN D2  // Pin al que está conectado el DATA del DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configuración del buzzer
const int buzzerPin = D1; // Pin del buzzer
bool buzzerState = false; // Estado del buzzer

// Umbral de temperatura para alarma
const float TEMPERATURA_UMBRAL = 50.0;

void setup() {
  Serial.begin(115200); // Inicializa comunicación serial
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Apaga el buzzer al inicio

  // Inicia el DHT11
  dht.begin();

  // Conectar a la red Wi-Fi
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { //Esperar conexión Wi-Fi
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConexión Wi-Fi establecida.");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  server.begin(); //Inicia el servidor web
}

void loop() {
  WiFiClient client = server.available(); //Esperar cliente
  if (client) {
    Serial.println("Nuevo cliente conectado.");
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // Procesar solicitud
    if (request.indexOf("/BUZZER=ON") != -1) {
      digitalWrite(buzzerPin, HIGH); //Encender buzzer
      buzzerState = true;
      enviarAThingSpeak(buzzerState, -1, -1); //estado del buzzer
    }
    if (request.indexOf("/BUZZER=OFF") != -1) {
      digitalWrite(buzzerPin, LOW); // Apagar buzzer
      buzzerState = false;
      enviarAThingSpeak(buzzerState, -1, -1); //estado del buzzer
    }

    // Responder al cliente
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<h1>Control del Buzzer</h1>");
    client.println("<p><a href=\"/BUZZER=ON\">Encender Buzzer</a></p>");
    client.println("<p><a href=\"/BUZZER=OFF\">Apagar Buzzer</a></p>");
    client.println("</html>");
    client.stop();
    Serial.println("Cliente desconectado.");
  }

  // Leer temperatura y humedad
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // Verificar si hay un error en la lectura
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error al leer el sensor DHT11");
    return;
  }

  // Mostrar datos en el monitor serial
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");

  // Activar alarma si supera el umbral
  if (temperatura > TEMPERATURA_UMBRAL) {
    digitalWrite(buzzerPin, HIGH); // Encender buzzer
    buzzerState = true;
  }

  // Enviar datos a ThingSpeak
  enviarAThingSpeak(buzzerState, temperatura, humedad);

  delay(15000); // Enviar datos cada 15 segundos
}

// Función para enviar datos a ThingSpeak
void enviarAThingSpeak(bool estado, float temperatura, float humedad) {
  if (WiFi.status() == WL_CONNECTED) { // Verificar conexión Wi-Fi
    WiFiClient client;
    HTTPClient http;

    // Preparar URL
    String url = String(serverName) + "?api_key=" + apiKey +
                 "&field1=" + (estado ? "1" : "0") +
                 "&field2=" + String(temperatura) +
                 "&field3=" + String(humedad);

    Serial.println("Enviando datos a ThingSpeak: " + url);

    http.begin(client, url); // Conectar al servidor
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.println("Respuesta de ThingSpeak: " + String(httpCode));
      Serial.println(http.getString());
    } else {
      Serial.println("Error al enviar datos: " + String(http.errorToString(httpCode).c_str()));
    }

    http.end();
  } else {
    Serial.println("Wi-Fi no conectado, no se pueden enviar datos.");
  }
}
