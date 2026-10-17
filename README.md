# Nacho AZ Mods

Colección de mods de **NachoCasta** para el XDJ-AZ, con código, herramientas y notas de investigación.
Este repositorio es privado. El catálogo de la aplicación y del AZ necesita URLs públicas;
por ahora no hay una publicación pública ni una entrada en el Mod Index.

## Empezar aquí

- [Estado actual y cómo continuar](docs/CURRENT-STATE.md).
- [Publicación y organización en Orca](docs/PUBLISHING.md).
- [Índice de investigación](docs/RESEARCH-INDEX.md).
- [Recursos externos de Ghidra, firmware y compilación](docs/LOCAL-RESOURCES.md).
- [Origen, licencias y binario validado](docs/PROVENANCE.md).

## Hub USB Libraries

Permite navegar las bibliotecas rekordbox de los pendrives de un hub, con servidores y
cachés propios por fuente, dentro del AZ. No necesita un ordenador ni reproductores externos.
El límite actual es ocho fuentes del hub. Cinco se probaron simultáneamente en el emulador;
no afirmamos haber probado ocho en hardware.

La **0.1.4** fue instalada por SSH y el usuario confirmó que funciona perfectamente en su AZ.
La **0.1.5** es la preparación para distribución: metadatos, icono y empaquetado estándar;
no está instalada en el equipo ni publicada. Ver el estado exacto antes de probar.

Código: [src/azhub/linkfake](src/azhub/linkfake). Módulo de compatibilidad del kernel:
[src/azhub/azhub.c](src/azhub/azhub.c). El header del SDK está en
[src/azmod/azmod-mod.h](src/azmod/azmod-mod.h).

Se conserva esta estructura para evitar perder referencias y contexto del trabajo original.
Los futuros mods pueden usar sus propias carpetas bajo `mods/<id>/`; cada uno tendrá su
manifest, compilación, licencia y versiones independientes.

## Referencias locales

Las capturas se conservan en `src/azhub/research/shots/`, ignoradas por Git. No se respaldan
ni se suben a GitHub por decisión del usuario. Tampoco se suben imágenes USB, música,
firmware, proyectos completos de Ghidra, desensamblados completos ni claves SSH.
Los índices y las notas sí se versionan. Este repositorio contiene scripts de investigación
históricos: leer sus comentarios antes de ejecutarlos; algunos modifican el emulador/equipo.

## Compilar y empaquetar

SDK fijado: `Kyle-Hosman/az-mods-sdk`, `v1.2.3`, commit
`62a156abf3a4789e9e3cdd610fe106227a3a8967`.

```sh
git clone --branch v1.2.3 --depth 1 https://github.com/Kyle-Hosman/az-mods-sdk .azmod-sdk
python3 .azmod-sdk/tools/azmod-pack build src/azhub/linkfake
python3 .azmod-sdk/tools/azmod-pack package src/azhub/linkfake
```

En Windows, usar WSL/Docker para compilar. La validación/empaquetado también funciona con
Python en Windows. `src/azhub/linkfake/build.sh` conserva el método Linaro usado para0.1.4;
`AZHUB_TOOLCHAIN_PREFIX` permite seleccionar el toolchain.

El código del mod y el SDK usan GPL-3.0-only; `azhub.c` y su módulo de kernel usan
GPL-2.0-only. Ver [PROVENANCE](docs/PROVENANCE.md). No es un producto oficial de AlphaTheta.
