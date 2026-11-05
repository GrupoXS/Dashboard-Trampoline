# Projeto 2 — Automotive Dashboard com Trampoline RTOS

## 1. Objetivo

Desenvolver um protótipo de painel automotivo (Instrument Cluster / ICM) que demonstre **controle determinístico baseado em tarefas de tempo real**, utilizando o **Trampoline RTOS** e sinais de veículo simulados.

A proposta é entregar uma versão demonstrável sem veículo, ECU, sensores, rede CAN física ou painel automotivo real. O Trampoline será executado em uma plataforma POSIX no Linux; os sinais do veículo serão gerados por software; e o painel gráfico será executado em Python.

> **Escopo recomendado:** protótipo majoritariamente simulado, com o kernel Trampoline real executando na plataforma POSIX. Não é uma validação de ECU física nem uma comprovação de hard real-time em hardware automotivo.

## 2. Estratégia geral: construir um MVP

Para reduzir o tempo de desenvolvimento, priorizar os itens que demonstram melhor a arquitetura e os conceitos do RTOS:

1. Executar uma aplicação mínima do Trampoline RTOS na plataforma POSIX.
2. Configurar tarefas, prioridades, alarmes, eventos e recursos por meio de OIL.
3. Gerar sinais de veículo por software.
4. Enviar telemetria do processo C para um dashboard Python via UDP local.
5. Exibir instrumentos e indicadores de advertência.
6. Registrar métricas de execução e resultados de testes.

Não é necessário implementar todo o conteúdo visual da referência. Um protótipo funcional com esses elementos já demonstra boa parte dos conceitos solicitados.

## 3. Arquitetura proposta

```text
+----------------------------------------+
|         Vehicle Signal Simulator       |
|                                        |
| Speed | RPM | Fuel | Coolant | Warnings|
+-------------------+--------------------+
                    |
                    v
+----------------------------------------+
|       Simulated Vehicle / CAN Data     |
|  Sinais fictícios com IDs e valores    |
|  inspirados em mensagens CAN           |
+-------------------+--------------------+
                    |
                    v
+----------------------------------------+
|             Trampoline RTOS            |
|                                        |
|  VehicleSimulationTask                 |
|  DiagnosticsTask                       |
|  WarningHandlerTask                    |
|  TelemetryTask                         |
|                                        |
|  OIL | Alarms | Events | Resources      |
|  Priorities | Scheduler                |
+-------------------+--------------------+
                    |
             UDP localhost
                    |
                    v
+----------------------------------------+
|           Python Dashboard             |
|                                        |
| Speedometer | RPM | Fuel | Temperature |
| Warning indicators | RTOS status       |
+----------------------------------------+
```

### Separação de responsabilidades

- **Trampoline RTOS / C:** executa a lógica periódica, diagnósticos, tratamento de eventos e preparação dos dados.
- **Simulador:** gera entradas artificiais que representam os sinais do veículo.
- **Comunicação UDP:** transporta os dados do processo C para a aplicação Python pela interface local (`127.0.0.1`).
- **Python / Streamlit / Plotly:** apresenta os dados graficamente. A interface não faz parte do caminho crítico de execução do RTOS.

É importante evitar que a atualização gráfica bloqueie tarefas críticas. A tarefa de telemetria deve ter prioridade baixa e utilizar envio não bloqueante ou uma estratégia simples de buffer, sempre que possível.

## 4. Ferramentas recomendadas

| Componente | Ferramenta | Finalidade |
|---|---|---|
| RTOS | Trampoline RTOS | Escalonamento e serviços de tempo real |
| Linguagem da aplicação RTOS | C | Simulador, diagnósticos, tarefas e comunicação |
| Configuração do RTOS | OIL | Declarar tarefas, prioridades, alarmes, eventos e recursos |
| Gerador de configuração | GOIL | Gerar estruturas e arquivos necessários à aplicação |
| Ambiente de execução | POSIX / Linux | Executar a versão virtual do Trampoline |
| Ambiente Windows | WSL2 + Ubuntu | Fornecer um ambiente Linux no computador Windows |
| Dashboard | Python + Streamlit | Interface gráfica no navegador |
| Gráficos | Plotly | Medidores e indicadores visuais |
| Comunicação | UDP local | Transportar telemetria do C para Python |
| Controle de versão | Git + GitHub | Versionamento e evidências do projeto |
| Testes e relatórios | Python / CSV / Markdown | Analisar métricas e registrar validação |

### Observação sobre Windows e POSIX

A documentação do Trampoline informa que o port POSIX é destinado a Linux e macOS e não roda nativamente no Windows. Como o computador de desenvolvimento é Windows, o caminho proposto é utilizar o Ubuntu dentro do WSL2. O WSL2 fornece um ambiente Linux, mas a execução do projeto deve ser validada no início, antes de desenvolver o restante.

## 5. Sinais do veículo simulados

Não serão necessários sensores físicos. Os sinais podem ser gerados por uma tarefa C, utilizando valores fixos, rampas, funções matemáticas ou cenários selecionáveis.

| Sinal | Unidade / faixa sugerida | Uso no dashboard |
|---|---:|---|
| Vehicle Speed | 0–220 km/h | Velocímetro |
| Engine RPM | 800–7.000 rpm | Tacômetro |
| Fuel Level | 0–100% | Indicador de combustível |
| Coolant Temperature | 70–120 °C | Indicador de temperatura |
| Warning Status | Bitmask ou flags | Luzes de advertência |

As faixas são valores didáticos para a demonstração; não representam limites de calibração de um veículo específico.

### Cenários de demonstração

| Cenário | Comportamento simulado | Resultado esperado |
|---|---|---|
| Normal Driving | Velocidade e RPM variam; combustível e temperatura permanecem em faixas normais | Painel sem advertências críticas |
| Low Fuel | Combustível cai abaixo de 10% | Indicador de combustível baixo |
| Engine Overheat | Temperatura ultrapassa 110 °C | Advertência de temperatura |
| Engine Overspeed | RPM ultrapassa 6.000 rpm | Advertência de sobre-rotação |
| Sensor Failure | Um sinal deixa de ser atualizado ou é marcado como inválido | Diagnóstico de falha de sinal |

Para a primeira versão, os cenários podem ser selecionados por uma variável de configuração ou comando simples. Uma interface para selecionar cenários pode ser adicionada depois.

## 6. Organização das tarefas do Trampoline

Uma configuração inicial possível é:

| Tarefa | Prioridade inicial | Ativação | Responsabilidade |
|---|---:|---|---|
| `VehicleSimulationTask` | 4 | Periódica, 100 ms | Atualizar os sinais simulados do veículo |
| `DiagnosticsTask` | 3 | Periódica, 100 ms | Verificar limites e detectar falhas |
| `WarningHandlerTask` | 5 | Por evento | Processar e atualizar o estado das advertências |
| `TelemetryTask` | 2 | Periódica, 100 ms | Enviar dados para o dashboard via UDP |

**Importante:** as prioridades e os períodos acima são valores iniciais para o protótipo, não o resultado de uma análise formal de escalonabilidade. No Trampoline/OSEK, a prioridade mais alta corresponde ao maior valor numérico. Os valores devem ser revisados conforme o comportamento e as medições da aplicação.

### Conceitos de RTOS a demonstrar

- **Periodic Tasks:** tarefas ativadas periodicamente por alarmes.
- **Events:** `DiagnosticsTask` pode sinalizar um evento quando detectar uma falha; `WarningHandlerTask` aguarda e trata esse evento.
- **Resources:** proteger estruturas de dados compartilhadas entre tarefas, quando necessário.
- **Alarms:** ativar tarefas em intervalos definidos.
- **Scheduling:** demonstrar o escalonamento por prioridade entre tarefas prontas para execução.

### Estrutura conceitual de dados

Uma estrutura compartilhada em C pode armazenar os valores do veículo:

```c
typedef struct {
    float speed_kmh;
    float engine_rpm;
    float fuel_percent;
    float coolant_temp_c;
    unsigned int warning_flags;
    unsigned int valid_signals;
} VehicleData;
```

O acesso concorrente a essa estrutura deve ser protegido de acordo com os serviços e regras do Trampoline. Uma tarefa não deve manter um recurso ocupado durante operações demoradas, como comunicação ou espera.

## 7. Comunicação entre Trampoline e dashboard

A alternativa mais simples para o protótipo é usar UDP local.

1. `TelemetryTask` obtém uma cópia consistente dos dados do veículo.
2. A aplicação C envia um pacote UDP para `127.0.0.1:5005`.
3. Um pequeno processo Python recebe os pacotes.
4. O dashboard Streamlit apresenta os valores recebidos.

Exemplo ilustrativo do payload em JSON:

```json
{
  "speed": 98,
  "rpm": 3200,
  "fuel": 65,
  "coolant": 92,
  "warning": 0
}
```

O JSON é apenas um exemplo de formato. A implementação pode utilizar uma estrutura binária ou texto delimitado para simplificar o envio em C. O processo Python deve validar os dados recebidos e lidar com ausência ou atraso de mensagens.

## 8. Dashboard / Instrument Cluster

O dashboard pode ser desenvolvido em Python com Streamlit e Plotly, sem criar uma interface gráfica dentro do RTOS.

### Elementos visuais mínimos

- Velocímetro analógico ou digital.
- Tacômetro (RPM).
- Indicador de combustível.
- Indicador de temperatura do líquido de arrefecimento.
- Luzes de advertência.
- Estado de comunicação / última atualização recebida.
- Estado simplificado das tarefas, quando essa informação for exportada pelo código C.

### Sugestão de layout

```text
+----------------------------------------------------------+
|                 AUTOMOTIVE DASHBOARD                     |
+----------------------+----------------------+------------+
|                      |                      |            |
|     SPEEDOMETER      |      TACHOMETER      |   FUEL     |
|       98 km/h        |       3200 rpm       |    65%     |
|                      |                      |            |
+----------------------+----------------------+------------+
|             COOLANT TEMPERATURE: 92 °C                   |
+----------------------------------------------------------+
|  ABS  |  ENGINE  |  LOW FUEL  |  OVERHEAT  |  SENSOR     |
+----------------------------------------------------------+
| RTOS: RUNNING       COMMUNICATION: OK       SCENARIO: NORM|
+----------------------------------------------------------+
```

O objetivo é ter uma interface visual próxima de um painel automotivo. Não é necessário reproduzir fielmente o desenho da referência.

## 9. Engineering Checks and Validation

Esta seção deve produzir evidências para os quatro itens de engenharia indicados no escopo original.

| Verificação | O que implementar | Evidência |
|---|---|---|
| Timing Analysis | Medir períodos e tempos de execução das tarefas | CSV e resumo de métricas |
| Task Deadlines | Comparar conclusão com deadline definido | Contador de misses e logs |
| Priority Assignment | Configurar prioridades e demonstrar preempção / ordem de execução | Log de ativações e execução |
| Fault Indicators | Injetar falhas e verificar as advertências | Capturas do dashboard e logs |

### Métricas recomendadas

- Task execution time (tempo de execução observado).
- Maximum execution time observado.
- Task period.
- Jitter observado.
- Number of deadline misses.
- Number of task activations.

### Exemplo de tabela para o relatório

Os números abaixo são **fictícios e servem somente para ilustrar o formato**. O relatório final deve conter valores efetivamente medidos durante a execução.

| Task | Period | Max execution time | Deadline misses |
|---|---:|---:|---:|
| `VehicleSimulationTask` | 100 ms | 2 ms | 0 |
| `DiagnosticsTask` | 100 ms | 1 ms | 0 |
| `TelemetryTask` | 100 ms | 3 ms | 0 |
| `WarningHandlerTask` | Event-driven | 1 ms | 0 |

### Limitações da validação temporal

A execução POSIX utiliza o sistema operacional hospedeiro. Portanto:

- Os tempos medidos dependem do computador e da carga do Linux/WSL2.
- As medições são úteis para avaliar o comportamento do protótipo, mas não comprovam garantias temporais de uma ECU real.
- Não declarar que o sistema possui hard real-time comprovado em hardware automotivo.
- Diferenciar claramente tempos observados na plataforma POSIX de requisitos temporais que precisariam ser validados na plataforma-alvo.

## 10. O que deixar fora do MVP

Para manter o escopo controlado, não implementar inicialmente:

- ECU física ou microcontrolador.
- Rede CAN física e transceiver CAN.
- Sensores reais.
- Integração com veículo real.
- AUTOSAR completo.
- ARXML completo.
- Simulação elétrica ou de hardware.
- Interface gráfica desenvolvida em C dentro do RTOS.

É possível representar mensagens com IDs e sinais CAN fictícios, desde que a documentação deixe explícito que são dados simulados e não tráfego CAN real.

## 11. Estrutura sugerida do repositório

```text
automotive-dashboard/
|
+-- rtos/
|   +-- vehicle_dashboard.oil
|   +-- vehicle_dashboard.c
|   +-- vehicle_simulator.c
|   +-- diagnostics.c
|   +-- warning_handler.c
|   +-- communication.c
|
+-- dashboard/
|   +-- app.py
|   +-- receiver.py
|   +-- requirements.txt
|
+-- tests/
|   +-- test_diagnostics.py
|   +-- test_timing.py
|
+-- reports/
|   +-- timing_report.csv
|   +-- validation_report.md
|
+-- README.md
```

A organização pode ser simplificada se o protótipo ficar pequeno. O importante é separar o código do RTOS, a interface gráfica, os testes e as evidências.

## 12. Plano de execução priorizado

### Etapa 1 — Validar o ambiente Trampoline

- Instalar WSL2 e Ubuntu.
- Clonar o repositório oficial do Trampoline.
- Preparar o GOIL e as ferramentas exigidas pela documentação.
- Compilar e executar o exemplo `examples/posix/one_task`.

**Critério de conclusão:** exemplo oficial compila e executa no ambiente escolhido.

### Etapa 2 — Criar a aplicação RTOS

- Criar o arquivo OIL da aplicação.
- Declarar as tarefas e prioridades.
- Configurar alarmes para as tarefas periódicas.
- Configurar o evento de advertência.
- Implementar os pontos de entrada das tarefas em C.

**Critério de conclusão:** aplicação inicia e executa as tarefas configuradas.

### Etapa 3 — Implementar os sinais simulados

- Gerar velocidade, RPM, combustível e temperatura.
- Implementar os cenários Normal Driving, Low Fuel, Engine Overheat, Engine Overspeed e Sensor Failure.
- Implementar a lógica de diagnóstico.

**Critério de conclusão:** valores mudam conforme o cenário e as falhas são detectadas.

### Etapa 4 — Implementar comunicação e dashboard

- Implementar envio UDP a partir da aplicação C.
- Criar o receptor Python.
- Criar os indicadores do dashboard com Streamlit e Plotly.
- Exibir o estado da comunicação.

**Critério de conclusão:** o dashboard exibe os valores enviados pela aplicação Trampoline.

### Etapa 5 — Fazer testes e gerar evidências

- Registrar tempo de execução e ativações das tarefas.
- Avaliar periodicidade e jitter observado.
- Contabilizar deadline misses.
- Testar os cenários de falha.
- Salvar logs, CSVs e capturas de tela.

**Critério de conclusão:** cada item de validação tem um teste e uma evidência correspondente.

## 13. Primeira validação do Trampoline no POSIX

A documentação oficial apresenta o exemplo `examples/posix/one_task` como primeiro passo para compilar e executar uma aplicação Trampoline na plataforma POSIX.

Fluxo geral documentado:

1. Preparar o GOIL e as ferramentas de compilação.
2. Compilar o suporte de runtime `viper`, utilizado pela plataforma POSIX.
3. Definir a variável de ambiente `VIPER_PATH` conforme o local da pasta `viper`.
4. Entrar em `examples/posix/one_task`.
5. Executar o GOIL com o alvo `posix/linux` e o arquivo OIL do exemplo.
6. Executar o script de build gerado (`make.py`).
7. Executar o binário gerado.

Exemplos de comandos mostrados na documentação (os caminhos devem ser ajustados à instalação local):

```bash
# Na raiz do repositório, compilar o viper
cd viper
make

# Exemplo: ajustar para o caminho real da pasta viper
export VIPER_PATH="$(pwd)"

# Voltar à raiz do Trampoline e entrar no exemplo
cd ../examples/posix/one_task

# Gerar os arquivos da aplicação
# Ajustar o caminho de --templates conforme a localização do repositório
goil --target=posix/linux --templates=../../../goil/templates/ one_task.oil

# Compilar a aplicação
./make.py

# Executar o binário gerado
./one_task_exe
```

> Os comandos são uma referência baseada no manual oficial. A instalação das dependências, a disponibilidade do comando `goil` no `PATH`, os caminhos de templates e as permissões dos scripts podem exigir ajustes no ambiente local. O primeiro objetivo é executar o exemplo oficial sem modificações.

## 14. Entregáveis finais

- Protótipo C executando com Trampoline RTOS na plataforma POSIX.
- Arquivo OIL com a configuração estática dos objetos do RTOS.
- Simulador de sinais do veículo.
- Dashboard gráfico integrado via UDP local.
- Cenários de falha demonstráveis.
- Código de testes e logs.
- `timing_report.csv` com medições observadas.
- `validation_report.md` descrevendo testes, resultados e limitações.
- Repositório GitHub com instruções para reproduzir a execução.

## 15. Definição de pronto (Definition of Done)

- [ ] O exemplo oficial do Trampoline POSIX compila e executa.
- [ ] A aplicação do projeto inicia no Trampoline.
- [ ] As tarefas periódicas estão configuradas via OIL.
- [ ] Há pelo menos um fluxo funcional baseado em evento.
- [ ] O acesso aos dados compartilhados está protegido quando necessário.
- [ ] Os sinais do veículo são gerados por software.
- [ ] O dashboard recebe e exibe os dados reais enviados pelo processo C.
- [ ] Os cenários de falha acionam os indicadores correspondentes.
- [ ] Há medições de execução, periodicidade, jitter e deadline misses.
- [ ] O relatório diferencia resultados POSIX de garantias de uma ECU física.
- [ ] O README explica como compilar, executar e demonstrar o projeto.

## 16. Referências oficiais

- [Repositório oficial Trampoline RTOS](https://github.com/TrampolineRTOS/trampoline)
- [Manual — Getting Started](https://github.com/TrampolineRTOS/trampoline/blob/master/documentation/manual/start.tex)
- [Manual — System generation and compilation](https://github.com/TrampolineRTOS/trampoline/blob/master/documentation/manual/compilation.tex)
- [README do Trampoline](https://github.com/TrampolineRTOS/trampoline/blob/master/README.md)

---

**Recomendação de execução:** comece pela Etapa 1 e só avance depois de compilar e executar `one_task`. Isso reduz o risco de desenvolver a aplicação e o dashboard antes de descobrir algum problema de ambiente ou toolchain.
