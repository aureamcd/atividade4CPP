#include <iostream>
#include <thread>
#include <future>
#include "httplib.h"
#include "json.hpp" // Inclua o header da biblioteca JSON

// Para conveniência
using json = nlohmann::json;

// Função para comunicar com um escravo. Retorna o resultado ou -1 em caso de erro.
int consultar_escravo(const std::string& host, int port, const std::string& endpoint, const std::string& data) {
    httplib::Client cli(host, port);
    
    // 1. Verificar a saúde do escravo
    auto health_res = cli.Get("/health");
    if (!health_res || health_res->status != 200) {
        std::cerr << "Erro: Escravo em " << host << ":" << port << " não está disponível." << std::endl;
        return -1; // Indica erro
    }

    // 2. Enviar os dados para processamento
    auto res = cli.Post(endpoint.c_str(), data, "text/plain");
    if (res && res->status == 200) {
        try {
            return std::stoi(res->body);
        } catch (const std::invalid_argument& e) {
            std::cerr << "Erro ao converter a resposta do escravo: " << res->body << std::endl;
            return -1;
        }
    } else {
        std::cerr << "Erro na comunicação com o escravo " << host << ":" << port << endpoint << std::endl;
        return -1;
    }
}

int main() {
    httplib::Server svr;

    // Endpoint principal que recebe o arquivo do cliente
    svr.Post("/processar", [&](const httplib::Request &req, httplib::Response &res) {
        std::string conteudo_arquivo = req.body;
        std::cout << "Recebida requisição para processar " << conteudo_arquivo.size() << " bytes." << std::endl;

        // Disparar as duas tarefas em paralelo usando std::async
        // O docker-compose permite usar o nome do serviço como hostname
        auto future_letras = std::async(std::launch::async, consultar_escravo, "escravo-letras", 8081, "/letras", conteudo_arquivo);
        auto future_numeros = std::async(std::launch::async, consultar_escravo, "escravo-numeros", 8082, "/numeros", conteudo_arquivo);

        // Aguardar e obter os resultados
        int contagem_letras = future_letras.get();
        int contagem_numeros = future_numeros.get();

        // Montar a resposta JSON
        json resposta_json;
        resposta_json["letras"] = contagem_letras;
        resposta_json["numeros"] = contagem_numeros;

        if (contagem_letras == -1 || contagem_numeros == -1) {
             resposta_json["status"] = "ERRO";
             resposta_json["descricao"] = "Um ou mais escravos falharam em processar a requisicao.";
             res.status = 500; // Internal Server Error
        } else {
             resposta_json["status"] = "SUCESSO";
        }
       
        res.set_content(resposta_json.dump(4), "application/json"); // .dump(4) para formatar o JSON
    });

    std::cout << "Servidor Mestre rodando na porta 8080..." << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}