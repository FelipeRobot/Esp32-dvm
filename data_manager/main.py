"""
Punto de entrada de data_manager: captura y visualizacion en tiempo real de
las mediciones del ESP32 (INA219).

Lee el puerto Serie del ESP32 (o un archivo de prueba), registra las muestras
en un CSV por sesion y abre la interfaz grafica con la tension vs tiempo y
las estadisticas de la ventana movil.

Uso:
    python3 data_manager/main.py                       # puerto autodetectado
    python3 data_manager/main.py --port /dev/ttyUSB0   # puerto explicito
    python3 data_manager/main.py --source captura.txt  # prueba sin hardware
"""

from __future__ import annotations

import argparse
import os
import queue
import sys
import time
from pathlib import Path

# Permite ejecutar tambien como "python3 -m data_manager.main" desde la raiz
# del repositorio: el directorio de este script debe estar en sys.path.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from tracker.csv_writer import CsvWriter
from tracker.recorder import CaptureSession
from tracker.serial_reader import (END_OF_SOURCE, FileSource, ReaderThread,
                                   SerialSource, detect_port, list_ports)

try:
    import tkinter as tk
    from visualization.gui import Dashboard
except ImportError as error:
    tk = None
    Dashboard = None
    GUI_IMPORT_ERROR = error

POLL_INTERVAL_MS = 50    # sondeo de la cola de lineas
STALE_SECONDS = 3.0      # sin muestras durante este tiempo => aviso
OVERFLOW_TEXT = "OVERFLOW del sensor (shunt saturado / rango insuficiente)"
INVALID_TEXT = "muestra invalida (revisar sensor/cableado)"


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Captura CSV y visualizacion en tiempo real del ESP32+INA219.")
    parser.add_argument("--port", default=None,
                        help="puerto serie (por defecto se autodetecta)")
    parser.add_argument("--baud", type=int, default=115200,
                        help="baudios del puerto serie (default 115200)")
    parser.add_argument("--source", metavar="ARCHIVO", default=None,
                        help="reproducir lineas desde un archivo en lugar del puerto serie")
    parser.add_argument("--window-s", type=float, default=60.0,
                        help="ventana movil en segundos (default 60)")
    parser.add_argument("--captures-dir", default=None,
                        help="directorio de CSV (default: data_manager/captures)")
    parser.add_argument("--no-csv", action="store_true",
                        help="no escribir captura CSV")
    parser.add_argument("--replay-delay", type=float, default=0.05,
                        help="retardo entre lineas al reproducir archivo (default 0.05 s)")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_arg_parser().parse_args(argv)

    if Dashboard is None:
        print(f"No se pudo cargar la interfaz grafica: {GUI_IMPORT_ERROR}",
              file=sys.stderr)
        print("Instala el modulo tkinter del sistema con: "
              "sudo apt install python3-tk", file=sys.stderr)
        return 3

    captures_dir = (Path(args.captures_dir) if args.captures_dir
                    else Path(__file__).parent / "captures")

    if args.source:
        source = FileSource(args.source, args.replay_delay)
    else:
        port = args.port or detect_port()
        if port is None:
            print("No se detecto ningun puerto USB serie.", file=sys.stderr)
            visible = list_ports()
            if visible:
                print("Puertos visibles: " + ", ".join(visible), file=sys.stderr)
            print("Conecta el ESP32 o indica el puerto con --port.", file=sys.stderr)
            return 2
        source = SerialSource(port, args.baud)

    # ------------------------------ captura -------------------------------
    writer = CsvWriter(captures_dir, enabled=not args.no_csv)
    writer.start()
    session = CaptureSession(writer, args.window_s)

    line_queue: queue.Queue = queue.Queue()
    reader = ReaderThread(source, line_queue)
    reader.start()

    # ----------------------------- interfaz --------------------------------
    state = {"ended": False, "sensor": None}

    def toggle_pause() -> None:
        session.paused = not session.paused
        dashboard.set_paused(session.paused)

    def clear_view() -> None:
        session.clear_view()
        dashboard.clear()

    try:
        dashboard = Dashboard(window_seconds=args.window_s,
                              on_pause_toggle=toggle_pause,
                              on_clear=clear_view)
    except tk.TclError as error:
        print(f"No se pudo abrir la ventana grafica: {error}", file=sys.stderr)
        print("¿Hay un servidor grafico disponible? (variable DISPLAY)", file=sys.stderr)
        return 3
    dashboard.set_csv_path(f"CSV: {writer.path}" if writer.enabled
                           else "CSV: deshabilitado")

    def compose_status() -> str:
        base = reader.status
        if state["ended"]:
            return f"{base} - fin de fuente"
        if session.paused:
            return f"{base} - captura en pausa"
        if state["sensor"]:
            return f"{base} - {state['sensor']}"
        stale = session.stale_seconds()
        if stale is not None and stale > STALE_SECONDS:
            return f"{base} - sin datos recientes"
        return base

    def poll() -> None:
        while True:
            try:
                item = line_queue.get_nowait()
            except queue.Empty:
                break

            if item is END_OF_SOURCE:
                state["ended"] = True
                break

            if session.paused:
                continue  # las lineas llegadas en pausa se descartan

            sample = session.process_raw_line(item)
            if sample is not None:
                if sample.overflow:
                    state["sensor"] = OVERFLOW_TEXT
                elif not sample.valid:
                    state["sensor"] = INVALID_TEXT
                else:
                    state["sensor"] = None
                dashboard.add_sample(session.elapsed(), sample,
                                     session.stats_dict())

        dashboard.set_status(compose_status())
        if not state["ended"]:
            dashboard.after(POLL_INTERVAL_MS, poll)

    def on_close() -> None:
        reader.stop()
        writer.close()
        dashboard.destroy()

    print(f"Fuente: {source.describe() if args.source else args.port or 'auto'}")
    print(f"CSV: {writer.path}" if writer.enabled else "CSV: deshabilitado")
    print("Cierra la ventana para terminar.")

    dashboard.protocol("WM_DELETE_WINDOW", on_close)
    dashboard.after(POLL_INTERVAL_MS, poll)
    dashboard.mainloop()

    reader.stop()
    writer.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
