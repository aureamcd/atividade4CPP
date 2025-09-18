#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "httplib.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <host_mestre> <porta_mestre> <caminho_arquivo.txt>" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    std::string path_arquivo = argv[3];

    // Ler o conteúdo do arquivo
    std::ifstream file(path_arquivo);
    if (!file.is_open()) {
        std::cerr << "Erro: Não foi possível abrir o arquivo " << path_arquivo << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string conteudo_arquivo = buffer.str();

    // Enviar para o mestre
    httplib::Client cli(host, port);
    std::cout << "Enviando conteúdo do arquivo para " << host << ":" << port << "..." << std::endl;

    auto res = cli.Post("/processar", conteudo_arquivo, "text/plain");

    // Exibir o resultado
    if (res) {
        std::cout << "\n--- Resultado Recebido ---\n" << std::endl;
        std::cout << "Status: " << res->status << std::endl;
        std::cout << "Corpo da Resposta:\n" << res->body << std::endl;
    } else {
        auto err = res.error();
        std::cerr << "Erro na requisição HTTP: " << httplib::to_string(err) << std::endl;
    }

    return 0;
}