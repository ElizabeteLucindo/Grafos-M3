#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <algorithm>
using namespace std;

struct Atividade {
    string nome;
    int duracao;
    vector<string> precedentes;
    int ES = 0, EF = 0; //inicio/término mais cedo
    int LS = 0, LF = 0, folga = 0; //inicio/termino mais tarde
};

// Função para gerar o HTML com o grafo
void gerarHTML(const vector<Atividade>& atividades,
               const map<string, int>& indice,
               const string& nomeArquivo = "pertCPM.html") {
    ofstream arq(nomeArquivo);
    if (!arq.is_open()) {
        cout << "Erro ao criar o arquivo HTML.\n";
        return;
    }

    arq << R"(
    <!DOCTYPE html>
    <html>
    <head>
    <meta charset="utf-8">
    <title>Grafo PERT/CPM</title>
    <script type="text/javascript" src="https://unpkg.com/vis-network/standalone/umd/vis-network.min.js"></script>
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

    for (size_t i = 0; i < atividades.size(); i++) {
        string color;
        if (atividades[i].folga == 0){
            color = "red";
        }else { color = "lightblue";}

        arq << "  { id: " << i << ", label: '"
            << atividades[i].nome
            << "\\nDur: " << atividades[i].duracao
            << " | Folga: " << atividades[i].folga
            << "', color: '" << color << "' }";
        if (i < atividades.size() - 1) arq << ",";
        arq << "\n";
    }

    arq << R"(]);

    var edges = new vis.DataSet([
    )";

    bool primeiro = true;
    for (size_t i = 0; i < atividades.size(); i++) {
        for (size_t j = 0; j < atividades[i].precedentes.size(); j++) {
            string p = atividades[i].precedentes[j];
            if (indice.count(p)) {
                if (!primeiro) arq << ",\n";
                primeiro = false;
                arq << "  { from: " << indice.at(p)
                    << ", to: " << i
                    << ", arrows: 'to', color: 'black' }";
            }
        }
    }

    arq << R"(]);

    var container = document.getElementById('mynetwork');
    var data = { nodes: nodes, edges: edges };
    var options = {
        layout: { hierarchical: { direction: 'LR', sortMethod: 'directed' } },
        edges: { smooth: false },
        physics: false
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


void calcularPERT(vector<Atividade>& atividades, map<string, int>& indice) {
    
    // Caminho de ida
    for (size_t i = 0; i < atividades.size(); i++) {
        int maiorEF = 0;
        for (size_t j = 0; j < atividades[i].precedentes.size(); j++) {
            string p = atividades[i].precedentes[j];
            if (indice.count(p)) {
                int idx= indice[p];
                maiorEF = max(maiorEF, atividades[idx].EF);
            }
        }
        atividades[i].ES = maiorEF;
        atividades[i].EF = atividades[i].ES + atividades[i].duracao;
    }

    // Tempo total
    int tempoFinal = 0;
    for (size_t i = 0; i < atividades.size(); i++)
        tempoFinal = max(tempoFinal, atividades[i].EF);

    // Caminho de volta 
    for (int i = (int)atividades.size() - 1; i >= 0; i--) {
        int menorLS = tempoFinal;
        for (size_t j = 0; j < atividades.size(); j++) {
            for (size_t k = 0; k < atividades[j].precedentes.size(); k++) {
            string p = atividades[j].precedentes[k];
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

// Mostra tabela no console
void mostrarTabela(const vector<Atividade>& atividades) {
    cout << "\n=== TABELA PERT/CPM ===\n";
    cout << "Atividade\tDuracao\t\tES\tEF\tLS\tLF\tFolga\n";
    cout << "------------------------------------------------------------------------\n";
    for (size_t i = 0; i < atividades.size(); i++) {
        auto& a = atividades[i];
        cout << a.nome << "\t\t" << a.duracao << "\t\t"
             << a.ES << "\t" << a.EF << "\t"
             << a.LS << "\t" << a.LF << "\t"
             << a.folga << endl;
    }

    cout << "\nCaminho crítico: ";
    bool primeiro = true;
    for (size_t j = 0; j < atividades.size(); j++) {
        auto& a = atividades[j];
        if (a.folga == 0) {
            if (!primeiro) cout << " → ";
            cout << a.nome;
            primeiro = false;
        }
    }
    cout << endl;
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

        cout << "Duração: ";
        cin >> atividades[i].duracao;

        cout << "Precedentes (separados por vírgula, '-' se não houver): ";
        string prec;
        cin >> prec;

        if (prec != "-") {
            stringstream ss(prec);
            string token;
            while (getline(ss, token, ',')) {
                atividades[i].precedentes.push_back(token);
            }
        }
    }

    calcularPERT(atividades, indice);
    mostrarTabela(atividades);

    char op;
    cout << "\nDeseja gerar o grafo visual? (s/n): ";
    cin >> op;
    if (op == 's' || op == 'S')
        gerarHTML(atividades, indice);

    cout << "\nPrograma finalizado.\n";
    return 0;
}
