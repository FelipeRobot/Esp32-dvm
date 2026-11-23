"""
Dashboard Tkinter con la grafica de tension en tiempo real y el panel de
estadisticas de la ventana movil.

La clase es una vista pura: recibe muestras y estadisticas desde fuera (el
hilo principal orquesta lectura, captura y refresco) y delega las acciones
de los botones en callbacks inyectados.
"""

from __future__ import annotations

import math
import tkinter as tk
from collections import deque
from tkinter import ttk

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

# Segundos de datos mostrados como minimo (la grafica muestra la ventana
# completa aunque aun no haya datos que la llenen).
LINE_COLOR = "#1f77b4"

STATS_CELLS = [
    ("v", "last",  "V actual (V)"),
    ("v", "mean",  "V promedio (V)"),
    ("v", "min",   "V minimo (V)"),
    ("v", "max",   "V maximo / pico (V)"),
    ("v", "rms",   "V RMS (V)"),
    ("i", "last",  "I actual (A)"),
    ("p", "last",  "P actual (W)"),
    ("v", "count", "muestras validas"),
]


def _fmt(value) -> str:
    if value is None or (isinstance(value, float) and math.isnan(value)):
        return "n/a"
    return f"{value:.3f}"


class Dashboard(tk.Tk):
    """Ventana principal de visualizacion en tiempo real."""

    def __init__(self, window_seconds: float = 60.0,
                 on_pause_toggle=None, on_clear=None):
        super().__init__()
        self.title("Monitor de carga DC - visualizacion en tiempo real")
        self.geometry("980x680")
        self.minsize(720, 480)

        self._window_seconds = float(window_seconds)
        self._on_pause_toggle = on_pause_toggle
        self._on_clear = on_clear
        self._paused = False

        # Datos de la grafica: ventana movil de (t, V)
        self._times: deque[float] = deque()
        self._volts: deque[float] = deque()

        self._build_stats_panel()
        self._build_plot()
        self._build_status_bar()

    # ------------------------------ construccion ------------------------------

    def _build_stats_panel(self) -> None:
        panel = ttk.LabelFrame(self, text="Estadisticas de la ventana movil")
        panel.pack(fill="x", padx=8, pady=(8, 4))

        self._stats_vars: dict[tuple[str, str], tk.StringVar] = {}
        for index, (group, key, caption) in enumerate(STATS_CELLS):
            row, col = divmod(index, 4)
            var = tk.StringVar(value="n/a")
            self._stats_vars[(group, key)] = var

            cell = ttk.Frame(panel)
            cell.grid(row=row, column=col, padx=14, pady=4, sticky="w")
            ttk.Label(cell, textvariable=var,
                      font=("TkDefaultFont", 14, "bold")).pack(anchor="w")
            ttk.Label(cell, text=caption).pack(anchor="w")

    def _build_plot(self) -> None:
        self._figure = Figure(figsize=(8, 4), dpi=100, tight_layout=True)
        self._ax = self._figure.add_subplot(111)
        self._canvas = FigureCanvasTkAgg(self._figure, master=self)
        self._canvas.get_tk_widget().pack(fill="both", expand=True,
                                          padx=8, pady=4)

    def _build_status_bar(self) -> None:
        bar = ttk.Frame(self)
        bar.pack(fill="x", padx=8, pady=(0, 8))

        self._status_var = tk.StringVar(value="conectando...")
        ttk.Label(bar, textvariable=self._status_var).pack(side="left")

        self._csv_var = tk.StringVar(value="")
        ttk.Label(bar, textvariable=self._csv_var).pack(side="left", padx=24)

        self._clear_btn = ttk.Button(bar, text="Limpiar vista",
                                     command=self._on_clear)
        self._clear_btn.pack(side="right")

        self._pause_btn = ttk.Button(bar, text="Pausar captura",
                                     command=self._on_pause_toggle)
        self._pause_btn.pack(side="right", padx=(0, 8))

    # ----------------------------- API publica --------------------------------

    def add_sample(self, elapsed: float, sample, stats: dict) -> None:
        """Anade una muestra a la grafica y refresca el panel."""
        self._times.append(elapsed)
        self._volts.append(sample.voltage_V)  # NaN => hueco en la grafica

        cutoff = elapsed - self._window_seconds
        while self._times and self._times[0] < cutoff:
            self._times.popleft()
            self._volts.popleft()

        self._update_stats_labels(stats)
        self._redraw()

    def set_status(self, text: str) -> None:
        self._status_var.set(text)

    def set_csv_path(self, text: str) -> None:
        self._csv_var.set(text)

    def set_paused(self, paused: bool) -> None:
        self._paused = paused
        self._pause_btn.config(text="Reanudar captura" if paused
                               else "Pausar captura")

    def clear(self) -> None:
        """Vacia la grafica y pone el panel en n/a."""
        self._times.clear()
        self._volts.clear()
        for var in self._stats_vars.values():
            var.set("n/a")
        self._redraw()

    # -------------------------------- internos --------------------------------

    def _update_stats_labels(self, stats: dict) -> None:
        for (group, key), var in self._stats_vars.items():
            value = stats.get(group, {}).get(key)
            var.set(_fmt(value))

    def _redraw(self) -> None:
        ax = self._ax
        ax.clear()

        ax.plot(self._times, self._volts, color=LINE_COLOR, linewidth=1.5)
        ax.set_title("Tension de la carga vs tiempo")
        ax.set_xlabel("tiempo (s)")
        ax.set_ylabel("tension (V)")
        ax.grid(True, alpha=0.3)

        # Eje X fijo a la ventana movil: la grafica se desplaza con los datos.
        t_max = max(self._times[-1], self._window_seconds) if self._times \
            else self._window_seconds
        ax.set_xlim(t_max - self._window_seconds, t_max)

        self._canvas.draw_idle()
