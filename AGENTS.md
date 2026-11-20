# AGENTS.md

Guía compacta para trabajar en este repositorio: adquisición eléctrica ESP32 + INA226 sobre PlatformIO (Arduino Framework).

## Comandos

- `pio run` — compila y verifica todo el proyecto. No hay tests ni CI: es la única verificación automatizada.
- `pio run -t upload` — carga en el ESP32 por USB.
- `pio device monitor` — monitor serie a 115200 baud.
- La primera compilación descarga la plataforma `espressif32` y el toolchain completo (~5 min). Las siguientes son rápidas.
- PlatformIO Core está en `~/.platformio/penv` (comando `pio` disponible en el PATH).

## C++17

`platformio.ini` activa C++17 con el par `build_unflags = -std=gnu++11` + `build_flags = -std=gnu++17`. Ambos son necesarios: si se borra el `build_unflags`, el estándar vuelve silenciosamente a gnu++11.

## Trampas de la librería INA226_WE (fijada a ^1.3.0)

- La 1.3.0 renombró los enums con prefijo `INA226_` (p. ej. `INA226_CONTINUOUS`) y deprecó `setCurrentRange()`: usar `setResistorRange(resistor, range)` para calibrar.
- `getBusPower()` devuelve **mW**, no W (el código lo convierte a W).
- `init()` devuelve `false` si el dispositivo no responde ACK: es la detección de ausencia.
- `getI2cErrorCode()` solo refleja el último acceso a registro: sirve para invalidar lecturas, no para diagnóstico fino.
- Antes de añadir llamadas nuevas, verificar firmas contra la fuente instalada (`~/.platformio/packages` o `.pio/libdeps/esp32dev/INA226_WE`) o el repo upstream `wollewald/INA226_WE`.

## Calibración: NO asumir valores

- Los valores de `src/config/HardwareConfig.h` (shunt 0.1 Ω, corriente máx. 0.8 A) son **provisionales y sin verificar contra hardware**. No darlos por buenos ni modificarlos sin medir; el README documenta el procedimiento de verificación.
- Todo pin, dirección y parámetro de hardware va centralizado en `HardwareConfig.h`.

## Convenciones del código

- Comentarios y documentación en español; identificadores en inglés.
- Sin `delay()`: programación periódica con `millis()` (`MeasurementService::update()`).
- Una lectura inválida jamás se representa como cero: se marca `valid=false` y se muestra `n/a`; el estado (`State`) indica la causa.
- Overflow (flag del sensor) no es error de comunicación: no dispara la recuperación automática.
- `src/main.cpp` es solo composición y salida Serial; la lógica vive en services/drivers. Jerarquía: `main` → `MeasurementService` → `INA226Sensor` (wrapper de INA226_WE) → `ElectricalMeasurement` (datos).

## Estado del repositorio

- Repo git inicializado pero sin commits todavía: no hay historial del que inferir intenciones.
- El firmware compila, pero **no ha sido probado sobre hardware real**: no afirmar funcionamiento físico.
