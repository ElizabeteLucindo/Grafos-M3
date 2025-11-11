#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <algorithm>
using namespace std;

// Estrutura para armazenar dados de cada atividade
struct Atividade {
    string nome;
    int duracao;
    vector<string> precedentes;
    int ES = 0, EF = 0;
    int LS = 0, LF = 0, folga = 0;
};

// Gera o grafo em HTML
void gerarHTML(const vector<Atividade>& atividades,
               const map<string, int>& indice,
               const string& nomeArquivo = "pertCPM.html") {
    ofstream arq(nomeArquivo);
    if (!arq.is_open()) {
        cout << "Erro ao criar o arquivo HTML.\n";
        return;
    }

    // Cabeçalho do HTML
    arq << R"(
    <!DOCTYPE html>
    <html>
    <head>
    <meta charset="utf-8">
    <title>Grafo PERT/CPM</title>
    <script src="https://unpkg.com/vis-network/standalone/umd/vis-network.min.js"></script>
    <style>
        #mynetwork { width: 100%; height: 600px; border: 1px solid gray; }
    </style>
    </head>
    <body>
    <h2>Grafo PERT/CPM</h2>
    <div id="mynetwork"></div>
    <script>
    var nodes = new vis.DataSet([
    )";

    // Marca em vermelho as atividades do caminho crítico
    for (size_t i = 0; i < atividades.size(); i++) {
        string color = (atividades[i].folga == 0) ? "red" : "lightblue";

        arq << "  { id: " << i
            << ", label: '" << atividades[i].nome
            << "', color: '" << color
            << "', shape: 'circle' }";

        if (i < atividades.size() - 1) arq << ",";
        arq << "\n";
    }

    // Adiciona nós de informação
    for (size_t i = 0; i < atividades.size(); i++) {
        arq << ",  { id: 'info" << i << "', label: '"
            << "Dur: " << atividades[i].duracao
            << " | Folga: " << atividades[i].folga << "\\n"
            << "ES: " << atividades[i].ES
            << " | EF: " << atividades[i].EF << "\\n"
            << "LS: " << atividades[i].LS
            << " | LF: " << atividades[i].LF
            << "', shape: 'box', color: '#fff8dc' }";
        arq << "\n";
    }

    arq << R"(]);

    var edges = new vis.DataSet([
    )";

    bool primeiro = true;
    // Cria conexões entre atividades
    for (size_t i = 0; i < atividades.size(); i++) {
        for (auto& p : atividades[i].precedentes) {
            if (indice.count(p)) {
                if (!primeiro) arq << ",\n";
                primeiro = false;
                arq << "  { from: " << indice.at(p)
                    << ", to: " << i
                    << ", arrows: 'to', color: 'black' }";
            }
        }
    }

    // Conecta cada atividade ao seu nó de informação
    for (size_t i = 0; i < atividades.size(); i++) {
        if (!primeiro) arq << ",\n";
        primeiro = false;
        arq << "  { from: " << i
            << ", to: 'info" << i
            << "', dashes: true, color: 'gray', arrows: '' }";
    }

    // Configuração visual do grafo
    arq << R"(]);

    var container = document.getElementById('mynetwork');
    var data = { nodes: nodes, edges: edges };
    var options = {
      layout: {
        hierarchical: {
          direction: 'LR',
          levelSeparation: 150,
          nodeSpacing: 150,
          treeSpacing: 200,
          sortMethod: 'directed'
        }
      },
      nodes: {
        margin: 8,
        font: { size: 12, multi: 'html', align: 'center' }
      },
      edges: {
        smooth: false,
        arrows: { to: { enabled: true, scaleFactor: 0.8 } }
      },
      physics: false,
      interaction: { dragNodes: true } // permite arrastar nós manualmente
    };

    var network = new vis.Network(container, data, options);
    </script>
    </body>
    </html>
    )";

    arq.close();
    cout << "\nArquivo '" << nomeArquivo << "' gerado! Abra-o no navegador.\n";

#ifdef _WIN32
    system(("start " + nomeArquivo).c_str());
#else
    system(("xdg-open " + nomeArquivo).c_str());
#endif
}

// Calcula tempos e folgas
void calcularPERT(vector<Atividade>& atividades, map<string, int>& indice) {
    
    // Cálculo do caminho direto
    for (size_t i = 0; i < atividades.size(); i++) {
        int maiorEF = 0;
        for (auto& p : atividades[i].precedentes) {
            if (indice.count(p)) {
                int idx = indice[p];
                maiorEF = max(maiorEF, atividades[idx].EF);
            }
        }
        atividades[i].ES = maiorEF;
        atividades[i].EF = atividades[i].ES + atividades[i].duracao;
    }

    // Determina o tempo total do projeto
    int tempoFinal = 0;
    for (auto& a : atividades)
        tempoFinal = max(tempoFinal, a.EF);

    // Cálculo do caminho de retorno
    for (int i = (int)atividades.size() - 1; i >= 0; i--) {
        int menorLS = tempoFinal;
        for (size_t j = 0; j < atividades.size(); j++) {
            for (auto& p : atividades[j].precedentes) {
                if (p == atividades[i].nome) {
                    menorLS = min(menorLS, atividades[j].LS);
                }
            }
        }
        atividades[i].LF = menorLS;
        atividades[i].LS = atividades[i].LF - atividades[i].duracao;
        atividades[i].folga = atividades[i].LS - atividades[i].ES;
    }
}

// Exibe tabela com resultados no console
void mostrarTabela(const vector<Atividade>& atividades) {
    cout << "\n=== TABELA PERT/CPM ===\n";
    cout << "Atividade\tDuracao\t\tES\tEF\tLS\tLF\tFolga\n";
    cout << "------------------------------------------------------------------------\n";
    for (auto& a : atividades) {
        cout << a.nome << "\t\t" << a.duracao << "\t\t"
             << a.ES << "\t" << a.EF << "\t"
             << a.LS << "\t" << a.LF << "\t"
             << a.folga << endl;
    }

    // Mostra o caminho crítico
    cout << "\nCaminho critico: ";
    bool primeiro = true;
    for (auto& a : atividades) {
        if (a.folga == 0) {
            if (!primeiro) cout << " - ";
            cout << a.nome;
            primeiro = false;
        }
    }
    cout << endl;
}

void exibirMenu() {
    cout << "\nMENU:\n";
    cout << "1 - Mostrar matriz\n";
    cout << "2 - Mostrar grafo\n";
    cout << "0 - Sair\n";
    cout << "Escolha uma opção: ";
}

int main() {
    int qtd;
    cout << "Quantas atividades? ";
    cin >> qtd;

    vector<Atividade> atividades(qtd);
    map<string, int> indice;

    for (int i = 0; i < qtd; i++) {
        cout << "\nAtividade " << i+1 << ":\n";
        cout << "Nome: ";
        cin >> atividades[i].nome;
        indice[atividades[i].nome] = i;

        cout << "Duracao: ";
        cin >> atividades[i].duracao;

        cout << "Precedentes (separados por virgula, '-' se nao houver): ";

        // Validação dos precedentes
        string entrada;
        while (true) {
            cin >> entrada;

            // Verifica caracteres válidos
            bool valido = true;
            for (char c : entrada) {
                if (!isalnum(c) && c != ',' && c != '-') {
                    valido = false;
                    break;
                }
            }

            if (!valido) {
                cout << "Entrada invalida! Use apenas numeros, virgulas ou '-'.\n";
                cout << "Digite novamente os precedentes: ";
                continue;
            }

            if (entrada == "-") {
                atividades[i].precedentes.clear();
                break;
            }

            // Separa e valida precedentes existentes
            stringstream ss(entrada);
            string token;
            valido = true;
            vector<string> lista;

            while (getline(ss, token, ',')) {
                token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());

                if (token.empty()) {
                    cout << "Entrada invalida: virgula extra.\n";
                    valido = false;
                    break;
                }

                if (!indice.count(token)) {
                    cout << "Precedente '" << token << "' nao existe. Digite somente atividades anteriores.\n";
                    valido = false;
                    break;
                }

                lista.push_back(token);
            }

            if (valido) {
                atividades[i].precedentes = lista;
                break;  
            }
        }
    }

    calcularPERT(atividades, indice);

    int opcao;
    do {
        exibirMenu();
        cin >> opcao;

        switch(opcao) {
            case 1:
                mostrarTabela(atividades);
                break;
            case 2:
                gerarHTML(atividades, indice);
                break;
            case 0:
                cout << "Saindo...\n";
                break;
            default:
                cout << "Opcao invalida!\n";
                break;
        }

    } while(opcao != 0);

    return 0;
}