# Sistema Distribuído Mestre-Escravo em C++

Este projeto implementa um sistema distribuído com arquitetura Mestre-Escravo em C++17. O sistema utiliza threads para paralelismo, comunicação via HTTP REST e contêineres Docker para orquestrar os serviços de back-end.

O objetivo é processar um arquivo de texto enviado por um cliente, contar o número de letras e dígitos de forma concorrente em servidores escravos, e retornar o resultado consolidado.

## ⚙️ Funcionamento e Arquitetura

O sistema é dividido em três componentes principais:

* **Servidor Mestre (Container 1):**
    * Atua como o orquestrador central do sistema.
    * Recebe requisições HTTP do cliente contendo o texto a ser processado.
    * Dispara duas threads em paralelo para se comunicar com os servidores escravos.
    * Antes de enviar o trabalho, verifica a saúde de cada escravo através de um endpoint `/health`.
    * Aguarda a resposta de ambos os escravos, consolida os resultados em um único objeto JSON e o devolve ao cliente.

* **Servidores Escravos (Containers 2 e 3):**
    * **Escravo 1 (Letras):** Um microserviço dedicado que expõe o endpoint `/letras`. Ele recebe um texto e retorna a contagem total de caracteres alfabéticos.
    * **Escravo 2 (Números):** Um microserviço similar que expõe o endpoint `/numeros`. Ele recebe um texto e retorna a contagem total de dígitos numéricos.

* **Cliente (Executado localmente):**
    * A interface do usuário para interagir com o sistema. Possui duas versões:
        1.  **GUI (Graphical User Interface):** Uma aplicação com janela gráfica feita em GTK+.
        2.  **CLI (Command-Line Interface):** Uma aplicação de terminal simples e direta.
    * O cliente é responsável por ler um arquivo `.txt`, enviá-lo para o Mestre e exibir a resposta final. Todo o processamento dos dados é feito nos servidores.

## 🛠️ Tecnologias Utilizadas

* **Linguagem:** C++17
* **Comunicação entre Processos:** HTTP REST (utilizando a biblioteca `cpp-httplib`)
* **Concorrência:** `std::async` e `std::future`
* **Manipulação de Dados:** `nlohmann/json` para a resposta consolidada
* **Interface Gráfica:** GTK+ 3.0
* **Conteinerização:** Docker
* **Orquestração:** Docker Compose

## 🚀 Como Compilar e Executar

Siga os passos abaixo para colocar todo o sistema em funcionamento.

### Pré-requisitos

1.  **Para os Servidores (Obrigatório):**
    * [Docker](https://www.docker.com/get-started)
    * [Docker Compose](https://docs.docker.com/compose/install/)

2.  **Para o Cliente (Escolha um ambiente):**
    * **Ambiente WSL/Ubuntu:** Veja as instruções na seção correspondente.
    * **Ambiente Windows Nativo:** Requer a instalação do **MSYS2** com o toolchain MinGW-w64.

### Passo 1: Iniciar os Servidores (Mestre e Escravos)

1.  Clone este repositório para a sua máquina local.
2.  Abra um terminal (CMD ou PowerShell) na pasta raiz do projeto.
3.  Execute o comando abaixo para construir as imagens Docker e iniciar os contêineres:
    ```bash
    docker-compose up --build
    ```
4.  Deixe este terminal aberto. Os servidores agora estão rodando e prontos para receber conexões na porta `8080`.

### Passo 2: Compilar e Executar o Cliente (Escolha seu Ambiente)

Você pode compilar e rodar o cliente tanto no WSL quanto nativamente no Windows.

---

#### Ambiente 1: WSL (Ubuntu)

1.  **Instalar dependências no Ubuntu:**
    ```bash
    sudo apt update
    sudo apt install build-essential libgtk-3-dev
    ```
2.  **Navegar até a pasta do cliente** em um novo terminal WSL/Ubuntu:
    ```bash
    cd cliente/
    ```
3.  **Compilar e Executar:**

    * **Cliente Gráfico (GTK+):**
        ```bash
        # Compilar
        g++ cliente_gtk.cpp -o cliente_gtk -std=c++17 $(pkg-config --cflags --libs gtk+-3.0) -pthread
        # Executar
        ./cliente_gtk
        ```

    * **Cliente via Linha de Comando (CLI):**
        ```bash
        # Compilar
        g++ cliente_cli.cpp -o cliente_cli -std=c++17 -pthread
        # Executar (usando um arquivo de exemplo)
        ./cliente_cli localhost 8080 ../teste.txt
        ```

---

#### Ambiente 2: Windows Nativo (com MSYS2)

Este método não requer WSL, mas exige a configuração do ambiente de desenvolvimento MSYS2.

1.  **Instalar o ambiente MSYS2:**
    * Baixe e instale o MSYS2 a partir do site oficial: [msys2.org](https://www.msys2.org/).
    * Abra o terminal **MSYS2 UCRT 64-bit**.
    * Atualize o sistema e instale o compilador C++ e ferramentas essenciais:
        ```bash
        pacman -Syu
        pacman -S mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-pkg-config
        ```
2.  **Instalar a biblioteca GTK3 no MSYS2:**
    * Ainda no terminal MSYS2, instale o GTK3:
        ```bash
        pacman -S mingw-w64-ucrt-x86_64-gtk3
        ```
3.  **Navegar até a pasta do cliente:**
    * Ainda no terminal MSYS2, navegue até a pasta do projeto (ex: `cd /c/Users/SeuUsuario/caminho/para/o/projeto/cliente`).

4.  **Compilar e Executar:**
    * As flags de compilação para Windows precisam de bibliotecas específicas para rede (`-lws2_32`) e threads (`-lwinpthread`).

    * **Cliente Gráfico (GTK+):**
        ```bash
        # Compilar
        g++ cliente_gtk.cpp -o cliente_gtk.exe -std=c++17 $(pkg-config --cflags --libs gtk+-3.0) -lws2_32 -lwinpthread
        # Executar
        ./cliente_gtk.exe
        ```

    * **Cliente via Linha de Comando (CLI):**
        ```bash
        # Compilar
        g++ cliente_cli.cpp -o cliente_cli.exe -std=c++17 -lws2_32 -lwinpthread
        # Executar
        ./cliente_cli.exe localhost 8080 ../teste.txt
        ```

### 📋 Exemplo de Uso (Cliente CLI)

1.  Crie um arquivo de exemplo chamado `teste.txt` na raiz do projeto com o seguinte conteúdo:
    ```
    Este é um sistema distribuído feito em C++ para a disciplina de Sistemas Operacionais.
    O ano é 2025 e a contagem de teste é 1, 2, 3, 4, 5.
    ```
2.  Com os servidores rodando e o cliente CLI compilado, execute-o. A saída no terminal será:
    ```json
    Enviando conteúdo do arquivo para localhost:8080...
    
    --- Resultado Recebido ---
    
    Status: 200
    Corpo da Resposta:
    {
        "letras": 104,
        "numeros": 9,
        "status": "SUCESSO"
    }
    ```