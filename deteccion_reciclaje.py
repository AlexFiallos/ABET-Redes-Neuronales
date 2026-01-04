import time
import cv2
import numpy as np
import paho.mqtt.client as mqtt
from ultralytics import YOLO
from ultralytics.utils.plotting import colors
import argparse
import os
from datetime import datetime
import csv

# ============================
# ARGUMENTOS DEL SCRIPT
# ============================
parser = argparse.ArgumentParser()
parser.add_argument("--model", type=str, required=True, help="ruta del modelo YOLO .pt")
parser.add_argument("--source", type=str, default="0", help="camara, video, imagen o carpeta")
parser.add_argument("--resolution", type=int, default=640, help="tamaño de entrada YOLO")
args = parser.parse_args()

# ============================
# CONFIGURACIÓN MQTT
# ============================
broker = "10.22.77.55"
# *** TÓPICO MODIFICADO A "detreci" ***
topic = "detreci"

client = mqtt.Client()
client.connect(broker, 1883, 60)

# ============================
# CARGAR MODELO
# ============================
model = YOLO(args.model)

# ============================
# CARGAR FUENTE
# ============================
source = args.source
if source.isnumeric():
    source = int(source)

cap = cv2.VideoCapture(source)
if not cap.isOpened():
    raise Exception(f"No se pudo abrir el source: {args.source}")

# ============================
# TIMERS
# ============================
tiempo_bloqueo = 5 # Bloqueo reducido para pruebas, puedes volver a 10*60 (10 min)
ultima_activacion = 0

# ============================
# FPS
# ============================
fps_buffer = []
longitud_buffer = 30

print("🧠 Sistema YOLO + MQTT listo...")

# ============================
# === HISTORIAL ===
# ============================
hist_dir = "historial"
os.makedirs(hist_dir, exist_ok=True)

csv_path = os.path.join(hist_dir, "activaciones.csv")

# Crear CSV si no existe
if not os.path.exists(csv_path):
    with open(csv_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["fecha_hora", "archivo_imagen", "objetos_detectados", "comando_mqtt", "comentario"])


def guardar_historial(frame, object_count, comando_mqtt):
    """Guarda foto + registro CSV."""
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    filename = f"{timestamp}_detectado.jpg"
    img_path = os.path.join(hist_dir, filename)

    # Guardar imagen
    cv2.imwrite(img_path, frame)

    # Guardar entrada en CSV
    with open(csv_path, "a", newline="") as f:
        writer = csv.writer(f)
        writer.writerow([
            datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            filename,
            object_count,
            comando_mqtt,
            "Activación por detección válida"
        ])

    print(f"📸 Historial guardado: {filename}")


# ============================
# LOOP PRINCIPAL
# ============================
while True:
    t_inicio = time.perf_counter()

    ret, frame = cap.read()
    if not ret:
        break

    # Inferencia
    results = model(frame, verbose=False, imgsz=args.resolution)
    detections = results[0].boxes
    labels = model.names

    object_count = 0
    
    # ----------------------------------------------------
    # LÓGICA DE CLASIFICACIÓN Y SELECCIÓN DE COMANDO
    # ----------------------------------------------------
    # Diccionario para almacenar la confianza más alta por clase
    max_confianza_por_clase = {}

    for det in detections:
        conf = det.conf.item()
        if conf < 0.5:
            continue

        class_id = int(det.cls.item())
        label_name = labels[class_id].upper() # Nombre de la clase en mayúsculas

        # Almacenar la confianza más alta para la clase actual
        if label_name in max_confianza_por_clase:
            max_confianza_por_clase[label_name] = max(max_confianza_por_clase[label_name], conf)
        else:
            max_confianza_por_clase[label_name] = conf
            
        # Dibujo del bounding box y etiqueta
        xyxy = det.xyxy.cpu().numpy().astype(int).squeeze()
        x1, y1, x2, y2 = xyxy
        color = colors(class_id, True)
        
        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
        cv2.putText(frame, f"{label_name} {conf*100:.1f}%", (x1, y1 - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)
        
        object_count += 1
        
    comando_mqtt = None
    
    if max_confianza_por_clase:
        # Encontrar la clase con la confianza más alta de todas las detectadas
        clase_mas_confiable = max(max_confianza_por_clase, key=max_confianza_por_clase.get)
        
        # Mapear la clase al comando que espera el ESP32
        if "PAPER" in clase_mas_confiable:
            comando_mqtt = "PAPER"
        elif "PLASTIC" in clase_mas_confiable:
            comando_mqtt = "PLASTIC"
        elif "METAL" in clase_mas_confiable:
            comando_mqtt = "METAL"
        
        # Muestra en la imagen la detección más relevante
        if comando_mqtt:
            cv2.putText(frame, f"Comando: {comando_mqtt}",
                        (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 0), 2)


    cv2.putText(frame, f"Objetos detectados: {object_count}",
                (10, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,255,255), 2)

    # -------------------------------
    # DETECCIÓN + BLOQUEO + ENVÍO MQTT
    # -------------------------------
    tiempo_actual = time.time()

    # Si hay una detección válida que mapea a un comando y no está en bloqueo
    if comando_mqtt and (tiempo_actual - ultima_activacion > tiempo_bloqueo):
        print(f"🎯 DETECCIÓN VÁLIDA ({comando_mqtt}) — enviando señal MQTT")
        client.publish(topic, comando_mqtt) # Envía el comando (ej: "PAPER")
        ultima_activacion = tiempo_actual

        # === GUARDAR EN HISTORIAL ===
        guardar_historial(frame, object_count, comando_mqtt)


    # ============================
    # FPS
    # ============================
    fps = 1 / (time.perf_counter() - t_inicio)
    fps_buffer.append(fps)
    if len(fps_buffer) > longitud_buffer:
        fps_buffer.pop(0)

    fps_promedio = np.mean(fps_buffer)

    cv2.putText(frame, f"FPS: {fps_promedio:.2f}",
                (10, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0,255,255), 2)

    # Mostrar ventana
    cv2.imshow("YOLO + MQTT", frame)

    # ESC para salir
    if cv2.waitKey(1) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()