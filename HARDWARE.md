# Hardware e pinagem

## Controlador suportado

- placa ESP32-2432S028, revisão com USB-C e micro-USB;
- módulo ESP32-WROOM-32;
- LCD ST7789 de 240 × 320 em orientação vertical;
- touch resistivo XPT2046;
- leitor microSD integrado;
- LED RGB integrado.

A pinagem abaixo é específica desta revisão. Outra variante da CYD deve ser
validada antes de usar o mesmo binário.

## Pinagem confirmada

| Função | Pinos ESP32 |
|---|---|
| TFT SPI | MISO 12, MOSI 13, SCLK 14, CS 15, DC 2, RST interno |
| Backlight | GPIO 21, PWM |
| Touch XPT2046 | MISO 39, MOSI 32, SCLK 25, CS 33, IRQ 36 |
| microSD | MISO 19, MOSI 23, SCLK 18, CS 5 |
| LED RGB | R 4, G 16, B 17 |
| Sensor térmico DS18B20 | GPIO 22, resistor pull-up externo de 4,7 kΩ |
| Módulo de relé | GPIO 27, ativo em nível alto |

Display, touch e SD usam configurações/barramentos próprios. O touch roda por
SPI em software a 1 MHz; o SD usa VSPI a 4 MHz.

## Configuração do display

- rotação `0`, geometria final 240 × 320;
- clock de escrita SPI: 80 MHz;
- leitura: 16 MHz;
- inversão de cores desabilitada;
- formato LVGL: RGB565 com bytes trocados;
- um buffer DMA parcial de 240 × 24 pixels;
- o buffer só é devolvido ao LVGL após a transferência DMA terminar;
- sincronização por scanline tenta iniciar, mas `FALLBACK` é esperado nesta
  placa quando o ST7789 não devolve uma linha confiável.

Se surgirem ruído, pixels errados ou instabilidade, o primeiro teste conservador
é reduzir `kDisplayWriteClockHz` em `include/BuildConfig.h` para 40 MHz.

## Touch e recalibração

A calibração é salva na NVS (`touch-cal`). Para forçar nova calibração:

1. em `platformio.ini`, adicione ou altere a flag para
   `-D KEEZER_FORCE_TOUCH_CALIBRATION=1`;
2. compile, grave e toque nos alvos apresentados;
3. retorne a flag para `0`, compile e grave novamente.

Deixar a flag em `1` repete a calibração em todo boot.

## microSD

Use cartão confiável formatado em FAT32. ExFAT não é recomendado para a pilha
Arduino SD usada aqui. Com o controlador desligado, insira o cartão e ligue-o;
o serial deve mostrar `microSD mounted` e a tela `CONFIG. > SISTEMA` deve indicar
`SD OK`. O cartão guarda histórico, não o catálogo principal dos KEGs.

## Temperatura e compressor

O hardware térmico está habilitado com proteções:

- DS18B20 externo alimentado em três fios, dados no GPIO 22 e resistor de
  4,7 kΩ entre dados e 3,3 V;
- GPIO 27 colocado em LOW/OFF antes de ser configurado como saída;
- módulo de relé confirmado como ativo em HIGH;
- leitura 1-Wire com CRC e detecção de desconexão;
- sensor inválido, ausente, fora da faixa ou sem comunicação força OFF;
- mínimo de 180 s desligado e 60 s ligado protege o compressor.

Não conecte compressor de rede elétrica diretamente ao ESP32. Use módulo de
relé/contator dimensionado, isolamento e proteção elétrica apropriados. Valide
primeiro com a carga de potência desconectada e confirme com multímetro que
boot, reset, perda do sensor e OTA mantêm a saída em OFF.

## Balança

A balança atual é um equipamento separado, conectado pela rede Wi-Fi. O
controlador aceita NFC presente ou ausente. A montagem e calibração do HX711 e
do PN532 pertencem ao firmware da balança; o contrato de dados está em
`SCALE_PROTOCOL.md`.
