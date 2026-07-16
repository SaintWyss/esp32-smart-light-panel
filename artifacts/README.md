# Artefactos

## smart-light-panel-color-timers-v0.5.0.bin

- Fecha: 2026-07-16
- Placa: Waveshare ESP32-S3-Touch-LCD-7 Rev.1.2, N16R8
- Estado: compilado, cargado, escritura verificada, Wi-Fi y consulta Tuya local verificadas
- Cambio: fuentes menores; navegación LUZ/COLOR/TIMER; siete tonos; timers de encendido/apagado
- Rendimiento: acciones al presionar, etiquetas estáticas, rectángulos planos, consulta cada 15 s
- Color: órdenes reales respondieron correctamente en 31–41 ms
- Timer: armado/cancelación implementados; pendiente prueba física completa esperando el vencimiento
- Tamaño: 1.354.304 bytes
- SHA-256: `52918eda8ca60bcfb29678dfb6ff59a607a039b23efd9f2fe8cf744c2e473e9c`

## smart-light-panel-minimal-v0.4.0.bin

- Fecha: 2026-07-16
- Placa: Waveshare ESP32-S3-Touch-LCD-7 Rev.1.2, N16R8
- Estado: compilado, cargado, escritura verificada, Wi-Fi conectado y consulta Tuya local verificada
- Cambio: rediseño minimalista completo; render diferencial; brillo local durante arrastre y una orden al soltar
- Accesibilidad: contraste corregido, estado expresado con texto y objetivos táctiles de 48 px o más
- Credenciales: no incluidas en el binario; la configuración existente permanece en NVS
- Tamaño: 1.408.128 bytes
- SHA-256: `b06d44a508dc6b8b626e50c2427f0b99661ea082a3e3e32fcb8a3a706cb0f20f`

## smart-light-panel-ledvance-tuya-v0.3.0.bin

- Fecha: 2026-07-16
- Placa: Waveshare ESP32-S3-Touch-LCD-7 Rev.1.2, N16R8
- Estado: compilado, cargado, escritura verificada, Wi-Fi conectado y consulta Tuya local verificada
- Cambio: controlador real LEDVANCE/Tuya LAN 3.3, aprovisionamiento seguro por USB y tarea de red no bloqueante
- Credenciales: no incluidas en el binario; se guardan después en NVS
- Tamaño: 1.396.352 bytes
- SHA-256: `f3e7ef5eb9b205dfdcf5e9c0fdf036520f94c2112004b9dd4d6df1577f0bf823`

## smart-light-panel-mock-v0.2.0.bin

- Fecha: 2026-07-16
- Placa: Waveshare ESP32-S3-Touch-LCD-7 Rev.1.2, N16R8
- Estado: compilado, cargado, escritura verificada y arranque serial verificado
- Cambio: refresco/táctil a 10 ms, compilación `-O2` y slider con actualización localizada
- Proveedor del foco: simulación local; no controla todavía un dispositivo real
- Tamaño: 779.136 bytes
- SHA-256: `eb6f4f3fcb2128a2bc983bab45b0ce68f7d551181064b91cf29cf78a0343b285`

## smart-light-panel-mock-v0.1.0.bin

- Fecha: 2026-07-16
- Placa: Waveshare ESP32-S3-Touch-LCD-7 Rev.1.2, N16R8
- Estado: compilado, cargado y arranque serial verificado
- Proveedor del foco: simulación local; no controla todavía un dispositivo real
- Tamaño: 667.952 bytes
- SHA-256: `45b7739127a10dfe3792f5192b400a39cf7e16efbd44d3fbd1caab22c3b1fd57`

La imagen de fábrica completa no vive aquí. Su copia durable está en:

`/mnt/hdd/Data/60_Ops/firmware-backups/Waveshare-ESP32-S3-Touch-LCD-7-Rev1.2/factory-2026-07-16/`
