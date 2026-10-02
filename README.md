# BBhM (Bieseche Bullshit Hen manager)

Gerenciador de arquivos pra PS3 com HEN. UI em SDL2.

## Passo 1 (esqueleto)
- Tela de teste estilo GNOME 2 (paineis + janela), cursor pelo analogico
- Log em /dev_hdd0/tmp/bbhm.log (puxe por FTP)
- A tela mostra o ultimo botao/eixo apertado: isso revela o mapeamento do controle

## Build
O GitHub Actions compila (imagem hldtux/ps3dev-sdl2) e publica o .pkg como artefato.
