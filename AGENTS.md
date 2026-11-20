# AGENTS.md

Guía compacta para trabajar en este repositorio: adquisición eléctrica ESP32 + INA226 sobre PlatformIO (Arduino Framework).

## Comandos

- Build canónico: `make build` (Docker; equivale a `docker compose run --rm build pio run`). No requiere PlatformIO en el host: todo está preinstalado en la imagen.
- `make image` reconstruye la imagen cuando cambian las versiones fijadas (la primera vez descarga ~2 GB de toolchain).
- `make upload` / `make monitor` corren en el **host** con PlatformIO CLI (`pio run -t upload` / `pio device monitor`): el contenedor no usa USB por diseño.
- `pio run` en el host sigue funcionando como alternativa (PlatformIO Core en `~/.platformio/penv`), pero el entorno de referencia es Docker.
- No hay tests todavía: `make test` solo informa. El CI compila; el paso de tests está preparado en el workflow.
- Antes de `make build`, comprobar `docker --version`: si no hay Docker en la máquina, usar `pio run` en host.

## C++17

`platformio.ini` activa C++17 con el par `build_unflags = -std=gnu++11` + `build_flags = -std=gnu++17`. Ambos son necesarios: si se borra el `build_unflags`, el estándar vuelve silenciosamente a gnu++11.

## Docker y versiones fijadas

- Toda dependencia va con versión explícita: imagen base por digest en `docker/Dockerfile`, PlatformIO Core 6.2.0, `espressif32@8.0.0` e `INA226_WE@~1.3.0` en `platformio.ini`. No desfijar versiones sin motivo.
- Cambiar un pin exige reconstruir la imagen (`make image`): el contenedor preinstala los paquetes en el build y compila sin red.
- La imagen crea el usuario `pio` con el UID/GID del host (build args) para que `.pio/` no quede propiedad de root; el CI exporta `UID=$(id -u)` por lo mismo.
- CI en `.github/workflows/ci.yml` (push a `master`/`feature/**` y PRs): mismo procedimiento que local.

## Trampas de la librería INA226_WE (fijada a ~1.3.0)

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

- `master` tiene el firmware base; la infraestructura Docker vive en la rama `feature/docker_resilience`.
- El firmware compila, pero **no ha sido probado sobre hardware real**: no afirmar funcionamiento físico.
- La imagen Docker no se validó localmente al crearla (la máquina de desarrollo no tenía Docker); el CI es la validación real.
