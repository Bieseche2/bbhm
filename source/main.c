/* BBhM - Bieseche Bullshit Hen manager
 * v0.0.2: input pelo pad nativo do PSL1GHT (io/pad.h), textos em cache,
 *         saida limpa com Start+Select, log mais detalhado */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <io/pad.h>

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

/* texto em cache: so re-renderiza quando a string muda */
typedef struct {
    char s[200];
    SDL_Texture *tx;
    int w, h;
} label_t;

static void label_set(SDL_Renderer *r, TTF_Font *f, label_t *l, const char *s,
                      Uint8 R, Uint8 G, Uint8 B)
{
    SDL_Color c;
    SDL_Surface *su;

    if (!f || !s) return;
    if (l->tx && strcmp(l->s, s) == 0) return;
    if (l->tx) { SDL_DestroyTexture(l->tx); l->tx = NULL; }
    strncpy(l->s, s, sizeof l->s - 1);
    l->s[sizeof l->s - 1] = 0;
    c.r = R; c.g = G; c.b = B; c.a = 255;
    su = TTF_RenderUTF8_Blended(f, l->s, c);
    if (!su) return;
    l->tx = SDL_CreateTextureFromSurface(r, su);
    l->w = su->w;
    l->h = su->h;
    SDL_FreeSurface(su);
}

static void label_draw(SDL_Renderer *r, label_t *l, int x, int y)
{
    SDL_Rect dst;
    if (!l->tx) return;
    dst.x = x; dst.y = y; dst.w = l->w; dst.h = l->h;
    SDL_RenderCopy(r, l->tx, NULL, &dst);
}

static void draw_cursor(SDL_Renderer *r, int x, int y)
{
    fill(r, x, y, 14, 14, 0, 0, 0);
    fill(r, x + 2, y + 2, 10, 10, 255, 255, 255);
}

#define NBTN 16
static const char *BTN_NAMES[NBTN] = {
    "Cross", "Circle", "Square", "Triangle", "L1", "R1", "L2", "R2",
    "L3", "R3", "Start", "Select", "Up", "Down", "Left", "Right"
};

int main(int argc, char *argv[])
{
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_RendererInfo info;
    SDL_Event ev;
    TTF_Font *font = NULL;
    padData pd;
    int cur[NBTN], prev[NBTN];
    char last[128] = "nenhum input ainda";
    char buf[200], held[200];
    int running = 1, w = 1280, h = 720, cx, cy, i, pad_ok = 0, evcount = 0;
    Uint32 t0, frames = 0, fps = 0;
    label_t l_menu, l_task, l_title, l_ver, l_rend, l_fps, l_pad, l_last,
            l_held, l_hint;

    (void)argc; (void)argv;
    memset(&l_menu, 0, sizeof l_menu);   memset(&l_task, 0, sizeof l_task);
    memset(&l_title, 0, sizeof l_title); memset(&l_ver, 0, sizeof l_ver);
    memset(&l_rend, 0, sizeof l_rend);   memset(&l_fps, 0, sizeof l_fps);
    memset(&l_pad, 0, sizeof l_pad);     memset(&l_last, 0, sizeof l_last);
    memset(&l_held, 0, sizeof l_held);   memset(&l_hint, 0, sizeof l_hint);
    memset(cur, 0, sizeof cur);
    memset(prev, 0, sizeof prev);

    bb_log("=== BBhM v0.0.2 iniciando ===");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
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

    ioPadInit(7);
    bb_log("ioPadInit ok");

    cx = w / 2; cy = h / 2;
    t0 = SDL_GetTicks();
    bb_log("entrando no loop");

    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (evcount < 30) {
                bb_log("evento SDL tipo=0x%x", (unsigned)ev.type);
                evcount++;
            }
            if (ev.type == SDL_QUIT) {
                bb_log("SDL_QUIT recebido");
                running = 0;
            }
        }

        /* pad nativo */
        memset(&pd, 0, sizeof pd);
        pad_ok = (ioPadGetData(0, &pd) == 0 && pd.len > 0);
        if (pad_ok) {
            cur[0]  = pd.BTN_CROSS;   cur[1]  = pd.BTN_CIRCLE;
            cur[2]  = pd.BTN_SQUARE;  cur[3]  = pd.BTN_TRIANGLE;
            cur[4]  = pd.BTN_L1;      cur[5]  = pd.BTN_R1;
            cur[6]  = pd.BTN_L2;      cur[7]  = pd.BTN_R2;
            cur[8]  = pd.BTN_L3;      cur[9]  = pd.BTN_R3;
            cur[10] = pd.BTN_START;   cur[11] = pd.BTN_SELECT;
            cur[12] = pd.BTN_UP;      cur[13] = pd.BTN_DOWN;
            cur[14] = pd.BTN_LEFT;    cur[15] = pd.BTN_RIGHT;

            held[0] = 0;
            for (i = 0; i < NBTN; i++) {
                if (cur[i]) {
                    strncat(held, BTN_NAMES[i], sizeof held - strlen(held) - 2);
                    strncat(held, " ", sizeof held - strlen(held) - 1);
                }
                if (cur[i] && !prev[i]) {
                    snprintf(last, sizeof last, "botao %s", BTN_NAMES[i]);
                    bb_log("input: %s", last);
                }
                prev[i] = cur[i];
            }

            {
                int ax = (int)pd.ANA_L_H - 128;
                int ay = (int)pd.ANA_L_V - 128;
                if (ax > 20 || ax < -20) cx += ax / 8;
                if (ay > 20 || ay < -20) cy += ay / 8;
            }

            if (cur[10] && cur[11]) {
                bb_log("Start+Select: saindo");
                running = 0;
            }
        } else {
            held[0] = 0;
        }
        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;
        if (cx > w - 14) cx = w - 14;
        if (cy > h - 14) cy = h - 14;

        /* textos (so re-renderizam quando mudam) */
        label_set(ren, font, &l_menu, "Applications   Places   System", 20, 20, 20);
        label_set(ren, font, &l_task, "BBhM", 20, 20, 20);
        label_set(ren, font, &l_title, "BBhM - teste do esqueleto", 255, 255, 255);
        label_set(ren, font, &l_ver, "BBhM v0.0.2 (passo 1b)", 20, 20, 20);
        snprintf(buf, sizeof buf, "Renderer: %s  %dx%d", info.name, w, h);
        label_set(ren, font, &l_rend, buf, 20, 20, 20);
        snprintf(buf, sizeof buf, "FPS: %u", (unsigned)fps);
        label_set(ren, font, &l_fps, buf, 20, 20, 20);
        snprintf(buf, sizeof buf, "Controle: %s", pad_ok ? "conectado" : "nao detectado");
        label_set(ren, font, &l_pad, buf, 20, 20, 20);
        snprintf(buf, sizeof buf, "Ultimo input: %s", last);
        label_set(ren, font, &l_last, buf, 20, 20, 20);
        snprintf(buf, sizeof buf, "Segurando: %s", held[0] ? held : "-");
        label_set(ren, font, &l_held, buf, 20, 20, 20);
        label_set(ren, font, &l_hint, "Sair: Start + Select", 90, 90, 90);

        /* desktop */
        fill(ren, 0, 0, w, h, 58, 110, 165);
        fill(ren, 0, 0, w, 32, 232, 232, 228);
        fill(ren, 0, 32, w, 1, 160, 160, 156);
        fill(ren, 0, h - 32, w, 32, 232, 232, 228);
        fill(ren, 0, h - 33, w, 1, 160, 160, 156);
        label_draw(ren, &l_menu, 12, 3);
        label_draw(ren, &l_task, 12, h - 29);

        /* janela de teste */
        fill(ren, w / 2 - 320, 110, 640, 380, 90, 90, 90);
        fill(ren, w / 2 - 318, 112, 636, 30, 74, 111, 165);
        fill(ren, w / 2 - 318, 142, 636, 346, 242, 241, 240);
        label_draw(ren, &l_title, w / 2 - 308, 114);
        label_draw(ren, &l_ver,  w / 2 - 300, 160);
        label_draw(ren, &l_rend, w / 2 - 300, 195);
        label_draw(ren, &l_fps,  w / 2 - 300, 230);
        label_draw(ren, &l_pad,  w / 2 - 300, 265);
        label_draw(ren, &l_last, w / 2 - 300, 300);
        label_draw(ren, &l_held, w / 2 - 300, 335);
        label_draw(ren, &l_hint, w / 2 - 300, 440);

        draw_cursor(ren, cx, cy);
        SDL_RenderPresent(ren);

        frames++;
        if (SDL_GetTicks() - t0 >= 1000) {
            fps = frames; frames = 0; t0 = SDL_GetTicks();
            bb_log("fps=%u pad=%d cursor=%d,%d", (unsigned)fps, pad_ok, cx, cy);
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
