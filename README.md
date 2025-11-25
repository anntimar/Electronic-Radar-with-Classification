# Radar Eletrônico – Zephyr RTOS (mps2/an385)

**Disciplina:** Sistemas Embarcados  
**Plataforma:** Zephyr RTOS + QEMU (`mps2/an385`)  

**Equipe:**
- Maria Antonia Ribeiro Ramos  
- Lucas Cassiano Maciel dos Santos  

---

## 1. Visão geral

Este projeto implementa um **radar eletrônico simplificado** rodando em cima do **Zephyr RTOS**, usando a plataforma emulada **ARM MPS2 AN385** (QEMU).

O sistema:

- Simula a passagem de veículos pelo radar.
- Classifica o veículo como **LEVE** (2 eixos) ou **PESADO** (≥ 3 eixos).
- Calcula uma **velocidade em km/h** (atualmente simulada).
- Compara com **limites de velocidade diferentes** para LEVE e PESADO.
- Exibe o resultado no “display” usando o **console do QEMU com cores ANSI**:
  - **Verde** → NORMAL
  - **Amarelo** → ALERTA
  - **Vermelho** → INFRAÇÃO
- Em caso de infração, aciona uma **câmera simulada** via **ZBUS**, que gera uma placa no padrão Mercosul e informa se ela é válida ou não.

O objetivo é praticar:

- Multithreading no Zephyr.
- Comunicação entre threads (fila de mensagens e ZBUS).
- Configuração via **Kconfig**.
- Uso de logs, console e testes com **ztest**.

---

## 2. Requisitos e ambiente

Foi usado o seguinte ambiente:

- WSL + Ubuntu
- Zephyr instalado (workspace `zephyrproject/`)
- **west** instalado
- **Zephyr SDK** instalado em `~/zephyr-sdk`

Variáveis de ambiente (normalmente colocadas no `~/.bashrc`):

```bash
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=$HOME/zephyr-sdk
```

Antes de compilar, sempre rodar:

```bash
cd ~/zephyrproject
source zephyr/zephyr-env.sh
```

---

## 3. Como compilar e rodar

Assumindo o projeto clonado em `~/workspace/radar`:

```bash
cd ~/workspace/radar

# Build para a plataforma mps2/an385
west build -b mps2_an385 . -d build
```

Se o build terminar sem erro, para executar no QEMU:

```bash
west build -t run -d build
```

Ao rodar, o console deve mostrar algo parecido com:

```text
*** Booting Zephyr OS build v4.2.0 ***
[00:00:00.000,000] <inf> radar_main: Radar eletrônico iniciado
[RADAR] NORMAL   | 52 km/h | PESADO
[RADAR] ALERTA   | 72 km/h | LEVE
[RADAR] INFRACAO | 125 km/h | LEVE
...
[00:00:06.550,000] <inf> radar_camera: Camera publicou placa: SBU7S08 (valida)
```

Para encerrar o QEMU: fechar a janela ou usar `Ctrl + A`, depois `X` (dependendo da configuração).

---

## 4. Estrutura do repositório

```text
radar/
├─ CMakeLists.txt      # Configuração de build da aplicação
├─ Kconfig             # Opções específicas do radar
├─ prj.conf            # Configurações da aplicação (kernel, log, ZBUS, random)
├─ app.overlay         # Overlay de devicetree (placeholder)
├─ README.md
├─ include/
│  └─ radar.h          # Tipos, structs e protótipos compartilhados
├─ src/
│  ├─ main.c           # Função main, criação das threads e filas
│  ├─ sensors.c        # Thread de sensores (simulação de eixos + velocidade)
│  ├─ control.c        # Thread de controle (limites, status, chamada da câmera)
│  ├─ camera.c         # Thread da câmera (ZBUS, geração/validação de placa)
│  ├─ display.c        # Thread de display (cores ANSI + console)
│  └─ utils.c          # Funções auxiliares (cálculo, classificação, placa)
└─ tests/
   ├─ speed/
   │  ├─ CMakeLists.txt
   │  ├─ testcase.yaml
   │  └─ src/test_speed.c  # Testes de velocidade e classificação
   └─ plate/
      ├─ CMakeLists.txt
      ├─ testcase.yaml
      └─ src/test_plate.c  # Testes de validação de placa
```

---

## 5. Configuração via Kconfig

No arquivo `Kconfig` foram definidas opções específicas do radar:

- `CONFIG_RADAR_SENSOR_DISTANCE_MM`  
  Distância entre sensores (mm).  
  > Hoje não está sendo usada diretamente no cálculo, mas a função de utilidade já suporta isso.

- `CONFIG_RADAR_SPEED_LIMIT_LIGHT_KMH`  
  Limite de velocidade para veículos **leves** (km/h).

- `CONFIG_RADAR_SPEED_LIMIT_HEAVY_KMH`  
  Limite de velocidade para veículos **pesados** (km/h).

- `CONFIG_RADAR_WARNING_THRESHOLD_PERCENT`  
  Percentual do limite a partir do qual o status entra em **ALERTA** (cor amarela).

- `CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT`  
  Porcentagem de chance da câmera simular uma **falha** (gerar placa inválida).

Essas opções podem ser alteradas via:

```bash
cd ~/workspace/radar
west build -b mps2_an385 . -d build -t menuconfig
```

---

## 6. Como o código está organizado

### 6.1. `include/radar.h`

- Define os tipos básicos:

  - `vehicle_type_t` → LEVE ou PESADO  
  - `radar_status_t` → NORMAL, WARNING, INFRACTION

- Estruturas usadas entre as threads:

  - `struct sensor_sample` → dados vindo dos sensores (eixos + velocidade)  
  - `struct display_msg` → dados prontos para exibição (amostra + status + placa)

- Protótipos das funções utilitárias:

  - `radar_calc_speed_kmh(...)`
  - `radar_classify_vehicle(...)`
  - `radar_validate_plate(...)`
  - `radar_generate_plate(...)`

- Protótipos das funções das threads:

  - `sensors_thread`, `control_thread`, `display_thread`, `camera_thread`

---

### 6.2. `src/main.c`

Responsável por:

- Declarar as filas:

  - `g_sensor_msgq` → sensores → controle  
  - `g_display_msgq` → controle → display  

- Definir o canal ZBUS da câmera (`camera_chan`).
- Definir pilha e dados das threads (`K_THREAD_STACK_DEFINE`).
- Criar as threads na função `main()`:

  - `sensors_thread`
  - `control_thread`
  - `display_thread`
  - `camera_thread`

---

### 6.3. `src/sensors.c`

- Implementa a `sensors_thread`.
- A cada ciclo:

  - Sorteia número de eixos:
    - 2 → veículo LEVE
    - 3 → veículo PESADO
  - Gera uma velocidade **aleatória** entre ~30 e 140 km/h usando `sys_rand32_get()`.
  - Preenche um `struct sensor_sample` e envia para `g_sensor_msgq`.

> **Importante:**  
> A função `radar_calc_speed_kmh()` já está pronta para calcular a velocidade com base em **distância** + **tempo**, usando `CONFIG_RADAR_SENSOR_DISTANCE_MM`.  
> No momento, o projeto está usando a abordagem simplificada (velocidade aleatória), mas seria fácil substituir a parte da simulação para algo do tipo:
> - simular `time_ms`,
> - ler `distance_mm` do Kconfig,
> - e calcular `speed_kmh = radar_calc_speed_kmh(distance_mm, time_ms)`.

---

### 6.4. `src/control.c`

- Implementa a `control_thread`.
- Lê os limites de velocidade do Kconfig.
- Para cada `sensor_sample` recebido:

  1. Classifica o veículo com `radar_classify_vehicle(axes)`.
  2. Escolhe o limite correto (leve ou pesado).
  3. Calcula se a velocidade está:
     - Abaixo do limite → **NORMAL**
     - Acima de `limit * (warning_percent/100)` e abaixo do limite → **ALERTA**
     - Acima do limite → **INFRAÇÃO**
  4. Em caso de INFRAÇÃO:
     - Publica um trigger no `camera_chan` (ZBUS).
     - Aguarda a resposta com a placa e se ela é válida.
  5. Preenche um `display_msg` com tudo (status, tipo, velocidade, placa, validade) e envia para `g_display_msgq`.

---

### 6.5. `src/display.c`

- Implementa a `display_thread`.
- Define códigos ANSI de cor:

  - Verde → NORMAL  
  - Amarelo → ALERTA  
  - Vermelho → INFRAÇÃO  

- Para cada mensagem recebida:

  - Monta uma linha no formato:

    ```text
    [RADAR] STATUS | XX km/h | TIPO | placa=XXXXX (valida/invalida)
    ```

  - Usa a cor ANSI de acordo com o status.
- Isso simula o **Display Dummy** usando o console do QEMU.

---

### 6.6. `src/camera.c`

- Implementa a `camera_thread`.
- Fica escutando o canal ZBUS `camera_chan`.
- Quando recebe um trigger:

  - Simula processamento (sleep).
  - Gera uma placa com `radar_generate_plate(...)`.
  - Valida a placa com `radar_validate_plate(...)`.
  - Publica a resposta no mesmo canal (placa + flag de válido).

A chance de a placa ser inválida é configurada por `CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT`.

---

### 6.7. `src/utils.c`

Funções auxiliares principais:

- `radar_calc_speed_kmh(distance_mm, time_ms)`  
  Calcula a velocidade em km/h com base na distância entre sensores e tempo de passagem.

- `radar_classify_vehicle(axes)`  
  2 eixos → LEVE, 3 ou mais → PESADO.

- `radar_validate_plate(plate)`  
  Verifica se a placa bate com um padrão Mercosul simplificado: `LLLNLNN`.

- `radar_generate_plate(out, len, valid)`  
  - Usa a taxa de falha do Kconfig para decidir se vai gerar uma placa “válida” ou “inválida”.
  - Quando válida, segue o padrão Mercosul; quando inválida, gera algo aleatório.

---

## 7. Testes automatizados

### 7.1. Testes de velocidade e classificação

Pasta: `tests/speed`

- Testa:
  - `radar_calc_speed_kmh()` com cenários conhecidos.
  - `radar_classify_vehicle()` para diferentes números de eixos.

Exemplo de comando (ajustar plataforma para o que existir no seu ambiente, ex.: `native_sim`, `native_sim_64`, `native_posix`):

```bash
west twister -T tests/speed -p native_sim
```

### 7.2. Testes de placa

Pasta: `tests/plate`

- Testa:
  - `radar_validate_plate()` com placas válidas/inválidas.

```bash
west twister -T tests/plate -p native_sim
```

---

## 8. Comentário final

O projeto cumpre o objetivo de implementar um **radar eletrônico simplificado** em Zephyr, explorando:

- Threads separadas para sensores, controle, display e câmera.
- Comunicação por filas e ZBUS.
- Configuração flexível via Kconfig.
- Feedback visual colorido no console.
- Testes automatizados das partes críticas (cálculo, classificação, placa).

A parte de velocidade hoje está mais focada na **simulação aleatória**, mas a estrutura do código já deixa preparado para evoluir para um cenário mais físico, usando **distância entre sensores** e **tempo de passagem** medido ou simulado.
