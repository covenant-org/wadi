# WADI

Streaming Video service for WebRTC with WHIP

## Prerequisitos

- CMake 3.5+
- C++17 compiler
- libX11-dev
- libXrandr-dev


## Compilación


```bash
cmake -B build -H .
cmake --build build
```

_Nota: Se incluye la librería WebRTC precompilada para ubuntu 22.04 y arm64, 
si se requiere otra distribución se deberá recompilar_

## Ejecución

```bash
./build/wadi -c yuyv -d 0 -h 720 -w 1280 -r 60 http://localhost:8889/wadi/whip
```
- -c formato de video
- -d dispositivo de captura (puede ser index o path)
- -h altura
- -w ancho
- -r fps
- url de streaming

Para los formatos de altura y ancho son los ofertados por el dispositivo de captura

