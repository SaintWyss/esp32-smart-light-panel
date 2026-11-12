<h1 align="center">esp32-smart-light-panel</h1>

<p align="center"><strong>Control táctil local de iluminación inteligente.</strong><br/>ESP32 · LVGL · Tuya LAN</p>

> Proyecto en desarrollo. El alcance y la validación de cada funcionalidad se detallan a continuación.


Panel táctil para controlar un foco inteligente desde una Waveshare ESP32-S3-Touch-LCD-7 Rev. 1.2 (800×480, N16R8). Esta versión documenta una configuración de hardware concreta; la compatibilidad con otros focos y placas requiere validación.

## Tecnologías, arquitectura y diseño

El firmware usa **C++17**, **Arduino para ESP32**, **PlatformIO** y **LVGL 8** sobre una pantalla táctil ESP32-S3. La comunicación local utiliza **Wi-Fi** y **Tuya LAN 3.3**; la configuración del dispositivo se guarda en NVS.

La interfaz depende del contrato `LightController`: la implementación Tuya gestiona la red y existe una implementación simulada para desarrollo. El procesamiento de red corre en una tarea separada de la UI. El diseño prioriza objetivos táctiles grandes, vistas de luz, color y temporizador, y actualizaciones localizadas para reducir trabajo de renderizado. Las decisiones visuales se conservan en `design-system/`.

## Estado

- Interfaz LVGL v8 basada en el ejemplo oficial de Waveshare.
- Control de encendido y brillo con objetivos táctiles grandes.
- Refresco y lectura táctil a 10 ms, compilación `-O2` y actualización localizada durante el arrastre del brillo.
- Control local probado del foco LEDVANCE SMART+ WiFi mediante Tuya LAN 3.3.
- Wi-Fi y protocolo se ejecutan en una tarea separada para no bloquear la interfaz.
- UI v0.5 compacta con vistas separadas `LUZ`, `COLOR` y `TIMER`, sin sombras ni animaciones.
- Fuentes reducidas a 12–20 px y controles táctiles de 44 px o más.
- Selección local de blanco, rojo, naranja, verde, cian, azul y violeta.
- Temporizador único y visible para encender o apagar en 15, 30 o 60 minutos, con cancelación.
- El estado real se consulta periódicamente, pero LVGL sólo redibuja valores que cambiaron.
- El brillo responde localmente durante el arrastre y envía una única orden Tuya al soltar el dedo.
- La Flash de fábrica permanece respaldada fuera del proyecto.

## Integración

Google Home continúa vinculado al foco, pero no interviene entre la pantalla y la lámpara. El panel se comunica directamente con el dispositivo LEDVANCE/Tuya dentro de la red local, con menor latencia y sin depender de Internet para cada orden.

La interfaz sigue dependiendo de `LightController`, de modo que el proveedor puede reemplazarse sin rehacer la pantalla.

## Credenciales

Las credenciales no están incluidas en el código ni en el firmware distribuible. `tools/provision.py` las transmite una vez por USB serie y la clave Tuya temporal se elimina de la PC cuando la placa confirma el guardado.

La configuración queda en NVS dentro de la Flash del ESP32. Esta partición no está cifrada en la versión actual: quien tenga acceso físico y herramientas de lectura podría recuperarla. No publicar respaldos posteriores al aprovisionamiento.

## Compilar

```bash
platformio run
```

## Cargar manualmente

La carga debe hacerse sólo después de una compilación aprobada y con el respaldo de fábrica verificado:

```bash
platformio run --target upload
```

PlatformIO puede detectar el puerto automáticamente. Si selecciona otro dispositivo, indicá el puerto de tu placa con `--upload-port`. Una carga normal reemplaza el programa y conserva la partición NVS con la configuración; verificá siempre el entorno y el respaldo antes de cargar.

## Rendimiento

Se conserva el doble framebuffer/direct mode recomendado por Waveshare para evitar tearing. La UI mantiene una copia del último estado renderizado y no invalida etiquetas, estilos ni slider cuando sus valores siguen iguales. Durante un arrastre no se realizan transacciones de red. LVGL usa `memcpy`/`memset` del sistema, la red corre en el núcleo 0 y la UI en el núcleo 1. No se aplicó overclock.

Mediciones físicas v0.5: LVGL estable promedió 0,4 ms por ciclo, el táctil 0,4 ms y Tuya local respondió normalmente en 22–41 ms. Se eliminaron bordes dinámicos y radios que producían picos de 65–83 ms, y el sondeo bajó de cada 5 a cada 15 segundos.

El temporizador actual usa el reloj de ejecución del ESP32: funciona sin Internet, pero se pierde si la placa se reinicia o se queda sin alimentación.

## Estructura

- `src/main.cpp`: inicio de pantalla, táctil y LVGL.
- `src/smart_light_ui.cpp`: interfaz de usuario.
- `include/light_controller.h`: contrato para el foco.
- `src/tuya_light_controller.cpp`: conexión Wi-Fi, tarea de red y sincronización de estado.
- `src/tuya_protocol_33.cpp`: tramas Tuya LAN 3.3, AES-128-ECB y CRC32.
- `src/device_config.cpp`: configuración persistente por USB serie/NVS.
- `tools/provision.py`: aprovisionamiento sin guardar secretos en el proyecto.
- `src/mock_light_controller.cpp`: simulación conservada para desarrollo sin foco.
- `reference/waveshare-official/`: ejemplos DrawColorBar y LVGL originales.
- `lib/`: versiones de librerías incluidas por Waveshare.
- `design-system/`: decisiones visuales persistentes.

## Fuentes técnicas

- https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7
- https://docs.waveshare.com/docs/ESP32/ESP32-S3/ESP32-S3-Touch-LCD-7/Arduino/Arduino-LVGL-Demo
- https://developers.home.google.com/apis
- https://github.com/jasonacox/tinytuya
