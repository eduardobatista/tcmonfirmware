# TCMon — Firmware

Firmware do Arduino Nano do sistema TCMon: 8 termopares multiplexados lidos
pelo Termopar Nanoshield (MAX31856), 2 entradas analógicas auxiliares e a
saída do aquecedor.

| Pasta | Versão | Recursos |
|---|---|---|
| `TCMon01/` | TCMon01 1.1 | Termopares e junta fria. Leitura de "pressão" em A5 (`p`). Sem controle nem entradas auxiliares. |
| `TCMon02/` | TCMon02 2.1 | Termopares, entradas auxiliares A0/A1 (`a`) e aquecedor com PWM lento e desligamento por timeout (`m`). |

`hardware/` contém o esquemático e a placa (v.01). `historico/` guarda um
sketch experimental antigo, com PID no Arduino, que não é usado.

## Protocolo serial (19200 8N1)

| Comando | Resposta | TCMon01 | TCMon02 |
|---|---|:-:|:-:|
| `h` | `k` (handshake) | ✓ | ✓ |
| `v` | versão em texto, terminada em `\n` (ex.: `TCMon02 2.1`) | ✓ | ✓ |
| `s<T>` | define o tipo de termopar (B E J K N R S T). O TCMon02 responde `k<T>`; o TCMon01 não responde. | ✓ | ✓ |
| `r<0-7>` | 5 bytes: temperatura (3 bytes com sinal, /4096 °C) + junta fria (2 bytes com sinal, /256 °C). Em falha: `0x80`, registrador de falhas, código (`00` aberto, `01` over/under, `02` int. OOR, `04` ext. OOR, `FF` outra), junta fria (2 bytes). | ✓ | ✓ |
| `c` | junta fria (2 bytes) | ✓ | ✓ |
| `p` | ADC de A5 (2 bytes) | ✓ | |
| `a<0-1>` | ADC de A0/A1 (2 bytes big-endian, 0–1023 = 0–5 V) | | ✓ |
| `m<byte>` | potência do aquecedor 0–100 %, sem resposta. Desliga após 5 s sem `m`. | | ✓ |

## Compilar

Requer `arduino-cli` com o core `arduino:avr`.

```shell
./build.sh
```

Gera `hex/TCMon01.hex` e `hex/TCMon02.hex`. Se `../tcmonrust` existir, copia os
dois para `../tcmonrust/firmware/`, onde ficam embutidos no software, que tem
um gravador próprio (menu **Firmware**).

Para gravar direto com o `arduino-cli`:

```shell
arduino-cli upload --fqbn arduino:avr:nano:cpu=atmega328old -p /dev/ttyUSB0 TCMon02
```

## Histórico de alterações

- **TCMon02 2.1 / TCMon01 1.1**: comando `v` (versão). `printErrors` sempre
  responde 5 bytes; antes, uma falha sem código conhecido gerava resposta de
  4 bytes e travava o software à espera do quinto byte.
