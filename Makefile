# Comandos unificados del proyecto (ver README).
# La compilacion se hace en Docker; la carga y el monitor, en el host.

COMPOSE ?= docker compose
SERVICE := build

.PHONY: help image build clean test upload monitor shell

help:
	@echo "Comandos disponibles:"
	@echo "  make image    Construye la imagen Docker (la primera vez descarga ~2 GB)"
	@echo "  make build    Compila el firmware en el contenedor"
	@echo "  make clean    Limpia los artefactos de compilacion (.pio/)"
	@echo "  make test     Ejecuta las pruebas (ninguna definida todavia)"
	@echo "  make upload   Carga el firmware desde el HOST (requiere PlatformIO CLI)"
	@echo "  make monitor  Monitor serie desde el HOST (requiere PlatformIO CLI)"
	@echo "  make shell    Abre una shell dentro del contenedor"

image:
	$(COMPOSE) build

build:
	$(COMPOSE) run --rm $(SERVICE) pio run

clean:
	$(COMPOSE) run --rm $(SERVICE) pio run -t clean

test:
	@echo "No hay pruebas definidas todavia (no existe el directorio test/)."
	@echo "Cuando existan, este target ejecutara: $(COMPOSE) run --rm $(SERVICE) pio test"

upload:
	pio run -t upload

monitor:
	pio device monitor

shell:
	$(COMPOSE) run --rm $(SERVICE) bash
