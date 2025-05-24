#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <math.h>

#define f64 double

#define FADE_FACTOR (255 / 256)
#define WIDTH 800
#define HEIGHT 800
#define DT 0.01
const f64 G = 1.0;

typedef struct vec2 {
    f64 x;
    f64 y;
} vec2;
typedef struct physical_object {
    vec2 pos;
    vec2 vel;
    f64 acc;
    f64 mass;
} pobj;

pobj objs[3] = {
    {
        .mass = 1.0,
        .pos = {-0.97000436,  0.24308753},
        .vel = { 0.4662036850,  0.4323657300 },
        .acc = 0.0
    },
    {
        .mass = 1.0,
        .pos = {  0.97000436, -0.24308753 },
        .vel = { 0.4662036850,  0.4323657300},
        .acc = 0.0
    },
    {
        .mass = 1.0,
        .pos = { 0.0, 0.0 },
        .vel = {-0.93240737,   -0.86473146 },
        .acc = 0.0
    }
};

HDC backHDC;
HBITMAP backBitmap;
unsigned char* pixelBuffer = NULL;

static void fade_trails() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int index = (y * WIDTH + x) * 4;
            for (int i = 0; i < 3; i++) {
                pixelBuffer[index + i] = pixelBuffer[index + i] * FADE_FACTOR;
            }
        }
    }
}

static void apply_forces(pobj* objs, int count) {
    for (int i = 0; i < count; i++) {
        vec2 tot_acc = { 0 };
        for (int j = 0; j < count; j++) {
            if (i == j) continue;

            vec2 delta = {
                .x = objs[j].pos.x - objs[i].pos.x,
                .y = objs[j].pos.y - objs[i].pos.y
            };
            f64 rsq = delta.x * delta.x + delta.y * delta.y;
            f64 r = sqrt(rsq);

            if (r < 1) continue;

            f64 force = G * objs[i].mass * objs[j].mass / rsq;
            vec2 dir = { delta.x / r, delta.y / r };

            tot_acc.x += force * dir.x / objs[i].mass;
            tot_acc.y += force * dir.y / objs[i].mass;
        }

        objs[i].vel.x += tot_acc.x * DT;
        objs[i].vel.y += tot_acc.y * DT;

        objs[i].pos.x += objs[i].vel.x * DT;
        objs[i].pos.y += objs[i].vel.y * DT;
    }
}

static void draw_frame() {
    fade_trails();
    apply_forces(objs, 3);

    for (int i = 0; i < 3; i++) {
        int x = (int)(objs[i].pos.x * 250.0) + WIDTH / 2;
        int y = (int)(objs[i].pos.y * 250.0) + HEIGHT / 2;

        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            int index = (y * WIDTH + x) * 4;
            pixelBuffer[index + i] = 255;
        }
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            BitBlt(hdc, 0, 0, WIDTH, HEIGHT, backHDC, 0, 0, SRCCOPY);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER: {
            draw_frame();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"Three-Body";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"ThreeBody",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        WIDTH, HEIGHT, NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);

    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = WIDTH;
    bi.bmiHeader.biHeight = -HEIGHT;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    backHDC = CreateCompatibleDC(NULL);
    backBitmap = CreateDIBSection(backHDC, &bi, DIB_RGB_COLORS, (void**)&pixelBuffer, NULL, 0);
    SelectObject(backHDC, backBitmap);

    memset(pixelBuffer, 0, WIDTH * HEIGHT * 4);

    SetTimer(hwnd, 1, 10, NULL);
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(backBitmap);
    DeleteDC(backHDC);
    return 0;
}
