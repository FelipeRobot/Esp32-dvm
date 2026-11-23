"""
Lector de lineas desde el puerto Serie del ESP32 (o desde un archivo de
prueba). Corre en un hilo propio y deposita las lineas crudas en una cola
thread-safe para que el hilo principal (GUI) las consuma.
"""

from __future__ import annotations

import queue
import threading
import time
from pathlib import Path

import serial
import serial.tools.list_ports

# Subcadenas que identifican puertos USB serie tipicos (CP210x, CH340, ACM...)
USB_PORT_HINTS = ("ttyUSB", "ttyACM", "cu.usb", "cu.SLAB", "COM")

RETRY_INTERVAL_S = 2.0  # espera entre reintentos de conexion

# Sentinel encolado al terminar la fuente (p. ej. fin de archivo de prueba)
END_OF_SOURCE = None


class LineSource:
    """Fuente de lineas de texto. Abstraccion comun de Serial y archivo."""

    def open(self) -> None:
        raise NotImplementedError

    def readline(self) -> str | None:
        """Siguiente linea, o None si no hay datos por ahora."""
        raise NotImplementedError

    def eof(self) -> bool:
        """True cuando la fuente termino (fin de archivo)."""
        return False

    def close(self) -> None:
        raise NotImplementedError

    def describe(self) -> str:
        raise NotImplementedError


def detect_port() -> str | None:
    """Devuelve el primer puerto USB serie detectado, o None."""
    for port in serial.tools.list_ports.comports():
        if any(hint in port.device for hint in USB_PORT_HINTS):
            return port.device
    return None


def list_ports() -> list[str]:
    """Lista todos los puertos serie visibles (para mensajes de ayuda)."""
    return [port.device for port in serial.tools.list_ports.comports()]


class SerialSource(LineSource):
    """Lineas leidas del puerto serie del ESP32."""

    def __init__(self, port: str, baud: int):
        self._port = port
        self._baud = baud
        self._serial: serial.Serial | None = None

    def open(self) -> None:
        self._serial = serial.Serial(self._port, self._baud, timeout=0.5)

    def readline(self) -> str | None:
        assert self._serial is not None
        data = self._serial.readline()  # bloquea hasta nueva linea o timeout
        if not data:
            return None
        return data.decode("utf-8", errors="replace")

    def close(self) -> None:
        if self._serial is not None:
            self._serial.close()
            self._serial = None

    def describe(self) -> str:
        return f"{self._port} @ {self._baud} baud"


class FileSource(LineSource):
    """Lineas leidas de un archivo, para pruebas sin hardware.

    El archivo se consume con un pequeno retardo entre lineas para que la
    GUI reproduzca la captura a un ritmo similar al real.
    """

    def __init__(self, path: str, replay_delay_s: float = 0.05):
        self._path = path
        self._delay = replay_delay_s
        self._file = None
        self._eof = False

    def open(self) -> None:
        self._file = open(self._path, "r", encoding="utf-8", errors="replace")
        self._eof = False

    def readline(self) -> str | None:
        assert self._file is not None
        line = self._file.readline()
        if line == "":
            self._eof = True
            return None  # fin de archivo
        if self._delay > 0:
            time.sleep(self._delay)
        return line

    def eof(self) -> bool:
        return self._eof

    def close(self) -> None:
        if self._file is not None:
            self._file.close()

    def describe(self) -> str:
        return f"archivo {self._path}"


class ReaderThread(threading.Thread):
    """Hilo que lee lineas de la fuente y las encola; reconecta ante fallos.

    El estado de conexion queda en `self.status` (cadena atomica para el
    GIL); el hilo principal la lee en cada ciclo de sondeo.
    """

    def __init__(self, source: LineSource, line_queue: queue.Queue):
        super().__init__(daemon=True)
        self._source = source
        self._queue = line_queue
        self._stop = threading.Event()
        self.status = "conectando..."

    def stop(self) -> None:
        self._stop.set()

    def run(self) -> None:
        while not self._stop.is_set():
            try:
                self._source.open()
            except (serial.SerialException, OSError) as error:
                self.status = f"sin conexion ({error}) - reintentando..."
                self._stop.wait(RETRY_INTERVAL_S)
                continue

            self.status = self._source.describe()
            while not self._stop.is_set():
                try:
                    line = self._source.readline()
                except (serial.SerialException, OSError) as error:
                    self.status = f"error de lectura ({error}) - reintentando..."
                    break

                if line is None:
                    if self._source.eof():
                        self.status = "fin de fuente"
                        self._queue.put(END_OF_SOURCE)
                        return
                    continue  # timeout sin datos: seguir leyendo

                self._queue.put(line)

            self._source.close()
