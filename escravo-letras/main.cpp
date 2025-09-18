#include <iostream>
// Em main.cpp (mestre, escravos, cliente)
#include "httplib.h"


int main() {
    httplib::Server svr;

    // Endpoint de verificação de saúde
    svr.Get("/health", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("OK", "text/plain");
    });

    // Endpoint que recebe o texto e conta as letras
    svr.Post("/letras", [](const httplib::Request &req, httplib::Response &res) {
        std::string texto = req.body;
        int count = 0;
        for (char c : texto) {
            if (std::isalpha(c)) {
                count++;
            }
        }
        std::cout << "Requisição recebida em /letras. Contagem: " << count << std::endl;
        res.set_content(std::to_string(count), "text/plain");
    });

    std::cout << "Servidor Escravo de Letras rodando na porta 8081..." << std::endl;
    svr.listen("0.0.0.0", 8081); // Escuta em todas as interfaces de rede

    return 0;
}