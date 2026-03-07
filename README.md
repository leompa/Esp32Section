# ESP32 Section Control para AgOpenGPS - Versión Simplificada

Control de secciones (section control) externo para **AgOpenGPS** usando un ESP32.  
Versión ligera y simplificada pensada para setups básicos con pocos relés y sin switches por sección.

![ESP32 Board](https://via.placeholder.com/600x300.png?text=ESP32+Section+Control)  
*(Reemplaza esta imagen con una foto real de tu placa o esquema cuando subas el repo)*

## Características principales

- Control de hasta **3 secciones** (ampliable fácilmente)
- Dos pulsadores para:
  - Toggle **AUTO / MANUAL**
  - Toggle **MASTER ON / OFF** (apaga todas las secciones cuando está OFF)
- Comunicación UDP con AgOpenGPS (puertos 8888 listen / 9999 send)
- IP fija configurable + broadcast automático
- Watchdog básico: apaga secciones si pierde conexión con AOG
- Informa correctamente a AOG el modo (AUTO/MANUAL) y el estado del master vía overrides (onLo/offLo)
- Basado en el excelente trabajo original de **Daniel Desmartins** y la comunidad AgOpenGPS, pero **simplificado** para hardware mínimo

**Diferencias con la versión original (más completa):**
- Sin switches individuales por sección
- Sin portal WiFi captive ni multi-SSID (SSID y pass hardcoded por simplicidad)
- Sin LEDs de estado PWM
- Sin generador de pulsos para velocidad (se puede agregar fácilmente)
- Sin soporte para hidráulico, tramlines ni geo-stop
- Menos consumo de memoria y más fácil de entender/modificar

## Hardware recomendado

- ESP32 DevKit (DOIT, NodeMCU, etc.)
- 3 relés (active-high o active-low, ajustable en código)
- 2 pulsadores momentáneos (conectados a GND, con pull-up interno o externo)
- Pines sugeridos (cambiables):
  - Relés: GPIO 25, 26, 27
  - Pulsador AUTO: GPIO 16
  - Pulsador MASTER: GPIO 17

**Conexión básica** (ejemplo):
