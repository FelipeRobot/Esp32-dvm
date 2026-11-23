# Monitor de carga DC (ESP32 + INA219) — Generador DC

Firmware para el ESP32 DevKit V1 que caracteriza experimentalmente un motor DC
utilizado como generador, midiendo la rama de carga con un sensor INA219 sobre
bus I2C:

- **Tensión de la carga** (V)
- **Corriente de la carga** (A)
- **Potencia eléctrica de salida** (W, calculada como P = V × I)
- **Tensión del shunt** (mV)

Firmware modular en C++17 (Arduino Framework, PlatformIO), pensado como base
limpia y extensible para futuras ampliaciones (RPM, tensión del generador,
almacenamiento…). Cada muestra se emite además como una línea CSV por Serial
(ver "Salida CSV para data_manager") para la captura y visualización en
tiempo real con Python (`data_manager/`).

> **Nota de alcance:** este firmware mide únicamente la rama de carga.
> La medición de la tensión del generador/condensador (divisor + ADC) y la
> medición de RPM (sensor Hall/óptico) están previstas como tareas
> posteriores.

---

## Arquitectura del experimento

```text
Generador DC ──► Condensador ──► Regulador DC-DC ──► INA219 (serie) ──► Carga (R + LED)
                                                    VIN+ ─── VIN−
```

El INA219 se intercala **en serie** con la carga, entre la salida del
regulador DC-DC y la carga:

```text
DC-DC (+) ─────────────────────── VIN+ (INA219)
                                  VIN− (INA219) ────── Carga (+)

DC-DC (−) ──────┬──────────────────────────────────── Carga (−)
                └───────────────── GND módulo INA219 y GND del ESP32

ESP32 3V3 ─── VCC (INA219)        ESP32 GPIO21 ─── SDA (INA219)
ESP32 GND ─── GND (INA219)        ESP32 GPIO22 ─── SCL (INA219)
```

## Componentes necesarios

| Componente | Notas |
|---|---|
| ESP32 DevKit V1 | Compatible con PlatformIO `esp32dev` |
| Módulo INA219 | Con shunt integrado (típicamente 0.1 Ω, **a verificar**) |
| Motor DC usado como generador | Objeto del experimento |
| Condensador + regulador DC-DC | Acondicionamiento previo a la carga |
| Carga (resistencia + LED) | Rama medida por el INA219 |
| Multímetro | Obligatorio para verificar el shunt y contrastar lecturas |

### Advertencias de seguridad eléctrica

- **Masa común obligatoria**: ESP32, módulo INA219 y el negativo del regulador
  deben compartir GND; si no, las mediciones y la propia comunicación I2C no
  tienen referencia.
- **Niveles I2C**: los pines del ESP32 trabajan a 3.3 V y **no toleran 5 V**.
  Alimenta el módulo INA219 a 3.3 V (no a 5 V), para que sus pull-ups del bus
  I2C no queden a 5 V.
- **No exceder 26 V** en el bus del INA219 (límite del chip).
- No exceder la corriente máxima del shunt (ver sección de calibración).
  Un shunt sobrecargado se daña y puede dañar lo conectado.
- Si intercambias VIN+ y VIN−, las lecturas de corriente serán negativas
  (el sensor es bidireccional).
- **Síntoma observado en pruebas**: si la carga no funciona con el INA219
  intercalado, revisa primero la continuidad de la rama serie (VIN+/VIN−,
  soldaduras del módulo) y su alimentación VCC/GND; el escaneo I2C del
  arranque confirma si el chip responde.

## Dependencias

Gestionadas automáticamente por PlatformIO vía `lib_deps`:

| Librería | Versión | Uso |
|---|---|---|
| [INA219_WE](https://github.com/wollewald/INA219_WE) | `^1.4.1` | Comunicación y calibración del INA219 |

Instalación manual equivalente: `pio lib install "wollewald/INA219_WE@^1.4.1"`

## Configuración de PlatformIO

| Parámetro | Valor |
|---|---|
| Plataforma | `espressif32` |
| Board | `esp32dev` (ESP32 DevKit V1) |
| Framework | `arduino` |
| Estándar C++ | `gnu++17` |
| Baud rate Serial | 115200 |
| I2C | SDA=GPIO21, SCL=GPIO22, 100 kHz |
| Dirección INA219 | `0x40` (A0=A1 abiertos; verificar con el escaneo I2C) |

## Parámetros de calibración (⚠ PROVISIONALES)

Los valores iniciales están en `src/config/HardwareConfig.h` y son
**provisionales**: corresponden al módulo INA219 comercial típico, pero deben
verificarse físicamente antes de fiarse de las lecturas.

| Parámetro | Valor inicial | Cómo verificarlo |
|---|---|---|
| `SHUNT_RESISTANCE_OHM` | `0.1` | **Medir**: hacer circular una corriente conocida y estable (p. ej. 100 mA medida con multímetro) por VIN+/VIN− y medir la caída entre ambos; `R = V/I`. Muchos módulos marcan el shunt SMD como "R100" (0.1 Ω), pero no lo des por hecho. |
| `INA219_I2C_ADDRESS` | `0x40` | A0 y A1 abiertos. Otras combinaciones: 0x41, 0x44, 0x45 (ver marcado del módulo). El escaneo I2C del arranque lista la dirección real. |
| Rango de shunt (PGA) | ±320 mV | Fijado en el driver (`INA219_PG_320`). Con 0.1 Ω → ±3.2 A. Para corrientes pequeñas con otro shunt puede interesar ±40 mV. |
| Rango de bus | 0–32 V | Configurado en el driver (`INA219_BRNG_32`). El chip admite hasta 26 V reales. |
| Niveles I2C | 3.3 V | Ver sección de seguridad. |

Opcional: si al contrastar con un multímetro calibrado hay una desviación
constante, aplicar `setCorrectionFactor()` en `INA219Sensor.cpp`.

## Compilación y carga

```bash
# Compilar (descarga toolchain y librerías la primera vez)
pio run

# Cargar en el ESP32 conectado por USB
pio run -t upload

# Monitor serie (115200 baud)
pio device monitor
```

## Salida de ejemplo

```text
=== Monitor de carga DC (INA219) - Generador ===
I2C: SDA=GPIO21 SCL=GPIO22 @ 100000 Hz, INA219 en 0x40
Shunt (PROVISIONAL): 0.100 ohm
Sample interval: 500 ms
Verifica la calibracion contra tu modulo antes de fiarte de las lecturas!
------------------------
Escaneo I2C (0x08-0x77)...
  Dispositivo encontrado en 0x40
------------------------
INA219 STATUS: OK
Voltage:           5.100 V
Current:           0.150 A
Power:             0.765 W
Shunt Voltage:    15.000 mV
DATA,12345,5.100,0.150,0.765,15.000,1,0
------------------------
```

Con el sensor desconectado o sin responder:

```text
Escaneo I2C (0x08-0x77)...
  Ningun dispositivo encontrado: revisar SDA/SCL y alimentacion
------------------------
INA219 STATUS: NOT DETECTED (revisar cableado/direccion)
Voltage:               n/a V
Current:               n/a A
Power:                 n/a W
Shunt Voltage:         n/a mV
------------------------
```

Una lectura inválida **nunca** se muestra como `0.000`: se marca `n/a` y el
estado indica la causa (no detectado, error de comunicación, overflow).

## Salida CSV para data_manager

Junto al reporte legible, cada muestra se emite como una línea CSV con el
prefijo `DATA,` para consumo automático (la aplicación `data_manager/` de
este repositorio la grafica en tiempo real y la guarda en CSV):

```text
DATA,12345,5.100,0.150,0.765,15.000,1,0
```

| Campo | Descripción |
|---|---|
| 1 | Prefijo fijo `DATA` (el consumidor ignora el resto de líneas) |
| 2 | `timestamp_ms`: `millis()` del ESP32 en la adquisición |
| 3–6 | `voltage_V`, `current_A`, `power_W`, `shunt_voltage_mV` |
| 7 | `valid` (0/1) |
| 8 | `overflow` (0/1) |

Una muestra inválida jamás se emite como ceros: los campos numéricos van
**vacíos** (`DATA,12345,,,,,0,0`) y el consumidor debe tratarlos como NaN.
Las líneas se emiten con el mismo disparador que el reporte legible (nueva
muestra o cambio de estado).

## Estructura del código

```text
src/
├── main.cpp                          # init, escaneo I2C, coordinación, salida Serial
├── config/
│   └── HardwareConfig.h              # pines, bus, calibración (un solo lugar)
├── drivers/
│   ├── INA219Sensor.h/.cpp           # wrapper del INA219_WE + gestión de errores
├── models/
│   └── ElectricalMeasurement.h       # estructura de datos de la medición
└── services/
    ├── MeasurementService.h/.cpp     # adquisición periódica y recuperación
```

- `main.cpp` es solo la raíz de composición, el escaneo I2C de arranque y la
  presentación.
- `INA219Sensor` encapsula la biblioteca INA219_WE por composición.
- `MeasurementService` programa la adquisición con `millis()` (sin `delay()`)
  y expone `latestMeasurement()` / `state()` para futuros consumidores.

## Manejo de errores

| Situación | Comportamiento |
|---|---|
| Sensor ausente al arrancar | Estado `NOT DETECTED`; reintento cada 2 s |
| Pérdida de comunicación en marcha | Tras 5 fallos consecutivos → estado `COMM ERROR`; re-inicialización del sensor cada 2 s |
| Overflow (shunt saturado o cálculo fuera de rango) | Estado `OVERFLOW` con aviso; la medición se marca inválida (no se muestra como cero) |
| Error I2C en una lectura | La muestra se marca `valid = false` y se imprime `n/a` |

La detección de dispositivo ausente se apoya en el ACK de `init()` de la
biblioteca y en una verificación de ACK propia tras cada lectura (INA219_WE
no expone código de error I2C; ver `AGENTS.md`).

## Limitaciones conocidas

- **Sin prueba en hardware realizada**: el código compila, pero no se ha
  verificado físicamente contra un módulo real en el momento de escribirlo.
- Los valores de calibración son provisionales hasta medir el shunt real.
- Con corriente negativa (VIN+/VIN− intercambiados), la potencia calculada
  como V × I también será negativa: revisar el cableado de la rama serie.
- Por alcance de esta versión: sin ADC/divisor para la tensión del
  generador, sin RPM, sin Wi-Fi/MQTT ni almacenamiento persistente. La
  captura CSV y la visualización en tiempo real viven en `data_manager/`
  (Python), que consume la salida `DATA,...` por Serial.

## Procedimiento de verificación del hardware

1. Verificar el shunt del módulo con multímetro (ver tabla de calibración) y
   actualizar `SHUNT_RESISTANCE_OHM`.
2. Cablear según el diagrama, con masa común y el módulo alimentado a 3.3 V.
3. `pio run -t upload` y `pio device monitor`.
4. **Verificación I2C**: el escaneo del arranque debe listar un dispositivo
   en `0x40`. Si no aparece nada, revisar SDA/SCL (GPIO21/GPIO22), VCC y GND
   del módulo; el firmware seguirá funcionando y reintentando cada 2 s.
5. Comprobar el informe: debe aparecer `INA219 STATUS: OK` y lecturas cada
   500 ms; sin carga, corriente ≈ 0 y tensión de shunt ≈ 0.
6. Contrastar tensión y corriente de la carga con el multímetro.
7. Prueba de robustez: desconectar SDA/SCL en caliente → debe pasar a
   `COMM ERROR`; reconectar → debe recuperarse solo.
