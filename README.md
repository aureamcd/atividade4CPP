# Sistema Distribuído com C++, Threads e Docker

Este projeto implementa um sistema distribuído com arquitetura Mestre-Escravo em C++ puro para contar letras e números em um arquivo de texto.

## Arquitetura

- **Cliente**: Aplicação de linha de comando que envia um arquivo `.txt` para o Mestre.
- **Mestre**: Servidor que recebe o arquivo, consulta dois escravos em paralelo (usando `std::async`) e consolida os resultados.
- **Escravo 1 (letras)**: Serviço que conta a quantidade de letras no texto recebido.
- **Escravo 2 (numeros)**: Serviço que conta a quantidade de dígitos numéricos no texto recebido.

## Tecnologias

- **Linguagem**: C++17
- **Comunicação**: HTTP REST (via `cpp-httplib`)
- **JSON**: `nlohmann/json`
- **Concorrência**: `std::async`
- **Contêineres**: Docker e Docker Compose

## Como Compilar e Executar

### Pré-requisitos
- Docker e Docker Compose instalados.
- Um compilador C++ (g++) para compilar o cliente localmente.

### 1. Iniciar os Servidores (Mestre e Escravos)

Na raiz do projeto, execute o comando para construir as imagens e iniciar os contêineres:

```bash
docker-compose up --build
```

Os três servidores (Mestre na porta 8080, Escravo Letras na 8081, Escravo Números na 8082) estarão rodando em segundo plano.

### 2. Compilar e Executar o Cliente

Navegue até o diretório do cliente e compile o `main.cpp`:

```bash
cd cliente
g++ -std=c++17 main.cpp -o cliente
```

### 3. Exemplo de Uso

Crie um arquivo de texto, por exemplo, `teste.txt`:

```
Este eh um teste com 123 numeros e algumas letras ABC. Total de 456.
```

Execute o cliente, passando o endereço do mestre (que está rodando no seu localhost), a porta e o nome do arquivo:

```bash
./cliente localhost 8080 teste.txt
```

#### Saída Esperada

```
Enviando conteúdo do arquivo para localhost:8080...

--- Resultado Recebido ---

Status: 200
Corpo da Resposta:
{
    "letras": 43,
    "numeros": 6,
    "status": "SUCESSO"
}
```