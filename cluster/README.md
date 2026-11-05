# VOLT — cluster veicular elétrico

Painel responsivo minimalista com API HTTP e atualização por Server-Sent Events. Requer Node.js 20 ou superior; não exige instalação de dependências.

## Executar no PowerShell

```powershell
$env:API_KEY = 'substitua-por-uma-chave-longa-e-aleatoria'
node server.mjs
```

Abra http://localhost:3000. Sem API_KEY, uma chave temporária é gerada e exibida no terminal. Estado em memória: reiniciar restaura valores iniciais. A demonstração é exclusivamente visual; encerre-a para acompanhar a API.

## Enviar dados

```powershell
$headers = @{ Authorization = "Bearer $env:API_KEY" }
$body = @{ speed = 88; gear = 'D'; battery = 76; range = 304; power = 42; lights = $true } | ConvertTo-Json
Invoke-RestMethod -Uri http://localhost:3000/api/telemetry -Method Post -Headers $headers -ContentType 'application/json' -Body $body
```

Campos omitidos são preservados. POST /api/telemetry exige chave Bearer. GET /api/state e GET /api/events são leitura sem autenticação, assim como o painel; não envie informações privadas ao publicar esse projeto sem adicionar controle de acesso de leitura.

| Campo | Valores |
|---|---|
| speed | 0–260 km/h |
| power | -100–300 kW; negativo indica regeneração |
| battery | 0–100 % |
| range | 0–1000 km |
| gear | P, R, N, D, B |
| temperature | -40–100 °C exterior |
| odometer | 0–9999999 km |
| left, right, lights | booleanos; setas e faróis |
| seatbelt, warning | booleanos; true acende aviso |

JSON inválido retorna 400; chave inválida 401; tipo de conteúdo inválido 415; corpo acima de 16 KB retorna 413. Limite o envio a uma frequência adequada (por exemplo, 10 Hz). Múltiplas telas acompanham o mesmo estado, com reconexão automática e indicação da idade da última leitura.

## Receber pela internet

Hospede o processo Node em um servidor com HTTPS ou configure um túnel HTTPS para a porta 3000. Use o endereço HTTPS resultante no POST. Para escutar interfaces externas, configure HOST=0.0.0.0 e PORT conforme o provedor. Configure API_KEY como segredo do ambiente. Em proxy reverso, desative buffering para /api/events e permita conexões persistentes. Configure rate limiting no proxy antes de exposição pública. O servidor usa uma instância e memória local; múltiplas réplicas exigem armazenamento e pub/sub compartilhados.

Este projeto ainda não foi publicado na internet. É um simulador visual, não um instrumento de condução real. Fontes Google são opcionais: sem internet, usa Arial.

## Verificar

`node --test test.mjs` verifica página, autenticação, validação, atualização parcial e transmissão SSE.
