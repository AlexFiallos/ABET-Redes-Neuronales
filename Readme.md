============================================================
SISTEMA DE CLASIFICACIÓN AUTOMÁTICA DE RESIDUOS
YOLOv8 + ESP32 + MQTT
============================================================

DESCRIPCIÓN GENERAL

Este proyecto implementa un sistema inteligente de clasificación
automática de residuos utilizando visión por computadora y control
embebido. El sistema es capaz de detectar y clasificar residuos
de tipo PAPER, PLASTIC y METAL en tiempo real.

La detección se realiza mediante un modelo YOLOv8 y la clasificación
física se ejecuta a través de un ESP32 que controla servomotores
mediante comunicación MQTT.


------------------------------------------------------------
ARQUITECTURA DEL SISTEMA
------------------------------------------------------------

Flujo general del sistema:

Cámara
  ↓
YOLOv8 (Detección de objetos)
  ↓
Filtrado por nivel de confianza
  ↓
Selección de clase dominante
  ↓
Publicación del comando vía MQTT
  ↓
ESP32 (Suscriptor MQTT)
  ↓
Control de servomotores
  ↓
Clasificación física del residuo


------------------------------------------------------------
TECNOLOGÍAS UTILIZADAS
------------------------------------------------------------

- Python 3
- YOLOv8 (Ultralytics)
- OpenCV
- MQTT (paho-mqtt)
- ESP32
- PubSubClient
- ESP32Servo
- WiFi
- Motor DC con driver L298N
- Servomotores


------------------------------------------------------------
ESTRUCTURA DEL PROYECTO
------------------------------------------------------------

proyecto-clasificador-residuos
|
|-- detection
|   |-- detector_yolo_mqtt.py
|   |-- modelo_yolo.pt
|
|-- esp32
|   |-- esp32_clasificador.ino


------------------------------------------------------------
FUNCIONAMIENTO DEL SISTEMA
------------------------------------------------------------

1. La cámara captura imágenes en tiempo real.
2. YOLOv8 detecta y clasifica los residuos.
3. Se filtran detecciones con confianza mayor al 50%.
4. Se selecciona la clase con mayor confianza.
5. Se envía un comando MQTT (PAPER, PLASTIC o METAL).
6. El ESP32 recibe el comando.
7. Los servomotores redirigen el residuo.
8. La banda transportadora mantiene el flujo continuo.


------------------------------------------------------------
RESULTADO FINAL
------------------------------------------------------------

El sistema permite clasificar residuos de manera automática,
eficiente y en tiempo real, integrando visión artificial,
comunicación inalámbrica y control embebido en una solución
funcional y escalable.


------------------------------------------------------------
AUTOR
------------------------------------------------------------

Proyecto académico desarrollado con YOLOv8 y ESP32.
