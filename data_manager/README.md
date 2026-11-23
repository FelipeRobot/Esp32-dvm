# data_manager — captura CSV y visualizacion en tiempo real

Aplicacion Python que consume la salida Serie del firmware `esp32-dvm`
(ESP32 + INA219), grafica la tension de la carga en tiempo real y guarda
todas las muestras en un CSV por sesion.

```
ESP32 (INA219) --USB/Serie--> data_manager
                                |-- tracker/        adquisicion, stats y CSV
                                |-- visualization/  GUI Tkinter + matplotlib
                                `-- captures/       CSV generados (gitignored)
```

## Requisitos

- Python >= 3.10 y `python3-tk` (paquete del sistema: `sudo apt install python3-tk`)
- Dependencias de pip en `requirements.txt`

```bash
cd data_manager
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Uso

```bash
# Desde la raiz del repositorio (puerto autodetectado, CSV activado)
.venv/bin/python data_manager/main.py

# Puerto explicito
.venv/bin/python data_manager/main.py --port /dev/ttyUSB0

# Solo visualizar, sin escribir CSV
.venv/bin/python data_manager/main.py --no-csv

# Probar sin hardware, reproduciendo una captura previa
.venv/bin/python data_manager/main.py --source captura.txt
```

La ventana muestra:

- **Grafica** de tension vs tiempo (ventana movil de `--window-s` segundos).
  Las muestras invalidas aparecen como huecos en la linea, nunca como ceros.
- **Panel de estadisticas** de la ventana movil: V actual, promedio, minimo,
  maximo (pico), RMS; I y P actuales; numero de muestras validas.
- **Barra de estado**: fuente de datos, estado del sensor (OK / OVERFLOW /
  muestra invalida / sin datos recientes) y ruta del CSV.
- Botones **Pausar captura** (descarta las muestras mientras esta activo) y
  **Limpiar vista** (reinicia grafica y estadisticas; el CSV no se toca).

Opciones completas: `python3 data_manager/main.py --help`.

## Formato del CSV

Un archivo por sesion en `captures/data_AAAA-MM-DD_HH-MM-SS.csv`, con cabecera
y una fila por muestra (flush en cada fila):

| Columna | Descripcion |
|---|---|
| `elapsed_s` | segundos desde la primera muestra de la sesion |
| `timestamp_ms` | `millis()` del ESP32 en la adquisicion |
| `voltage_V`, `current_A`, `power_W`, `shunt_voltage_mV` | mediciones (vacio = NaN) |
| `valid` | 1 lectura fiable, 0 lectura invalida |
| `overflow` | 1 cuando el INA219 indica overflow |

Los valores invalidos van como campos vacios (NaN al leer con pandas), nunca
como cero.

## Estructura

```
data_manager/
├── main.py                  # punto de entrada: args, hilos y GUI
├── requirements.txt
├── tracker/                 # logica de captura (probable sin pantalla)
│   ├── serial_reader.py     # hilo lector: Serial o archivo, con reconexion
│   ├── parser.py            # parsea lineas "DATA,..."
│   ├── stats.py             # ventana movil: promedio, min, max, RMS
│   ├── recorder.py          # sesion de captura: orquesta stats + CSV
│   └── csv_writer.py        # CSV por sesion con flush por muestra
└── visualization/
    └── gui.py               # Tkinter + matplotlib: grafica y panel
```

## Solucion de problemas

- **No detecta el puerto**: conecta el ESP32 y usa `--port /dev/ttyUSB0`
  (o el que liste el mensaje de error). Verifica permisos con
  `sudo usermod -aG dialout $USER` (requiere cerrar sesion).
- **`ModuleNotFoundError: No module named 'tkinter'`**: falta el paquete del
  sistema `python3-tk`.
- **La grafica no avanza**: el firmware debe estar cargado con la salida
  `DATA,...` (compilalo con `pio run -t upload`). Con el monitor serie de
  PlatformIO abierto al mismo tiempo el puerto queda ocupado: cierra uno de
  los dos.
- **Lineas invalidas en el CSV**: son muestras `valid=0` (sensor ausente o
  overflow); el firmware nunca las emite como ceros.
