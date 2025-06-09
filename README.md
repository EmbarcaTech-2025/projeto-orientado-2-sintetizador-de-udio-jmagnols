# Projetos de Sistemas Embarcados - EmbarcaTech 2025

Autor: João Magno Lourenço Soares

Curso: Residência Tecnológica em Sistemas Embarcados

Instituição: EmbarcaTech - HBr

Brasília, junho de 2025

# Sintetizador de Áudio

Este projeto transforma a placa BitDogLab em um sintetizador de áudio simples, capaz de gravar, filtrar, visualizar e reproduzir clipes de som. Ele utiliza o microfone, os botões, o LED RGB e o display OLED da placa para criar uma experiência interativa completa.

## Vídeo do projeto

[![Vídeo do sintetizador](https://img.youtube.com/vi/Fc04BosU7tY.jpg)](https://youtube.com/shorts/Fc04BosU7tY?feature=share)

## Visão Geral

O sintetizador opera através de uma máquina de estados controlada por dois botões:
* **Botão A:** Inicia a gravação de um clipe de áudio de 2 segundos.
* **Botão B:** Reproduz o último clipe de áudio gravado.

### Implementações extra

* Após a gravação, um filtro de **Janela de Blackman** é aplicado para suavizar o áudio, e a forma de onda resultante é exibida no display OLED.

* Além disso, o código roda com clkdiv=0, isto é, os delays são definidos ao longo da execução do código, fazendo com que seja possível configurar a frequência de gravação diferente da de reprodução caso isso seja desejado (pode ser usado para fazer a reprodução da voz de forma mais fina, por exemplo)

## Estrutura de Arquivos

O projeto utiliza a seguinte estrutura de pastas:

```
sintetizador_de_audio/
|-- hal/                     # camada HAL
|   |-- hal_buttons.h
|   |-- hal_buttons.c
|   |-- hal_led.h
|   |-- hal_led.c
|   |-- hal_mic.h
|   |-- hal_mic.c
|   |-- hal_buzzer.h
|   |-- hal_buzzer.c
|   |-- hal_display.h
|   |-- hal_display.c
|-- include/                  # Arquivos de cabeçalho (.h) para drivers
|   |-- ssd1306.h
|   |-- ssd1306_font.h
|   |-- ssd1306_i2c.h
|-- src/                  # Arquivos de implementação (.c) para drivers
|   |-- ssd1306_i2c.c
|-- main.c                # Lógica principal do aplicativo
|-- CMakeLists.txt        # Arquivo de configuração da compilação
```

## Como Funciona

#### 1. Gravação de Áudio com ADC
O som ambiente é capturado pelo microfone da placa, conectado ao **GPIO 28 (Canal ADC 2)**. O sinal analógico é digitalizado a uma taxa de **16000 amostras por segundo (8 kHz)** por 5 segundos. As amostras de 12 bits (0-4095) são armazenadas em um buffer na memória (`sample_buffer`).

#### 2. Máquina de Estados e Controle por Botões
O fluxo do programa é gerenciado por uma máquina de estados simples controlada por interrupções nos botões:
* **`STATE_IDLE`**: Estado inicial. O LED azul pisca, e o display convida o usuário a gravar. Aguarda o Botão A.
* **`STATE_RECORDING`**: Acionado pelo Botão A. O LED vermelho acende, e o display exibe "Gravando...". Ao final, aplica o filtro, exibe a forma de onda e transita para `STATE_HAS_RECORDING`.
* **`STATE_HAS_RECORDING`**: Indica que um áudio está na memória, pronto para ser reproduzido. O LED azul fica fixo, e a forma de onda permanece no display. Aguarda o Botão A (para gravar novo áudio) ou o Botão B (para tocar).
* **`STATE_PLAYBACK`**: Acionado pelo Botão B. O LED verde acende, e o display exibe "Tocando...". Ao final, retorna para `STATE_HAS_RECORDING`.

#### 3. Filtro de Janela de Blackman
Este é o passo crucial para melhorar a qualidade do som.
* **Problema:** Gravar um trecho de áudio cria cortes abruptos no início e no fim, que soam como "cliques".
* **Solução:** A janela de Blackman é um filtro matemático que suaviza as bordas do áudio gravado, funcionando como um "fade-in" e "fade-out" muito rápidos. Ela é aplicada multiplicando-se cada amostra do áudio por um coeficiente.
* **Fórmula:**
    $w(n) = 0.42 - 0.5 \cos\left(\frac{2\pi n}{N-1}\right) + 0.08 \cos\left(\frac{4\pi n}{N-1}\right)$
* **Implementação:** O filtro é aplicado ao sinal de áudio (componente AC) subtraindo-se o offset DC (valor de silêncio, ~2048), multiplicando pelo coeficiente da janela e, em seguida, somando o offset de volta.

#### 4. Visualização no Display OLED
O display OLED, conectado via I2C nos pinos **SDA 14** e **SCL 15**, fornece feedback visual:
* Exibe mensagens de status para guiar o usuário.
* Após a gravação e filtragem, desenha uma representação da forma de onda do áudio. Para caber nos 128 pixels de largura da tela, o programa mapeia as 16.000 amostras, desenhando uma linha vertical para cada conjunto de 125 amostras. A altura da linha representa a amplitude da amostra.

#### 5. Reprodução com PWM
O áudio filtrado é reproduzido pelo buzzer no **GPIO 21**. Cada amostra do buffer é usada para definir a largura do pulso de um sinal PWM, fazendo o buzzer vibrar e recriar o som original.

## Como Usar
1.  **Ligar a Placa:** Conecte a BitDogLab. O **LED azul começará a piscar**, e o display mostrará `Aperte A->Gravar`.
2.  **Gravar:** Pressione o **Botão A**. O **LED vermelho** acenderá por 2 segundos. Faça um som alto (como uma palma) durante este tempo.
3.  **Visualizar:** Ao final da gravação, o **LED vermelho se apagará**, a forma de onda do seu som aparecerá no display, e o **LED azul ficará aceso (fixo)**.
4.  **Tocar:** Pressione o **Botão B**. O **LED verde** acenderá, e o buzzer tocará o som gravado. Ao final, a forma de onda reaparecerá no display, e o LED azul voltará a ficar aceso.
5.  **Repetir:** Você pode pressionar o Botão B para ouvir novamente ou o Botão A para iniciar uma nova gravação.

## Compilação
Para compilar o projeto, certifique-se de que o Pico SDK está configurado em seu ambiente. Em seguida, utilize o compilador ninja da extensão da Rasp Pico.
Após a compilação, o arquivo `synth_audio.uf2` será gerado dentro da pasta `build`.

## 📜 Licença
GNU GPL-3.0.

