# ESP32 Electrical Monitor (INA226)

Sistema de adquisición de datos eléctricos DC basado en ESP32 y sensor INA226
sobre bus I2C. Mide:

- **Tensión de bus** (V)
- **Corriente** (A)
- **Potencia** (W)
- **Tensión del shunt** (mV)

Firmware modular en C++17 (Arduino Framework, PlatformIO), pensado como base
limpia y extensible para futuras ampliaciones (Wi-Fi, MQTT, almacenamiento…).

---

## Componentes necesarios

| Componente | Notas |
|---|---|
| ESP32 DevKit (clásico) | Cualquier variante compatible con PlatformIO `esp32dev` |
| Módulo INA226 | Con shunt integrado (típicamente 0.1 Ω, **a verificar**) |
| Fuente DC | 0–36 V (rango de bus del INA226) |
| Carga para pruebas | Respetar la corriente máxima del shunt |
| Multímetro | Obligatorio para verificar el shunt y contrastar lecturas |

## Diagrama de conexiones

Configuración **high-side** (la recomendada para este sensor):

```text
Fuente DC (+) ───────┬─────────────────────────────── IN+ (INA226)
                     └─────────────────────────────── VBUS (INA226)
                         IN- (INA226) ─────────────── Carga (+)

Fuente DC (-) ───────┬─────────────────────────────── Carga (-)
                     └─────────────────────────────── GND del módulo INA226
                                                      y GND del ESP32

ESP32 3V3 ─── VCC (INA226)        ESP32 GPIO21 ─── SDA (INA226)
ESP32 GND ─── GND (INA226)        ESP32 GPIO22 ─── SCL (INA226)
```

| Pin módulo INA226 | Conexión |
|---|---|
| VCC | ESP32 3V3 (recomendado; el módulo admite 3–5.5 V) |
| GND | Masa común: ESP32 GND **y** negativo de la fuente |
| SDA | ESP32 GPIO 21 |
| SCL | ESP32 GPIO 22 |
| IN+ | Positivo de la fuente DC |
| IN− | Positivo de la carga |
| VBUS | Positivo de la fuente DC (o IN+) |
| Carga (−) | Negativo de la fuente DC (masa común) |

### Advertencias de seguridad eléctrica

- **Masa común obligatoria**: ESP32, módulo INA226 y fuente DC deben compartir
  GND; si no, las mediciones y la propia comunicación I2C no tienen referencia.
- **Niveles I2C**: los pines del ESP32 trabajan a 3.3 V y **no toleran 5 V**.
  Si el módulo se alimenta a 5 V, sus pull-ups del bus I2C quedarían a 5 V.
  Aliméntalo a 3.3 V o verifica el diseño de pull-ups / usa adaptador de nivel.
- **No conectes ninguna salida del módulo a un pin del ESP32 sin verificar
  antes sus niveles de tensión** (aplica al pin ALERT si algún día se usa).
- No exceder **36 V** en el bus ni la corriente máxima del shunt (ver sección
  de calibración). Un shunt sobrecargado se daña y puede dañar lo conectado.
- Si intercambias IN+ e IN−, las lecturas de corriente serán negativas
  (el sensor es bidireccional).

## Dependencias

Todas las dependencias tienen versión fijada para reproducibilidad (imagen
base, PlatformIO Core, plataforma y librerías). La compilación se hace en el
contenedor Docker (ver "Entorno de desarrollo"); PlatformIO gestiona la
librería vía `lib_deps`:

| Librería | Versión | Uso |
|---|---|---|
| [INA226_WE](https://github.com/wollewald/INA226_WE) | `~1.3.0` (fijada) | Comunicación y calibración del INA226 |

La versión 1.3.0 es importante: renombra los enums con prefijo `INA226_` y
ofrece `setResistorRange()` (la antigua `setCurrentRange()` está deprecada).

Instalación manual equivalente: `pio lib install "wollewald/INA226_WE@~1.3.0"`

## Configuración de PlatformIO

| Parámetro | Valor |
|---|---|
| PlatformIO Core | `6.2.0` (fijado en la imagen Docker) |
| Plataforma | `espressif32@8.0.0` (fijada) |
| Board | `esp32dev` (ESP32 DevKit clásico) |
| Framework | `arduino` |
| Estándar C++ | `gnu++17` |
| Baud rate Serial | 115200 |

## Parámetros de calibración (⚠ PROVISIONALES)

Los valores iniciales están en `src/config/HardwareConfig.h` y son
**provisionales**: corresponden al módulo INA226 comercial típico, pero deben
verificarse físicamente antes de fiarse de las lecturas.

| Parámetro | Valor inicial | Cómo verificarlo |
|---|---|---|
| `SHUNT_RESISTANCE_OHM` | `0.1` | **Medir**: hacer circular una corriente conocida y estable (p. ej. 100 mA medida con multímetro) por IN+/IN− y medir la caída entre ambos; `R = V/I`. Muchos módulos marcan el shunt SMD como "R100" (0.1 Ω), pero no lo des por hecho. |
| `MAX_EXPECTED_CURRENT_A` | `0.8` | Debe cumplir `I_max ≤ 81.92 mV / R_shunt` (límite del INA226). Con 0.1 Ω → máx. ±819 mA. Además, comprobar la potencia disipada en el shunt: `P = I²·R` (0.8 A · 0.1 Ω → 64 mW); consultar la hoja de datos del resistor del módulo. |
| `INA226_I2C_ADDRESS` | `0x40` | A0 y A1 abiertos. Otras combinaciones: 0x41, 0x44, 0x45 (ver marcado del módulo). |
| Rango de bus | 0–36 V | Límite del módulo/INA226. El ESP32 no queda expuesto a esta tensión: solo mide. |
| Niveles I2C | 3.3 V | Ver sección de seguridad. |

Opcional: si al contrastar con un multímetro calibrado hay una desviación
constante, aplicar `setCorrectionFactor()` en `INA226Sensor.cpp`.

## Entorno de desarrollo (Docker)

Toda la compilación se hace en un contenedor reproducible: en una máquina
limpia basta con instalar **Docker** (y `git` para clonar). No hace falta
instalar PlatformIO ni toolchains en el host.

### Requisitos del host

| Herramienta | Mínimo | Uso |
|---|---|---|
| Docker Engine + Compose v2 | 24+ (Linux) | Compilar el firmware |
| Docker Desktop | Última estable (macOS / Windows) | Compilar el firmware |
| git | Reciente | Clonar el repositorio |
| PlatformIO CLI (opcional) | 6.2.0 | Solo para cargar y monitorizar desde el host |

### Inicio del entorno

```bash
git clone <url-del-repositorio>
cd esp32-dvm

# 1. Construir la imagen (la primera vez descarga ~2 GB de toolchain; tarda varios minutos)
make image            # o: docker compose build

# 2. Compilar el firmware
make build            # o: docker compose run --rm build pio run
```

La imagen fija todas las versiones (imagen base por digest, PlatformIO Core
6.2.0, plataforma `espressif32@8.0.0`, librerías en `platformio.ini`): dos
máquinas distintas producen binarios equivalentes.

### Comandos unificados

| Comando | Acción |
|---|---|
| `make image` | Construye la imagen Docker |
| `make build` | Compila el firmware en el contenedor |
| `make clean` | Limpia los artefactos de compilación (`.pio/`) |
| `make test` | Ejecuta las pruebas (todavía no hay ninguna definida) |
| `make upload` | Carga el firmware desde el **host** |
| `make monitor` | Monitor serie (115200 baud) desde el **host** |
| `make shell` | Shell dentro del contenedor |

Sin `make` (Windows sin WSL), los equivalentes directos:

```bash
docker compose build                              # make image
docker compose run --rm build pio run             # make build
docker compose run --rm build pio run -t clean    # make clean
docker compose run --rm build bash                # make shell
```

### Carga del firmware y monitor serie (desde el host)

El acceso USB **no es obligatorio desde Docker**: la carga y el monitor serie
se ejecutan desde el sistema anfitrión, evitando permisos y drivers dentro
del contenedor.

Opción A — PlatformIO CLI en el host (recomendada):

```bash
pip install platformio==6.2.0
pio run -t upload      # o: make upload
pio device monitor     # o: make monitor
```

Opción B — solo esptool (más ligero, sin PlatformIO completo):

```bash
pip install esptool
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x10000 .pio/build/esp32dev/firmware.bin
```

La opción B solo escribe la aplicación: exige que el ESP32 ya tenga el
bootloader y las particiones grabados por una carga completa previa
(`pio run -t upload`).

### Puerto serie por sistema operativo

| Sistema | Puerto | Notas |
|---|---|---|
| Linux | `/dev/ttyUSB0` (o `ttyACM0`) | Añadir el usuario al grupo `dialout` (Debian/Ubuntu) o `uucp` (Arch): `sudo usermod -aG dialout $USER` y reabrir sesión |
| macOS | `/dev/cu.usbserial-*` | Instalar el driver del chip USB-serial: CP210x (la mayoría de DevKits) o CH340 (clones) |
| Windows | `COMx` (Administrador de dispositivos) | Instalar el driver CP210x/CH340; el número aparece en "Puertos (COM y LPT)" |

Para indicar el puerto: `pio run -t upload --upload-port /dev/ttyUSB0` y
`pio device monitor -p /dev/ttyUSB0`.

*Nota avanzada*: también es posible pasar el dispositivo USB al contenedor
(`docker compose run --rm --device /dev/ttyUSB0 build bash`), pero no es el
flujo recomendado por los permisos que exige en cada sistema.

### Solución de problemas

| Síntoma | Causa habitual | Solución |
|---|---|---|
| `docker: permission denied` | Usuario fuera del grupo `docker` | `sudo usermod -aG docker $USER` y reabrir sesión (o Docker rootless) |
| `make image` muy lento la primera vez | Descarga ~2 GB de toolchain | Normal: ocurre una vez por versión fijada |
| Falta espacio en disco en `make image` | Toolchains + cachés de capas | Liberar varios GB; en Docker Desktop revisar "Resources" |
| `No such port /dev/ttyUSB0` | Cable, conector o driver | `dmesg | grep tty` tras conectar; revisar driver CP210x/CH340 |
| `Permission denied /dev/ttyUSB0` | Falta el grupo de dialout | `sudo usermod -aG dialout $USER` (Linux) y reabrir sesión |
| Monitor: "puerto ocupado" | Otro monitor/IDE abierto | Cerrar el otro proceso |
| `make` no existe (Windows) | `make` no viene con Windows | Usar los comandos `docker compose` equivalentes (o WSL/Git Bash) |
| El CI falla en `docker compose build` | Alguna versión fijada retirada del registro | Actualizar pins según "Actualización de dependencias" |

### Actualización de dependencias

Las dependencias se actualizan a propósito, nunca implícitamente:

1. Comprobar versiones nuevas dentro del contenedor:
   `docker compose run --rm build pio pkg outdated`
2. Editar los pins: `platformio.ini` (plataforma/librerías) y
   `docker/Dockerfile` (digest de la imagen base, `PLATFORMIO_VERSION`).
3. Reconstruir y compilar: `make image && make build`.
4. Si cambia la imagen base, verificar el digest nuevo en Docker Hub.
5. Commit con los cambios de pins.

## Integración continua

`.github/workflows/ci.yml` ejecuta en cada push (a `master` o `feature/**`) y
pull request el **mismo procedimiento local**: construye la imagen y compila
con `docker compose run --rm build pio run`. El paso de pruebas queda
preparado para activarse cuando exista un directorio `test/`. El primer build
en CI tarda varios minutos (sin caché de imagen entre ejecuciones).

## Salida de ejemplo

```text
=== ESP32 Electrical Monitor ===
I2C: SDA=GPIO21 SCL=GPIO22 @ 100000 Hz, address 0x40
Shunt (PROVISIONAL): 0.100 ohm | Max current (PROVISIONAL): 0.800 A
Sample interval: 500 ms
Verifica la calibracion contra tu modulo antes de fiarte de las lecturas!
------------------------
INA226 STATUS: OK
Voltage:          12.000 V
Current:           1.500 A
Power:            18.000 W
Shunt Voltage:     7.500 mV
------------------------
```

Con el sensor desconectado o sin responder:

```text
INA226 STATUS: NOT DETECTED (revisar cableado/direccion)
Voltage:               n/a V
Current:               n/a A
Power:                 n/a W
Shunt Voltage:         n/a mV
------------------------
```

Una lectura inválida **nunca** se muestra como `0.000`: se marca `n/a` y el
estado indica la causa (no detectado, error de comunicación, overflow).

## Estructura del código

```text
├── platformio.ini                    # board, framework, versiones fijadas
├── Makefile                          # comandos unificados (Docker + host)
├── docker-compose.yml                # servicio de compilación
├── docker/
│   └── Dockerfile                    # imagen reproducible (versiones fijadas)
├── .devcontainer/
│   └── devcontainer.json             # VS Code Dev Containers
├── .github/workflows/
│   └── ci.yml                        # CI: mismo procedimiento que local
└── src/
    ├── main.cpp                      # init, coordinación, salida Serial
    ├── config/
    │   └── HardwareConfig.h          # pines, bus, calibración (un solo lugar)
    ├── drivers/
    │   ├── INA226Sensor.h/.cpp       # wrapper del INA226_WE + gestión de errores
    ├── models/
    │   └── ElectricalMeasurement.h   # estructura de datos de la medición
    └── services/
        ├── MeasurementService.h/.cpp # adquisición periódica y recuperación
```

- `main.cpp` es solo la raíz de composición y la presentación.
- `INA226Sensor` encapsula la biblioteca INA226_WE por composición.
- `MeasurementService` programa la adquisición con `millis()` (sin `delay()`)
  y expone `latestMeasurement()` / `state()` para futuros consumidores.

## Manejo de errores

| Situación | Comportamiento |
|---|---|
| Sensor ausente al arrancar | Estado `NOT DETECTED`; reintento cada 2 s |
| Pérdida de comunicación en marcha | Tras 5 fallos consecutivos → estado `COMM ERROR`; re-inicialización del sensor cada 2 s |
| Overflow de registro (rango insuficiente) | Estado `OVERFLOW` con aviso; la medición se marca inválida (no se muestra como cero) |
| Error I2C en una lectura | La muestra se marca `valid = false` y se imprime `n/a` |

La detección de dispositivo ausente se apoya en el ACK de `init()` de la
biblioteca y en su código de error I2C (`getI2cErrorCode()`).

## Limitaciones conocidas

- **Sin prueba en hardware realizada**: el código compila, pero no se ha
  verificado físicamente contra un módulo real en el momento de escribirlo.
- La imagen Docker tampoco se ha validado localmente en el momento de
  crearla (la máquina de desarrollo no tenía Docker instalado); el workflow
  de CI la valida en cada push.
- Los valores de calibración son provisionales hasta medir el shunt real.
- `getI2cErrorCode()` de la librería refleja solo el último acceso a registro;
  es suficiente para invalidar lecturas, pero el código numérico concreto
  puede variar según el core Arduino.
- El registro de potencia del INA226 es sin signo: con corriente negativa
  (sentido inverso) el valor de potencia no es representativo.
- Por alcance de esta primera versión: sin Wi-Fi, MQTT, HTTP, GUI, bases de
  datos ni almacenamiento persistente.

## Procedimiento de prueba del hardware

1. Verificar el shunt del módulo con multímetro (ver tabla de calibración) y
   actualizar `SHUNT_RESISTANCE_OHM` / `MAX_EXPECTED_CURRENT_A`.
2. Cablear según el diagrama, con masa común y el módulo alimentado a 3.3 V.
3. Cargar y monitorizar desde el host: `make upload` y `make monitor`
   (o `pio run -t upload` / `pio device monitor`).
4. Comprobar el arranque: debe aparecer `INA226 STATUS: OK` y lecturas cada
   500 ms; sin carga, corriente ≈ 0 y tensión de shunt ≈ 0.
5. Contrastar tensión de bus y corriente con el multímetro.
6. Prueba de robustez: desconectar SDA/SCL en caliente → debe pasar a
   `COMM ERROR`; reconectar → debe recuperarse solo.
