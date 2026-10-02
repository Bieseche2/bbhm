/* BBhM - Bieseche Bullshit Hen manager
 * Passo 1: esqueleto SDL2 (tela de teste estilo GNOME 2 + cursor + log) */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <SDL_ttf.h>

#define APP_DIR   "/dev_hdd0/game/BBHM00001/USRDIR"
#define FONT_PATH APP_DIR "/font.ttf"
#define LOG_PATH  "/dev_hdd0/tmp/bbhm.log"

static FILE *g_log = NULL;

static void bb_log(const char *fmt, ...)
{
    va_list ap;
    if (!g_log) g_log = fopen(LOG_PATH, "a");
    if (!g_log) return;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}

static void fill(SDL_Renderer *r, int x, int y, int w, int h,
                 Uint8 R, Uint8 G, Uint8 B)
{
    SDL_Rect rc;
    rc.x = x; rc.y = y; rc.w = w; rc.h = h;
    SDL_SetRenderDrawColor(r, R, G, B, 255);
    SDL_RenderFillRect(r, &rc);
}

static void text(SDL_Renderer *r, TTF_Font *f, const char *s,
                 int x, int y, Uint8 R, Uint8 G, Uint8 B)
{
    SDL_Color c;
    SDL_Surface *su;
    SDL_Texture *tx;
    SDL_Rect dst;

    if (!f || !s || !*s) return;
    c.r = R; c.g = G; c.b = B; c.a = 255;
    su = TTF_RenderUTF8_Blended(f, s, c);
    if (!su) return;
    tx = SDL_CreateTextureFromSurface(r, su);
    dst.x = x; dst.y = y; dst.w = su->w; dst.h = su->h;
    if (tx) {
        SDL_RenderCopy(r, tx, NULL, &dst);
        SDL_DestroyTexture(tx);
    }
    SDL_FreeSurface(su);
}

static void draw_cursor(SDL_Renderer *r, int x, int y)
{
    fill(r, x, y, 14, 14, 0, 0, 0);
    fill(r, x + 2, y + 2, 10, 10, 255, 255, 255);
}

int main(int argc, char *argv[])
{
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_RendererInfo info;
    SDL_Joystick *js = NULL;
    SDL_Event ev;
    TTF_Font *font = NULL;
    char last[128] = "nenhum input ainda";
    char buf[192];
    const char *jsname = "nenhum";
    int running = 1, w = 1280, h = 720, cx, cy;
    Uint32 t0, frames = 0, fps = 0;

    (void)argc; (void)argv;
    bb_log("=== BBhM iniciando ===");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
        bb_log("SDL_Init falhou: %s", SDL_GetError());
        return 1;
    }

    win = SDL_CreateWindow("BBhM", SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED, 1280, 720, 0);
    if (!win) {
        bb_log("SDL_CreateWindow falhou: %s", SDL_GetError());
        return 1;
    }

    ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren) {
        bb_log("renderer padrao falhou (%s), tentando software", SDL_GetError());
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!ren) {
        bb_log("sem renderer: %s", SDL_GetError());
        return 1;
    }
    SDL_GetRendererInfo(ren, &info);
    SDL_GetRendererOutputSize(ren, &w, &h);
    bb_log("renderer=%s saida=%dx%d", info.name, w, h);

    if (TTF_Init() != 0) {
        bb_log("TTF_Init falhou: %s", TTF_GetError());
    } else {
        font = TTF_OpenFont(FONT_PATH, 22);
        if (!font) bb_log("fonte nao abriu (%s): %s", FONT_PATH, TTF_GetError());
    }

    bb_log("joysticks=%d", SDL_NumJoysticks());
    if (SDL_NumJoysticks() > 0) {
        js = SDL_JoystickOpen(0);
        if (js) {
            jsname = SDL_JoystickName(js) ? SDL_JoystickName(js) : "sem nome";
            bb_log("joystick0=%s botoes=%d eixos=%d", jsname,
                   SDL_JoystickNumButtons(js), SDL_JoystickNumAxes(js));
        }
    }

    cx = w / 2; cy = h / 2;
    t0 = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_JOYBUTTONDOWN) {
                snprintf(last, sizeof last, "botao %d", ev.jbutton.button);
                bb_log("input: %s", last);
            } else if (ev.type == SDL_JOYAXISMOTION &&
                       (ev.jaxis.value > 12000 || ev.jaxis.value < -12000)) {
                snprintf(last, sizeof last, "eixo %d = %d",
                         ev.jaxis.axis, ev.jaxis.value);
                bb_log("input: %s", last);
            }
        }

        if (js) {
            int ax = SDL_JoystickGetAxis(js, 0);
            int ay = SDL_JoystickGetAxis(js, 1);
            if (ax > 4000 || ax < -4000) cx += ax / 3000;
            if (ay > 4000 || ay < -4000) cy += ay / 3000;
        }
        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;
        if (cx > w - 14) cx = w - 14;
        if (cy > h - 14) cy = h - 14;

        /* desktop */
        fill(ren, 0, 0, w, h, 58, 110, 165);
        /* painel superior e inferior */
        fill(ren, 0, 0, w, 32, 232, 232, 228);
        fill(ren, 0, 32, w, 1, 160, 160, 156);
        fill(ren, 0, h - 32, w, 32, 232, 232, 228);
        fill(ren, 0, h - 33, w, 1, 160, 160, 156);
        text(ren, font, "Applications   Places   System", 12, 3, 20, 20, 20);
        text(ren, font, "BBhM", 12, h - 29, 20, 20, 20);

        /* janela de teste */
        fill(ren, w / 2 - 320, 110, 640, 340, 90, 90, 90);
        fill(ren, w / 2 - 318, 112, 636, 30, 74, 111, 165);
        fill(ren, w / 2 - 318, 142, 636, 306, 242, 241, 240);
        text(ren, font, "BBhM - teste do esqueleto", w / 2 - 308, 114, 255, 255, 255);

        snprintf(buf, sizeof buf, "BBhM v0.0.1 (passo 1)");
        text(ren, font, buf, w / 2 - 300, 160, 20, 20, 20);
        snprintf(buf, sizeof buf, "Renderer: %s  %dx%d", info.name, w, h);
        text(ren, font, buf, w / 2 - 300, 195, 20, 20, 20);
        snprintf(buf, sizeof buf, "FPS: %u", (unsigned)fps);
        text(ren, font, buf, w / 2 - 300, 230, 20, 20, 20);
        snprintf(buf, sizeof buf, "Joystick: %s", jsname);
        text(ren, font, buf, w / 2 - 300, 265, 20, 20, 20);
        snprintf(buf, sizeof buf, "Ultimo input: %s", last);
        text(ren, font, buf, w / 2 - 300, 300, 20, 20, 20);
        text(ren, font, "Sair: botao PS > Sair do jogo", w / 2 - 300, 400, 90, 90, 90);

        draw_cursor(ren, cx, cy);
        SDL_RenderPresent(ren);

        frames++;
        if (SDL_GetTicks() - t0 >= 1000) {
            fps = frames; frames = 0; t0 = SDL_GetTicks();
            bb_log("fps=%u", (unsigned)fps);
        }
        SDL_Delay(5);
    }

    bb_log("=== BBhM saindo ===");
    if (font) TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    if (g_log) fclose(g_log);
    return 0;
}
