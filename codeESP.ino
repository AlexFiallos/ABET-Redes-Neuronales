#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// ============================
// 📡 CONFIGURACIÓN DE RED
// ============================
const char* ssid = "MecaUIDE";          //
const char* password = "mecatupapa";  // 
const long RECONEXION_INTERVALO = 5000; // Intervalo de reintento de conexión (5 segundos)
unsigned long ultimo_reintento_wifi = 0;

// ============================
// 🧠 CONFIGURACIÓN MQTT
// ============================
const char* mqtt_server = "10.22.77.55"; // Broker del script Python
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32_Clasificador_"; // Se le añade la MAC para hacerlo único
const char* mqtt_topic = "detreci"; // Tópico de clasificación

// ============================
// ⚙️ CONFIGURACIÓN DE PINES
// ============================
const int servoPin7 = 4; // Servo para 'Paper' (Puerta 1)
const int servoPin9 = 10; // Servo para 'Plastic' (Puerta 2)
const int motor_ENA = 0; // Habilitador del Motor
const int motor_IN1 = 1; // Dirección 1
const int motor_IN2 = 2; // Dirección 2

// ============================
// 🛠️ OBJETOS Y VARIABLES
// ============================
WiFiClient espClient;
PubSubClient client(espClient);
Servo servo7;
Servo servo9;

int servo7_pos = 0; // Posición actual del Servo 7
int servo9_pos = 0; // Posición actual del Servo 9

// ============================
// 📡 FUNCIONES DE CONEXIÓN
// ============================

/**
 * @brief Intenta conectar o reconectar a la red WiFi.
 */
void setup_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return; // Ya está conectado, no hace nada
  }

  // Si no está conectado y ha pasado el tiempo de espera
  if (millis() - ultimo_reintento_wifi > RECONEXION_INTERVALO) {
    ultimo_reintento_wifi = millis();
    Serial.println("❌ WiFi desconectado. Reintentando...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int max_intentos = 20; // 10 segundos
    while (WiFi.status() != WL_CONNECTED && max_intentos > 0) {
      delay(500);
      Serial.print(".");
      max_intentos--;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ WiFi conectado exitosamente!");
      Serial.print("Dirección IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n❌ Falló la conexión WiFi. Se reintentará.");
    }
  }
}

/**
 * @brief Intenta conectar o reconectar al broker MQTT.
 */
void reconnect_mqtt() {
  if (WiFi.status() != WL_CONNECTED) {
    // Si no hay WiFi, no se puede conectar a MQTT.
    Serial.println("⚠️ Esperando conexión WiFi para intentar MQTT...");
    return;
  }
  
  if (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    String client_id_str = String(mqtt_client_id) + WiFi.macAddress();
    
    // Intentar conectar
    if (client.connect(client_id_str.c_str())) {
      Serial.println("✅ MQTT conectado!");
      // Una vez conectado, nos suscribimos
      if (client.subscribe(mqtt_topic)) {
        Serial.print("✅ Suscrito al tópico: ");
        Serial.println(mqtt_topic);
        // Estado inicial de los servos
        moverServos("METAL");
      } else {
        Serial.println("❌ Falló la suscripción MQTT.");
      }
    } else {
      Serial.print("❌ Falló la conexión MQTT, rc=");
      Serial.print(client.state());
      Serial.println(". Reintentando en 5 segundos...");
      // Los códigos de estado MQTT son:
      // -1: Conexión rechazada: versión de protocolo inaceptable
      // -2: Conexión rechazada: identificador de cliente inválido
      // -3: Conexión rechazada: servidor no disponible
      // -4: Conexión rechazada: nombre de usuario o contraseña incorrectos
      // -5: Conexión rechazada: no autorizado
      delay(5000);
    }
  }
}

// ============================
// CONTROL DE SERVOS Y MOTOR
// ============================

/**
 * @brief Mueve los servos según el comando de detección.
 * @param comando: "PAPER", "PLASTIC", o "METAL".
 */
void moverServos(String comando) {
  if (comando == "PAPER") {
    servo7_pos = 60;
    servo9_pos = 135;
    Serial.println("➡️  COMANDO EJECUTADO: PAPER (Servo 7: 90°, Servo 9: 0°)");
  } else if (comando == "PLASTIC") {
    servo7_pos = 0;
    servo9_pos = 0;
    Serial.println("➡️  COMANDO EJECUTADO: PLASTIC (Servo 7: 0°, Servo 9: 90°)");
  } else if (comando == "METAL") {
    servo7_pos = 0;
    servo9_pos = 135;
    Serial.println("➡️  COMANDO EJECUTADO: METAL/DEFAULT (Servos: 0°)");
  } else {
    Serial.print("⚠️ Comando no reconocido: ");
    Serial.println(comando);
    return; // Mantener el último estado si el comando no es válido
  }
  
  // Escribir la posición a los servos
  servo7.write(servo7_pos);
  servo9.write(servo9_pos);
}

/**
 * @brief Configura e inicia el motor de la banda transportadora.
 */
void setup_motor() {
  pinMode(motor_ENA, OUTPUT);
  pinMode(motor_IN1, OUTPUT);
  pinMode(motor_IN2, OUTPUT);

  // Motor SIEMPRE en movimiento (dirección y velocidad fijas)
  digitalWrite(motor_IN1, HIGH); // Activa un sentido
  digitalWrite(motor_IN2, LOW);  // Desactiva el otro
  digitalWrite(motor_ENA, HIGH); // Habilita el driver (velocidad máxima)

  Serial.println("✅ Motor L298N configurado y girando constantemente.");
}


// ============================
// 📩 FUNCIÓN DE CALLBACK MQTT
// ============================
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("\n📥 Mensaje recibido en [");
  Serial.print(topic);
  Serial.print("]: ");
  
  // Convertir el payload a String
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  
  // **COMPROBACIÓN DE COMANDO VÁLIDO**
  moverServos(message);
}

// ============================
// 🚀 SETUP
// ============================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("--- INICIO DE SISTEMA ESP32 CLASIFICADOR ---");
  
  // --- Setup de Servos ---
  servo7.attach(servoPin7);
  servo9.attach(servoPin9);
  Serial.print("✅ Servos adjuntados a pines ");
  Serial.print(servoPin7);
  Serial.print(" y ");
  Serial.println(servoPin9);

  // --- Setup de Motor ---
  setup_motor(); 
  
  // --- Setup de WiFi ---
  setup_wifi(); // Intenta la primera conexión
  
  // --- Setup de MQTT ---
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ============================
// 🔄 LOOP
// ============================
void loop() {
  // 1. Verificar y reconectar WiFi si es necesario
  setup_wifi();

  // 2. Si hay WiFi, verificar y reconectar MQTT
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect_mqtt();
    }
    
    // 3. Procesar el tráfico MQTT (solo si está conectado)
    if (client.connected()) {
      client.loop(); // Esencial para mantener la conexión y recibir mensajes
    }
  }

  // El motor y los servos mantienen su último estado por sí mismos.
  delay(10);
}