# Projeto 2 — Automotive Dashboard com Trampoline RTOS

## Rodar localmente (localhost)

Requisitos: Node.js 20 ou superior. Para executar também o simulador C, use Linux/macOS ou WSL2 com `make` e um compilador C (`cc`/`gcc`). Não é necessário instalar pacotes npm.

**Terminal 1 — servidor e front VOLT:**

```sh
cd cluster
API_KEY=local-demo-key npm start
```

Abra <http://localhost:3000>. Mantenha esse terminal aberto. No PowerShell, configure a chave com `$env:API_KEY = 'local-demo-key'` antes de executar `npm start`.

**Terminal 2 — enviar telemetria de teste (opcional):** na raiz do repositório, execute:

```sh
cd rtos
API_KEY=local-demo-key make run
```

O painel deve atualizar com os sinais simulados. Pare cada processo com `Ctrl+C`. Este comando executa o cliente POSIX de integração. A aplicação Trampoline POSIX está configurada em `rtos/vehicle_dashboard.oil`; para compilá-la, consulte [`rtos/README.md`](rtos/README.md) e forneça o caminho da instalação Trampoline.

**Testes:** rode `npm test` dentro de `cluster/` e `make test` dentro de `rtos/`.

## 1. Objetivo

Desenvolver um protótipo de painel automotivo (Instrument Cluster / ICM) que demonstre **controle determinístico baseado em tarefas de tempo real**, utilizando o **Trampoline RTOS** e sinais de veículo simulados.

A proposta é entregar uma versão demonstrável sem veículo, ECU, sensores, rede CAN física ou painel automotivo real. O Trampoline será executado em uma plataforma POSIX no Linux; os sinais do veículo serão gerados por software; e o painel gráfico será o front VOLT existente em `cluster/`.

> **Escopo recomendado:** protótipo majoritariamente simulado, com o kernel Trampoline real executando na plataforma POSIX. Não é uma validação de ECU física nem uma comprovação de hard real-time em hardware automotivo.

> **Ajuste de arquitetura:** o painel Node.js/HTML do diretório `cluster/` substitui a proposta anterior de dashboard Python. O produtor de telemetria em C enviará snapshots por HTTP para a API local do cluster; o servidor distribuirá as atualizações ao navegador via Server-Sent Events (SSE). Os sinais serão alinhados ao painel elétrico (EV).

> **Estado da implementação:** `rtos/` contém o primeiro cliente POSIX em C e um teste ponta a ponta contra a API/SSE. Ele valida o contrato de integração, mas ainda não é uma aplicação Trampoline: `goil` não está instalado neste ambiente e a configuração OIL/tarefas ainda será adicionada.

## 2. Estratégia geral: construir um MVP

Para reduzir o tempo de desenvolvimento, priorizar os itens que demonstram melhor a arquitetura e os conceitos do RTOS:

1. Executar uma aplicação mínima do Trampoline RTOS na plataforma POSIX.
2. Configurar tarefas, prioridades, alarmes, eventos e recursos por meio de OIL.
3. Gerar sinais de veículo por software.
4. Enviar telemetria do processo C para a API HTTP do cluster VOLT.
5. Exibir instrumentos e indicadores de advertência.
6. Registrar métricas de execução e resultados de testes.

Não é necessário implementar todo o conteúdo visual da referência. Um protótipo funcional com esses elementos já demonstra boa parte dos conceitos solicitados.

## 3. Arquitetura proposta

```text
+-----------------------------------+
| Trampoline RTOS / aplicação C     |
| Simulação | Diagnóstico | Alarmes  |
| Eventos | Warning | Telemetria     |
+------------------+----------------+
         |
  snapshot EV validado
         |
         v
+-----------------------------------+
| Cliente HTTP C (baixa prioridade) |
| POST JSON + Bearer para loopback  |
+------------------+----------------+
         |
         v
+-----------------------------------+
| Node.js: cluster/server.mjs       |
| Validação | estado | API HTTP     |
+------------------+----------------+
         |
        SSE
         |
         v
+-----------------------------------+
| Front VOLT: HTML/CSS/JavaScript   |
| Velocidade | bateria | autonomia  |
| Potência | marcha | alertas       |
+-----------------------------------+
```

### Separação de responsabilidades

- **Trampoline RTOS / C:** executa a simulação periódica, diagnósticos, tratamento de eventos e preparação dos snapshots de telemetria.
- **Cliente HTTP C:** envia JSON para `http://127.0.0.1:3000/api/telemetry`, autenticado com `API_KEY` configurada no ambiente.
- **Node.js:** valida os campos, mantém o estado em memória e publica atualizações em `/api/events`.
- **Front VOLT:** renderiza as atualizações SSE; a interface não faz parte do caminho crítico do RTOS.

Uma chamada HTTP síncrona pode bloquear. Não enviar da tarefa crítica nem manter um recurso compartilhado durante a chamada: manter a telemetria em baixa prioridade, definir timeout e, se necessário, transferir o snapshot para um worker POSIX fora do escalonador.

## 4. Ferramentas recomendadas

| Componente | Ferramenta | Finalidade |
|---|---|---|
| RTOS | Trampoline RTOS | Escalonamento e serviços de tempo real |
| Linguagem da aplicação RTOS | C | Simulador, diagnósticos, tarefas e comunicação |
| Configuração do RTOS | OIL | Declarar tarefas, prioridades, alarmes, eventos e recursos |
| Gerador de configuração | GOIL | Gerar estruturas e arquivos necessários à aplicação |
| Ambiente de execução | POSIX / Linux | Executar a versão virtual do Trampoline |
| Ambiente Windows | WSL2 + Ubuntu | Fornecer um ambiente Linux no computador Windows |
| Dashboard | Node.js + HTML/CSS/JavaScript | API e interface VOLT já existentes em `cluster/` |
| Atualização do painel | Server-Sent Events (SSE) | Atualizações em tempo real no navegador |
| Comunicação RTOS/UI | HTTP + JSON em loopback | POST autenticado para a API do cluster |
| Controle de versão | Git + GitHub | Versionamento e evidências do projeto |
| Testes e relatórios | C / Node.js / CSV / Markdown | Verificar integração e registrar medições |

### Observação sobre Windows e POSIX

A documentação do Trampoline informa que o port POSIX é destinado a Linux e macOS e não roda nativamente no Windows. Como o computador de desenvolvimento é Windows, o caminho proposto é utilizar o Ubuntu dentro do WSL2. O WSL2 fornece um ambiente Linux, mas a execução do projeto deve ser validada no início, antes de desenvolver o restante.

## 5. Sinais do veículo simulados

Não serão necessários sensores físicos. Os sinais podem ser gerados por uma tarefa C, utilizando valores fixos, rampas, funções matemáticas ou cenários selecionáveis.

| Sinal | Unidade / faixa sugerida | Uso no dashboard |
|---|---:|---|
| Vehicle Speed | 0–260 km/h | Velocímetro |
| Motor Power | -100–300 kW | Escala de regeneração/potência |
| Battery | 0–100% | Indicador de bateria |
| Range | 0–1000 km | Autonomia estimada |
| Temperature | -40–100 °C | Temperatura exterior |
| Gear | P, R, N, D, B | Marcha selecionada |
| Odometer | 0–9999999 km | Hodômetro |
| Indicators | booleanos | Setas, faróis, cinto e alerta |

As faixas são valores didáticos para a demonstração; não representam limites de calibração de um veículo específico.

### Cenários de demonstração

| Cenário | Comportamento simulado | Resultado esperado |
|---|---|---|
| Normal Driving | Velocidade e potência variam; bateria e autonomia diminuem gradualmente | Painel acompanha o estado via SSE |
| Regeneration | Potência negativa dentro da faixa permitida | Escala de potência indica regeneração |
| Low Battery | Bateria cai para uma faixa baixa definida pelo simulador | Indicador de bateria atualizado |
| Vehicle Warning | Flag de alerta ou cinto é ativada | Indicador correspondente acende |
| Sensor Failure | Um sinal é inválido ou deixa de atualizar | Diagnóstico registra falha e mantém/assinala o último estado válido |

Para a primeira versão, os cenários podem ser selecionados por uma variável de configuração ou comando simples. Uma interface para selecionar cenários pode ser adicionada depois.

## 6. Organização das tarefas do Trampoline

Uma configuração inicial possível é:

| Tarefa | Prioridade inicial | Ativação | Responsabilidade |
|---|---:|---|---|
| `VehicleSimulationTask` | 4 | Periódica, 100 ms | Atualizar os sinais simulados do veículo |
| `DiagnosticsTask` | 3 | Periódica, 100 ms | Verificar limites e detectar falhas |
| `WarningHandlerTask` | 5 | Por evento | Processar e atualizar o estado das advertências |
| `TelemetryTask` | 2 | Periódica, 100 ms | Enfileirar snapshot para envio HTTP fora do caminho crítico |

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
  float power_kw;
  float battery_percent;
  float range_km;
  char gear;
  float temperature_c;
  unsigned int odometer_km;
  unsigned int indicators;
} VehicleData;
```

O acesso concorrente a essa estrutura deve ser protegido de acordo com os serviços e regras do Trampoline. Uma tarefa não deve manter um recurso ocupado durante operações demoradas, como comunicação ou espera.

## 7. Comunicação entre Trampoline e cluster VOLT

1. `TelemetryTask` copia um snapshot consistente dos dados do veículo.
2. O cliente C envia um `POST` JSON autenticado para `http://127.0.0.1:3000/api/telemetry`.
3. O servidor Node valida os campos e faixas, atualiza o estado e incrementa `sequence`.
4. O servidor transmite o novo snapshot pelo endpoint SSE `/api/events`.
5. O front VOLT existente renderiza os dados no navegador.

O payload deve usar exatamente o contrato da API: `speed`, `power`, `battery`, `range`, `gear`, `temperature`, `odometer`, `left`, `right`, `lights`, `seatbelt` e `warning`. Os limites e tipos estão documentados em [`cluster/README.md`](cluster/README.md). O servidor gera `updatedAt` e `sequence`; o produtor não deve enviá-los.

O cliente usa `Authorization: Bearer API_KEY`. Configure a chave no ambiente do processo produtor e nunca no JavaScript do navegador ou em arquivo versionado. Para desenvolvimento, mantenha o servidor e o produtor no mesmo computador, usando loopback.

O envio HTTP pode bloquear por até o timeout configurado. Não fazer a chamada mantendo um recurso compartilhado nem em tarefa crítica; usar tarefa de baixa prioridade ou worker POSIX separado. Uma falha de envio não deve parar a simulação/diagnóstico.

## 8. Dashboard / Instrument Cluster

O dashboard da demonstração é o front VOLT existente em `cluster/public/`; não criar uma segunda interface em Python para este fluxo.

### Elementos visuais mínimos

- Velocidade e marcha.
- Bateria, autonomia e potência/regeneração.
- Temperatura exterior e odômetro.
- Indicadores de seta, faróis, cinto e alerta.
- Estado de conexão e idade da última telemetria.

### Sugestão de layout

```text
+----------------------------------------------------------+
|                 AUTOMOTIVE DASHBOARD                     |
+----------------------+----------------------+------------+
|  SPEED  | BATTERY | RANGE | POWER / REGEN | GEAR       |
|  98 km/h|  65%    | 240km | 32 kW         | D          |
+----------------------+----------------------+------------+
| TEMPERATURE | ODOMETER | LIGHTS | SEATBELT | WARNING    |
+----------------------------------------------------------+
| RTOS: RUNNING       CONNECTION: OK       UPDATED: <1 s   |
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
|   +-- vehicle_simulator.c
|   +-- diagnostics.c
|   +-- warning_handler.c
|   +-- cluster_client.c
|   +-- Makefile
|
+-- cluster/
|   +-- server.mjs
|   +-- public/
|   +-- test.mjs
|
+-- tests/
|   +-- test_diagnostics.c
|   +-- test_cluster_integration.mjs
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

- Gerar velocidade, potência, bateria, autonomia, marcha e temperatura compatíveis com o cluster VOLT.
- Implementar cenários de condução normal, regeneração, bateria baixa e falha de sinal.
- Implementar a lógica de diagnóstico.

**Critério de conclusão:** valores mudam conforme o cenário e as falhas são detectadas.

### Etapa 4 — Implementar comunicação e dashboard

- Implementar o cliente HTTP C para `POST /api/telemetry`, com Bearer key, validação local e timeout.
- Reutilizar o servidor Node e o front VOLT em `cluster/`; o servidor já distribui estado via SSE.
- Manter chamadas de rede fora de tarefas críticas e de seções protegidas por recurso.
- Exibir e validar estado de conexão/idade da telemetria no painel.

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
- Cliente C integrado à API HTTP do cluster VOLT, com atualização do browser via SSE.
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
- [ ] A transmissão HTTP tem timeout e não bloqueia tarefas críticas nem mantém recurso compartilhado.
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

## Aplicação web de demonstração (cluster)

O repositório também contém um painel web de demonstração de telemetria veicular em `cluster/`. Ele requer Node.js 20 ou superior e não precisa instalar dependências.

Na raiz do repositório, execute:

```sh
cd cluster
npm start
```

Abra <http://localhost:3000>. Para rodar os testes, use `npm test` na pasta `cluster/`. A API e as opções de configuração estão documentadas em [cluster/README.md](cluster/README.md).

> O painel web é apenas uma simulação visual; não é um instrumento de condução real.
