# Estágio de compilação
FROM gcc:11 AS builder

WORKDIR /app

# Copia os fontes
COPY . .

# Baixa as bibliotecas header-only
# Você também pode adicioná-las diretamente ao repositório
ADD https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h .
ADD https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp .

# Compila a aplicação
# O -o app define o nome do executável
RUN g++ -std=c++17 -pthread -static-libstdc++ main.cpp -o app

# Estágio final de execução
FROM debian:bullseye-slim

WORKDIR /app

# Copia apenas o executável compilado do estágio anterior
COPY --from=builder /app/app .

# Expõe a porta que o serviço usará (informativo)
# A porta real será mapeada no docker-compose.yml
# EXPOSE 8080 

# Comando para rodar a aplicação
CMD ["./app"]