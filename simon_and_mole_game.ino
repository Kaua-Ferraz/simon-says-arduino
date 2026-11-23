// Pinos de conexão com os LEDs (ex: pinos digitais 2, 3, 4 e 5)
const int leds[] = {2, 3, 4, 5};
// Pinos de conexão com os Botões correspondentes
const int botoes[] = {8, 9, 10, 11};
// Pinos do Shift Register (74HC595) para controlar o display
const int DATA = 6;
const int CLOCK = 7;
const int LATCH = 13;
// Pinos que controlam qual dos 4 dígitos do display está ligado no momento
const int pinosDigitos[] = {A0, A1, A2, A3};

// Configurações Gerais dos Jogos
const int TAMANHO_MAXIMO = 20; // Tamanho máximo da sequência do Simão Diz
int jogoSelecionado = 1;       // Define o jogo inicial: 1 = Simon Says, 2 = Bata na Toupeira
int pontuacaoAtual = 0;        // Variável que guarda a pontuação para mostrar no display

// Variáveis específicas para o jogo Simon Says (Simão Diz)
int sequencia[TAMANHO_MAXIMO]; // Guarda a sequência de luzes gerada
int tamanhoSequencia;          // Quantos LEDs a sequência atual tem
int tempoLed;                  // Tempo que o LED fica aceso
int tempoEntre;                // Tempo de pausa entre um LED e outro

// Mapa de Bits (Display 7 Seg)
// Como o meu pino QA está quebrado kkkk, usamos os bits 1 a 7 (QB a QH) para desenhar os números.
const byte NUMEROS[10] = {
  0x7E, // 0
  0x0C, // 1
  0xB6, // 2
  0x9E, // 3
  0xCC, // 4
  0xDA, // 5
  0xFA, // 6
  0x0E, // 7
  0xFE, // 8
  0xDE  // 9
};

// Configuração inicial (roda apenas uma vez quando liga)
void setup()
{
  Serial.begin(9600); // Inicia a comunicação com o computador (Monitor Serial)

  // Configura os pinos de Entradas (botões) e Saídas (LEDs e Display)
  for (int i = 0; i < 4; i++)
  {
    pinMode(leds[i], OUTPUT);           // LED envia luz
    pinMode(botoes[i], INPUT_PULLUP);   // Botão recebe sinal (PULLUP inverte a lógica: LOW = apertado)
    pinMode(pinosDigitos[i], OUTPUT);   // Pinos que ativam cada dígito do display
    digitalWrite(pinosDigitos[i], HIGH); // Inicia com todos os dígitos desligados
  }

  // Configura os pinos de comunicação com o Shift Register
  pinMode(DATA, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(LATCH, OUTPUT);

  // Usa o "ruído" de um pino analógico vazio para gerar números realmente aleatórios
  randomSeed(analogRead(A5)); 
  apagarTodosLeds(); // Garante que tudo comece apagado
}

// Função responsável por desenhar a pontuação no display muito rapidamente (Multiplexação)
void atualizarDisplay()
{
  // Separa o número total (ex: 1234) em casas decimais individuais
  int milhares = (pontuacaoAtual / 1000) % 10; // Extrai o '1'
  int centenas = (pontuacaoAtual / 100) % 10;  // Extrai o '2'
  int dezenas = (pontuacaoAtual / 10) % 10;    // Extrai o '3'
  int unidades = pontuacaoAtual % 10;          // Extrai o '4'

  // Transforma os números no formato de bits que o display entende
  byte digitos[4] = {
    NUMEROS[milhares],
    NUMEROS[centenas],
    NUMEROS[dezenas],
    NUMEROS[unidades]
  };

  // Lógica para apagar os zeros à esquerda (para não mostrar "0005", apenas "5")
  if (pontuacaoAtual < 1000) digitos[0] = 0;
  if (pontuacaoAtual < 100)  digitos[1] = 0;
  if (pontuacaoAtual < 10)   digitos[2] = 0;
  if (pontuacaoAtual == 0)   digitos[3] = NUMEROS[0]; // Se a pontuação for 0, mantém o último zero

  // Loop que pisca um dígito de cada vez muito rápido
  for (int i = 0; i < 4; i++)
  {
    // Apaga todos os dígitos para não misturar os números (efeito fantasma)
    for (int j = 0; j < 4; j++)
    {
      digitalWrite(pinosDigitos[j], HIGH);
    }

    // Prepara o Shift Register e envia o desenho do número atual
    digitalWrite(LATCH, LOW);
    shiftOut(DATA, CLOCK, MSBFIRST, digitos[i]);
    digitalWrite(LATCH, HIGH);

    // Liga apenas o dígito atual para mostrar o número que acabou de ser enviado
    digitalWrite(pinosDigitos[i], LOW);
    delayMicroseconds(2000); // Espera 2 milissegundos para o olho humano captar a luz
  }
}

// Substitui o delay() comum. Ao invés de travar o Arduino, ele conta o tempo 
// e continua atualizando o display para ele não apagar.
void esperar(unsigned long ms)
{
  unsigned long inicio = millis(); // Marca o tempo inicial

  // Enquanto o tempo desejado não passar, fica rodando a função de display
  while (millis() - inicio < ms)
  {
    atualizarDisplay();
  }
}

// Apaga os 4 LEDs de uma vez
void apagarTodosLeds()
{
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(leds[i], LOW);
  }
}

// Acende um LED específico por um tempo e depois o desliga, com um tempo de pausa
void acenderLed(int led, int tempoAcendido, int tempoDesligado)
{
  digitalWrite(leds[led], HIGH);
  esperar(tempoAcendido);

  digitalWrite(leds[led], LOW);
  esperar(tempoDesligado);
}

// Pisca o LED de um botão rapidinho para confirmar uma ação
void piscarBotao(int led)
{
  digitalWrite(leds[led], LOW);
  esperar(100);
  digitalWrite(leds[led], HIGH);
  esperar(100);
  digitalWrite(leds[led], LOW);
  esperar(100);
}

// Fica aguardando o jogador apertar um botão. 
// Tem um timeout opcional. Retorna o número do botão ou -1 se o tempo acabar.
int esperarBotao(unsigned long timeout = 0)
{
  unsigned long inicio = millis(); // Marca quando começou a esperar

  while (true)
  {
    atualizarDisplay(); // Mantém o display aceso enquanto espera

    // Verifica se o tempo limite acabou (usado no Bata na Toupeira)
    if (timeout > 0 && (millis() - inicio >= timeout))
      return -1; // -1 significa "Demorou demais"

    // Verifica se algum dos 4 botões foi pressionado
    for (int i = 0; i < 4; i++)
    {
      if (digitalRead(botoes[i]) == LOW) // LOW = apertado
      {
        digitalWrite(leds[i], HIGH); // Acende o LED do botão apertado

        // Trava o código aqui até o jogador soltar o botão (evita clique duplo sem querer)
        while (digitalRead(botoes[i]) == LOW)
          atualizarDisplay(); 

        digitalWrite(leds[i], LOW); // Apaga ao soltar

        return i; // Retorna qual botão foi apertado (0, 1, 2 ou 3)
      }
    }
  }
}

// Animação de quando o jogador perde o jogo
void gameOver()
{
  Serial.println("\nGAME OVER!");

  // Pisca todos os 4 LEDs três vezes seguidas
  for (int repeticao = 0; repeticao < 3; repeticao++)
  {
    for (int i = 0; i < 4; i++)
      digitalWrite(leds[i], HIGH);
    esperar(300);
    apagarTodosLeds();
    esperar(300);
  }

  esperar(1000); // Pausa antes de voltar ao menu
}

// Simão Diz (Memória)
void iniciarSimon()
{
  Serial.println("\n--- SIMÃO DIZ ---");

  // Configurações iniciais do jogo
  tamanhoSequencia = 1;
  tempoLed = 500;
  tempoEntre = 300;
  pontuacaoAtual = 0;

  // Sorteia o primeiro LED da sequência
  sequencia[0] = random(0, 4);
  esperar(1000);

  // Loop infinito do jogo (só sai se perder)
  while (true)
  {
    pontuacaoAtual = tamanhoSequencia; // Atualiza o display com o nível atual
    esperar(500);

    // Mostra a sequência atual de LEDs para o jogador
    for (int i = 0; i < tamanhoSequencia; i++)
    {
      acenderLed(sequencia[i], tempoLed, tempoEntre);
    }

    // Espera o jogador apertar os botões, verificando um por um
    for (int i = 0; i < tamanhoSequencia; i++)
    {
      // Se ele demorar ou apertar errado (diferente da sequência gerada)
      if (esperarBotao(0) != sequencia[i])
      {
        gameOver(); // O jogador errou
        return;     // Encerra a função do jogo e volta pro loop principal
      }
    }

    // Se o jogador acertou a sequência e ainda não estourou o limite da memória
    if (tamanhoSequencia < TAMANHO_MAXIMO)
    {
      // Adiciona mais um LED aleatório na lista
      sequencia[tamanhoSequencia] = random(0, 4);
      tamanhoSequencia++;
    }

    // Deixa o jogo mais rápido e difícil a cada rodada
    if (tempoLed > 150)   tempoLed -= 30;
    if (tempoEntre > 100) tempoEntre -= 20;

    esperar(500); // Pausa antes da próxima rodada
  }
}

// Bata na Toupeira (Reflexos)
void iniciarMole()
{
  Serial.println("\n--- BATA NA TOUPEIRA ---");

  pontuacaoAtual = 0;
  int tempoParaPegar = 1200; // Começa dando 1.2 segundos para reagir

  esperar(1000);

  // Loop infinito do jogo
  while (true)
  {
    int ratoAtual = random(0, 4); // Sorteia qual LED vai acender

    digitalWrite(leds[ratoAtual], HIGH); // Acende o LED sorteado

    // Espera o jogador bater com um tempo limite
    int botaoPressionado = esperarBotao(tempoParaPegar);

    digitalWrite(leds[ratoAtual], LOW); // Apaga o LED

    // Verifica se acertou o botão certo no tempo certo
    if (botaoPressionado == ratoAtual)
    {
      pontuacaoAtual++; // Ganha um ponto

      // Aumenta a dificuldade deixando o tempo mais curto
      if (tempoParaPegar > 300)
        tempoParaPegar -= 40; 

      esperar(300); // Pausa curtinha antes do próximo LED
    }
    else // Se bateu no botão errado ou demorou demais
    {
      gameOver();
      return; // Sai do jogo
    }
  }
}

// Acende o LED correspondente ao jogo escolhido para facilitar a visualização
void mostrarSelecaoMenu()
{
  apagarTodosLeds();
  if (jogoSelecionado == 1)
    digitalWrite(leds[2], HIGH); // LED Azul indica Simão Diz
  else
    digitalWrite(leds[3], HIGH); // LED Amarelo indica Bata na Toupeira
}

// Tela de Seleção de Jogo
void menu()
{
  jogoSelecionado = 1; // Padrão
  mostrarSelecaoMenu();

  while (true)
  {
    pontuacaoAtual = jogoSelecionado; // Mostra "1" ou "2" no display
    atualizarDisplay();

    // Lógica do Botão Vermelho (botoes[0]): Alternar o Jogo
    if (digitalRead(botoes[0]) == LOW)
    {
      piscarBotao(0);

      // Espera soltar
      while (digitalRead(botoes[0]) == LOW)
        atualizarDisplay();

      // Troca entre 1 e 2
      jogoSelecionado = (jogoSelecionado == 1) ? 2 : 1;
      mostrarSelecaoMenu();
      esperar(200); // Pequena pausa (Debounce) para evitar clique duplo
    }

    // Lógica do Botão Verde (botoes[1]): Confirmar e Iniciar
    if (digitalRead(botoes[1]) == LOW)
    {
      piscarBotao(1);

      // Espera soltar
      while (digitalRead(botoes[1]) == LOW)
        atualizarDisplay();

      apagarTodosLeds();

      // Faz o display piscar 3 vezes para anunciar que o jogo vai começar
      for (int p = 0; p < 3; p++)
      {
        pontuacaoAtual = 0; // Apaga a pontuação (mostrando zero não visível ou vazio)
        for (int j = 0; j < 4; j++)
          digitalWrite(pinosDigitos[j], HIGH); // Força os dígitos a desligarem fisicamente
        delay(150);

        pontuacaoAtual = jogoSelecionado; // Volta o número do jogo selecionado
        esperar(150);
      }
      esperar(500);
      return; // Sai do menu e avança no código
    }
  }
}

// A função principal que repete para sempre no Arduino
void loop()
{
  menu(); // Abre a tela de escolha

  // Checa a variável definida no menu para abrir o jogo correto
  if (jogoSelecionado == 1)
  {
    iniciarSimon();
  }
  else
  {
    iniciarMole();
  }
  // Quando o jogo termina (return), o loop recomeça chamando o menu() de novo.
}