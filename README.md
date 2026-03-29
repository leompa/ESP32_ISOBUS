🚜 ESP32 ISOBUS RPM Counter

Proyecto basado en ESP32 (ESP-IDF) para la lectura y cálculo de RPM a partir de sensores digitales, pensado para integrarse con sistemas ISOBUS / AgOpenGPS mediante comunicación UDP.

📌 Descripción

Este proyecto permite:

Leer señales digitales (pulsos) desde sensores (ej: inductivos, hall, etc.)
Detectar cambios de estado (flancos)
Calcular RPM en tiempo real
Enviar los datos por red (UDP)
Integrarse con sistemas externos como AgOpenGPS

Está diseñado para trabajar con múltiples sensores en paralelo.

⚙️ Características principales
Soporte para múltiples entradas (ej: 4 sensores)
Detección por cambio de estado (ANYEDGE)
Cálculo de tiempo entre pulsos usando esp_timer
Comunicación UDP (broadcast o IP dinámica)
Posibilidad de auto-detección de IP cliente (ej: handshake con AgOpenGPS)
Preparado para watchdog (detección de caída de conexión)
🧰 Hardware requerido
ESP32 (ej: esp32dev)
Sensores de pulso (Hall, inductivo, encoder, etc.)
Fuente de alimentación estable
Red WiFi disponible
🔌 Conexiones

Ejemplo de pines utilizados:

#define NUM_SENSORS 4

GPIO_NUM_27
GPIO_NUM_26
GPIO_NUM_25
GPIO_NUM_33

Cada pin recibe una señal digital (HIGH/LOW) del sensor.

🧠 Funcionamiento
Se detecta un cambio de estado en el GPIO
Se registra el timestamp con esp_timer_get_time()
Se calcula el tiempo entre pulsos
Se convierte a RPM:
RPM = 60 / (tiempo_entre_pulsos_en_segundos)
Se envía el valor por UDP
📡 Comunicación UDP
Puerto configurable
Puede trabajar en:
Broadcast (255.255.255.255)
IP específica (detectada automáticamente)

Ejemplo de flujo:

Se recibe mensaje "hello" desde AgOpenGPS
Se guarda la IP origen
Se envían datos directamente a esa IP
📁 Estructura del proyecto
├── src/
│   ├── main.cpp
│   ├── rpm_counter.cpp
│   └── vt_application.cpp
├── include/
│   └── rpm_counter.hpp
├── platformio.ini
🚀 Compilación

Usando PlatformIO:

pio run

Subir al dispositivo:

pio run --target upload

Monitor serie:

pio device monitor
🧪 Configuración WiFi

Se utiliza WiFiManager, por lo que:

El ESP32 crea un AP si no tiene credenciales
Te conectás desde el celular/PC
Configurás la red
🔄 Mejoras futuras
 Filtro de ruido en señales (debounce)
 Promedio de RPM
 Soporte para más sensores
 Integración directa ISOBUS (VT)
 Reconexión automática UDP
 Watchdog de comunicación
🧑‍💻 Autor

Leandro Abel Marchiori Piccato
Analista en Sistemas - Especialista en redes y sistemas embebidos
