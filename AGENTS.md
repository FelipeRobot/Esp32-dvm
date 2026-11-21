# AGENTS.md

Guía compacta para trabajar en este repositorio: adquisición eléctrica ESP32 + INA219 sobre PlatformIO (Arduino Framework). El firmware mide la rama de carga de un motor DC usado como generador.

## Comandos

- `pio run` — compila y verifica todo el proyecto. No hay tests ni CI: es la única verificación automatizada.
- `pio run -t upload` — carga en el ESP32 por USB.
- `pio device monitor` — monitor serie a 115200 baud.
- La primera compilación descarga la plataforma `espressif32` y el toolchain completo (~5 min). Las siguientes son rápidas.
- PlatformIO Core está en `~/.platformio/penv` (comando `pio` disponible en el PATH).

## C++17

`platformio.ini` activa C++17 con el par `build_unflags = -std=gnu++11` + `build_flags = -std=gnu++17`. Ambos son necesarios: si se borra el `build_unflags`, el estándar vuelve silenciosamente a gnu++11.

## Trampas de la librería INA219_WE (fijada a ^1.4.1)

- `init()` escribe 0x8000 en CONFIG (reset del chip) y deja 12 bits, modo continuo, PGA ±320 mV y bus de 32 V. Devuelve `false` si el dispositivo no responde ACK: es la detección de ausencia.
- **No existe `getI2cErrorCode()`** (a diferencia de INA226_WE): `INA219Sensor::readMeasurement()` verifica el ACK con un `Wire.beginTransmission`/`endTransmission` propio tras cada lectura; código ≠ 0 invalida la muestra.
- **No existen `waitUntilConversionCompleted()` ni `readAndClearFlags()`**: en `begin()`/`reinitialize()` se sondea `getConversionReady()` con un tope de 10 ms para no mostrar ceros en el primer informe.
- `getOverflow()` solo pone el flag `overflow` a `true`, nunca lo limpia; `getShuntVoltage_mV()` sí lo resetea cuando el shunt no está saturado. Orden de lectura obligatorio en `readMeasurement()`: shunt → V → I → `getOverflow()` → `getConversionReady()` (este último lee el registro de potencia, lo que resetea el CNVR y con él el MOVF, según la semántica del INA219; sin esa limpieza el overflow podría quedar enclavado entre muestras).
- `setShuntSizeInOhms(shunt)` calibra por factor `shunt/0.1`; la PGA define el rango del shunt (el driver fija `INA219_PG_320` = ±320 mV → ±3.2 A con 0.1 Ω). No hay equivalente a `setResistorRange()` de INA226_WE.
- `getCurrent_mA()` devuelve mA (el driver convierte a A). `getBusPower()` devuelve mW, pero **no se usa**: la potencia se calcula como P = V × I con las lecturas mostradas.
- Antes de añadir llamadas nuevas, verificar firmas contra la fuente instalada (`~/.platformio/packages` o `.pio/libdeps/esp32dev/INA219_WE`) o el repo upstream `wollewald/INA219_WE` (tag `1.4.1`).

## Calibración: NO asumir valores

- Los valores de `src/config/HardwareConfig.h` (shunt 0.1 Ω) son **provisionales y sin verificar contra hardware**. No darlos por buenos ni modificarlos sin medir; el README documenta el procedimiento de verificación.
- Todo pin, dirección y parámetro de hardware va centralizado en `HardwareConfig.h`.

## Convenciones del código

- Comentarios y documentación en español; identificadores en inglés.
- Sin `delay()`: programación periódica con `millis()` (`MeasurementService::update()`).
- Una lectura inválida jamás se representa como cero: se marca `valid=false` y se muestra `n/a`; el estado (`State`) indica la causa.
- Overflow (flag del sensor) no es error de comunicación: no dispara la recuperación automática.
- `src/main.cpp` es solo composición, escaneo I2C de arranque y salida Serial; la lógica vive en services/drivers. Los drivers no imprimen por Serial. Jerarquía: `main` → `MeasurementService` → `INA219Sensor` (wrapper de INA219_WE) → `ElectricalMeasurement` (datos).
- El escaneo I2C del arranque (en `main.cpp`) es la forma canónica de verificar la presencia y dirección real del INA219; no asumir que 0x40 responde.

## Estado del repositorio

- El firmware compila, pero **no ha sido probado sobre hardware real**: no afirmar funcionamiento físico.
- Pendiente de tareas futuras: medición de la tensión del generador/condensador (divisor + ADC), medición de RPM (sensor Hall/óptico con interrupciones) y exportación CSV por Serial. No implementarlas mezclando configuraciones de otras placas ESP32.
