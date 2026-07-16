# Smart Light Panel — sistema visual

## Dirección

Control doméstico compacto para una pantalla táctil fija de 800×480. Tres vistas livianas: `LUZ`, `COLOR` y `TIMER`. Tipografía contenida, rectángulos planos, contraste alto y respuesta al apoyar el dedo.

## Estructura

```text
┌ Luz del living                               CONECTADO ┐
│ [ LUZ ] [ COLOR ] [ TIMER ]                            │
├────────────────────────────────────────────────────────┤
│ LUZ: estado + encendido + brillo + presets             │
│ COLOR: blanco + seis tonos, con nombres separados      │
│ TIMER: estado + apagar en + encender en + cancelar     │
└────────────────────────────────────────────────────────┘
```

## Tokens

- Canvas `#F4F3EF`; superficie `#FFFFFF`; texto `#20201D`.
- Secundario `#686761`; divisor `#D7D5CE`; conectado `#346538`; error `#9F2F2D`.
- Luz encendida `#D8902F` con texto oscuro.
- Tipografía Montserrat 12/14/16/20; no usar tamaños mayores.
- Bordes cuadrados, 1 px; sombras y animaciones prohibidas.

## Interacción y rendimiento

- Objetivos táctiles de 44 px como mínimo.
- Navegación y botones primarios actúan en `LV_EVENT_PRESSED`.
- El brillo se actualiza localmente y envía una orden al soltar.
- Sólo se ocultan/muestran la vista anterior y la nueva.
- La selección de color actualiza una etiqueta, no los siete botones.
- El estado remoto se consulta cada 15 segundos.
- No comunicar estado únicamente mediante color: siempre incluir texto.
