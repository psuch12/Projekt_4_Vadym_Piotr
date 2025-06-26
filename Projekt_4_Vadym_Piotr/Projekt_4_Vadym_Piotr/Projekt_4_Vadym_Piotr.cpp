#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>

#pragma comment (lib, "Gdiplus.lib")
#pragma comment (lib, "Comctl32.lib")

using namespace Gdiplus;

#define IDC_GROUPBOX 101
#define IDC_RADIO_SQUARES 102
#define IDC_RADIO_TRIANGLES 103
#define IDC_RADIO_CIRCLES 104
#define IDC_EDIT_MASS 105
#define IDC_BUTTON_AUTOBUILD 106
#define IDC_BUTTON_AUTOBUILD2 107
#define IDT_AUTOBUIILD_TIMER 1

enum ShapeType { SHAPE_NONE, SHAPE_SQUARE, SHAPE_CIRCLE, SHAPE_TRIANGLE };
enum GameMode { MODE_SQUARES, MODE_TRIANGLES, MODE_CIRCLES };

enum ErrorType {
    ERROR_NONE,
    ERROR_PICKUP_WRONG_SHAPE,
    ERROR_PICKUP_NOT_TOPMOST,
    ERROR_PICKUP_MISS,
    ERROR_PICKUP_TOO_HEAVY,
    ERROR_DROP_MISALIGNED_OR_OVERLAP,
    ERROR_DROP_WRONG_SHAPE,
    ERROR_DROP_TOO_HIGH
};

enum AutoBuildState {
    STATE_IDLE,
    STATE_FINDING_BLOCK,
    STATE_MOVING_TROLLEY_TO_OBJ,
    STATE_LOWERING_HOOK_TO_OBJ,
    STATE_PICKING_UP,
    STATE_RAISING_HOOK,
    STATE_MOVING_TROLLEY_TO_TARGET,
    STATE_LOWERING_HOOK_TO_TARGET,
    STATE_PLACING,
    STATE_DONE
};

struct CraneObject {
    ShapeType type;
    int x, y;
    int size;
    int mass;
    bool isCarried;
    int stackLevel;
};

GameMode g_currentMode = MODE_SQUARES;
int trolleyX = 450;
int hookY = 200;
bool isCarryingObject = false;
int carriedObjectIndex = -1;
std::vector<CraneObject> sceneObjects;
ErrorType g_currentError = ERROR_NONE;
const WCHAR* g_errorMessage = L"";
int g_maxLiftMass = 50;
HWND g_hEditMass;
AutoBuildState g_autoBuildState = STATE_IDLE;
int g_autoBuildTargetIndex = -1;
Point g_autoBuildTargetPos;
ShapeType g_shapeToFind;
int g_currentBuildSequence = 0;

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 700;
const int PANEL_WIDTH = 200;
const int GAME_AREA_WIDTH = WINDOW_WIDTH - PANEL_WIDTH;
const int GROUND_Y = WINDOW_HEIGHT - 120;
const int OBJECT_SIZE = 30;
const int MAX_TOWER_HEIGHT = 3;
const int SAFE_HOOK_Y = 100;

HWND g_hRadioSquares, g_hRadioTriangles, g_hRadioCircles;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
void InitializeObjects();
void DrawShape(Graphics& g, const CraneObject& obj);
bool IsSpaceFree(RectF proposedRect, int ignoreIndex);
void ResetProgram(HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GDIPlusCraneSimulator";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, L"GDIPlusCraneSimulator", L"Symulator Dźwigu",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}

void InitializeObjects() {
    sceneObjects.clear();
    int startX = 250;
    int spacing = 45;
    int masses[] = { 10, 50, 25, 40 };
    for (int i = 0; i < 4; ++i) sceneObjects.push_back({ SHAPE_SQUARE, startX + (i * spacing), GROUND_Y, OBJECT_SIZE, masses[i], false, 1 });
    for (int i = 0; i < 4; ++i) sceneObjects.push_back({ SHAPE_TRIANGLE, startX + 200 + (i * spacing), GROUND_Y, OBJECT_SIZE, masses[i], false, 1 });
    for (int i = 0; i < 4; ++i) sceneObjects.push_back({ SHAPE_CIRCLE, startX + 400 + (i * spacing), GROUND_Y, OBJECT_SIZE, masses[i], false, 1 });
}

void ResetProgram(HWND hwnd) {
    g_currentMode = MODE_SQUARES;
    trolleyX = 450;
    hookY = 200;
    isCarryingObject = false;
    carriedObjectIndex = -1;
    g_currentError = ERROR_NONE;
    g_autoBuildState = STATE_IDLE;
    g_currentBuildSequence = 0;

    InitializeObjects();

    Button_SetCheck(g_hRadioSquares, BST_CHECKED);
    Button_SetCheck(g_hRadioTriangles, BST_UNCHECKED);
    Button_SetCheck(g_hRadioCircles, BST_UNCHECKED);

    SetFocus(hwnd);

    InvalidateRect(hwnd, NULL, FALSE);
}


bool IsSpaceFree(RectF proposedRect, int ignoreIndex) {
    for (size_t i = 0; i < sceneObjects.size(); ++i) {
        if (i == ignoreIndex) continue;
        RectF existingRect(sceneObjects[i].x, sceneObjects[i].y, sceneObjects[i].size, sceneObjects[i].size);
        if (proposedRect.IntersectsWith(existingRect)) {
            return false;
        }
    }
    return true;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        InitializeObjects();
        int panelX = GAME_AREA_WIDTH;

        CreateWindowEx(WS_EX_CLIENTEDGE, L"BUTTON", L"Wybierz tryb gry:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, panelX + 10, 10, 180, 130, hwnd, (HMENU)IDC_GROUPBOX, GetModuleHandle(NULL), NULL);
        g_hRadioSquares = CreateWindowEx(0, L"BUTTON", L"Buduj z kwadratów", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, panelX + 20, 40, 160, 20, hwnd, (HMENU)IDC_RADIO_SQUARES, GetModuleHandle(NULL), NULL);
        g_hRadioTriangles = CreateWindowEx(0, L"BUTTON", L"Buduj z trójkątów", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, panelX + 20, 70, 160, 20, hwnd, (HMENU)IDC_RADIO_TRIANGLES, GetModuleHandle(NULL), NULL);
        g_hRadioCircles = CreateWindowEx(0, L"BUTTON", L"Buduj z kół", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, panelX + 20, 100, 160, 20, hwnd, (HMENU)IDC_RADIO_CIRCLES, GetModuleHandle(NULL), NULL);
        Button_SetCheck(g_hRadioSquares, BST_CHECKED);

        CreateWindowEx(0, L"STATIC", L"Maks. udźwig:", WS_CHILD | WS_VISIBLE, panelX + 10, 150, 100, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
        g_hEditMass = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"50", WS_CHILD | WS_VISIBLE | ES_NUMBER, panelX + 110, 150, 60, 20, hwnd, (HMENU)IDC_EDIT_MASS, GetModuleHandle(NULL), NULL);
        CreateWindowEx(0, L"STATIC", L"(Zatwierdź Enter)", WS_CHILD | WS_VISIBLE, panelX + 10, 175, 180, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

        CreateWindowEx(0, L"BUTTON", L"Buduj wieżę (KW, T, K)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, panelX + 10, 230, 180, 30, hwnd, (HMENU)IDC_BUTTON_AUTOBUILD, GetModuleHandle(NULL), NULL);
        CreateWindowEx(0, L"BUTTON", L"Buduj wieżę (K, KW, T)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, panelX + 10, 270, 180, 30, hwnd, (HMENU)IDC_BUTTON_AUTOBUILD2, GetModuleHandle(NULL), NULL);

        WCHAR buffer[10];
        GetWindowText(g_hEditMass, buffer, 10);
        g_maxLiftMass = _wtoi(buffer);

        SetWindowSubclass(g_hEditMass, EditSubclassProc, 0, 0);

        return 0;
    }
    case WM_ERASEBKGND: {
        return 1;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 255));
        return (LRESULT)hBrush;
    }
    case WM_CTLCOLORBTN:
    {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;

        WCHAR className[256];
        GetClassName(hCtrl, className, 256);
        if (wcscmp(className, L"Button") == 0) {
            LONG style = GetWindowLong(hCtrl, GWL_STYLE);
            if ((style & BS_TYPEMASK) == BS_GROUPBOX) {
                SetBkColor(hdc, RGB(240, 240, 255));
                SetTextColor(hdc, RGB(0, 0, 0));
                static HBRUSH hGroupBoxBrush = CreateSolidBrush(RGB(240, 240, 255));
                return (LRESULT)hGroupBoxBrush;
            }
        }

        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 255));
        return (LRESULT)hBrush;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED) {
            if (g_autoBuildState != STATE_IDLE) return 0;

            switch (LOWORD(wParam)) {
            case IDC_RADIO_SQUARES: case IDC_RADIO_TRIANGLES: case IDC_RADIO_CIRCLES:
            {
                bool modeChanged = false;
                GameMode newMode = g_currentMode;
                if (LOWORD(wParam) == IDC_RADIO_SQUARES && g_currentMode != MODE_SQUARES) { newMode = MODE_SQUARES; modeChanged = true; }
                if (LOWORD(wParam) == IDC_RADIO_TRIANGLES && g_currentMode != MODE_TRIANGLES) { newMode = MODE_TRIANGLES; modeChanged = true; }
                if (LOWORD(wParam) == IDC_RADIO_CIRCLES && g_currentMode != MODE_CIRCLES) { newMode = MODE_CIRCLES; modeChanged = true; }

                if (modeChanged) {
                    g_currentMode = newMode;
                    isCarryingObject = false;
                    carriedObjectIndex = -1;
                    g_currentError = ERROR_NONE;
                    InitializeObjects();
                    InvalidateRect(hwnd, NULL, FALSE);
                    SetFocus(hwnd);
                }
                break;
            }
            case IDC_BUTTON_AUTOBUILD:
                isCarryingObject = false;
                carriedObjectIndex = -1;
                g_currentError = ERROR_NONE;
                InitializeObjects();

                g_currentBuildSequence = 1;
                g_shapeToFind = SHAPE_SQUARE;
                g_autoBuildState = STATE_FINDING_BLOCK;
                g_autoBuildTargetPos = { 850, GROUND_Y };
                SetTimer(hwnd, IDT_AUTOBUIILD_TIMER, 15, NULL);
                break;

            case IDC_BUTTON_AUTOBUILD2:
                isCarryingObject = false;
                carriedObjectIndex = -1;
                g_currentError = ERROR_NONE;
                InitializeObjects();

                g_currentBuildSequence = 2;
                g_shapeToFind = SHAPE_CIRCLE;
                g_autoBuildState = STATE_FINDING_BLOCK;
                g_autoBuildTargetPos = { 850, GROUND_Y };
                SetTimer(hwnd, IDT_AUTOBUIILD_TIMER, 15, NULL);
                break;
            }
        }
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT_MASS) {
            WCHAR buffer[10];
            GetWindowText(g_hEditMass, buffer, 10);
            g_maxLiftMass = _wtoi(buffer);
        }
        return 0;
    }
    case WM_TIMER:
    {
        if (wParam == IDT_AUTOBUIILD_TIMER)
        {
            int targetX = 0, targetY = 0;

            switch (g_autoBuildState)
            {
            case STATE_FINDING_BLOCK:
                g_autoBuildTargetIndex = -1;
                for (size_t i = 0; i < sceneObjects.size(); ++i) {
                    if (sceneObjects[i].type == g_shapeToFind && sceneObjects[i].y == GROUND_Y) {
                        g_autoBuildTargetIndex = i;
                        break;
                    }
                }
                if (g_autoBuildTargetIndex != -1) {
                    g_autoBuildState = STATE_MOVING_TROLLEY_TO_OBJ;
                }
                else {
                    g_autoBuildState = STATE_DONE;
                }
                break;

            case STATE_MOVING_TROLLEY_TO_OBJ:
                targetX = sceneObjects[g_autoBuildTargetIndex].x + OBJECT_SIZE / 2;
                if (trolleyX < targetX) trolleyX += 5;
                else if (trolleyX > targetX) trolleyX -= 5;
                if (abs(trolleyX - targetX) < 5) {
                    trolleyX = targetX;
                    g_autoBuildState = STATE_LOWERING_HOOK_TO_OBJ;
                }
                break;

            case STATE_LOWERING_HOOK_TO_OBJ:
                targetY = sceneObjects[g_autoBuildTargetIndex].y;
                if (hookY < targetY) hookY += 10;
                if (hookY >= targetY) {
                    hookY = targetY;
                    g_autoBuildState = STATE_PICKING_UP;
                }
                break;

            case STATE_PICKING_UP:
                carriedObjectIndex = g_autoBuildTargetIndex;
                isCarryingObject = true;
                sceneObjects[carriedObjectIndex].isCarried = true;
                g_autoBuildState = STATE_RAISING_HOOK;
                break;

            case STATE_RAISING_HOOK:
                if (hookY > SAFE_HOOK_Y) hookY -= 10;
                else {
                    hookY = SAFE_HOOK_Y;
                    g_autoBuildState = STATE_MOVING_TROLLEY_TO_TARGET;
                }
                break;

            case STATE_MOVING_TROLLEY_TO_TARGET:
                targetX = g_autoBuildTargetPos.X + OBJECT_SIZE / 2;
                if (trolleyX < targetX) trolleyX += 5;
                else if (trolleyX > targetX) trolleyX -= 5;
                if (abs(trolleyX - targetX) < 5) {
                    trolleyX = targetX;
                    g_autoBuildState = STATE_LOWERING_HOOK_TO_TARGET;
                }
                break;

            case STATE_LOWERING_HOOK_TO_TARGET:
                targetY = g_autoBuildTargetPos.Y;
                if (hookY < targetY) hookY += 10;
                if (hookY >= targetY) {
                    hookY = targetY;
                    g_autoBuildState = STATE_PLACING;
                }
                break;

            case STATE_PLACING:
                sceneObjects[carriedObjectIndex].x = g_autoBuildTargetPos.X;
                sceneObjects[carriedObjectIndex].y = g_autoBuildTargetPos.Y;
                sceneObjects[carriedObjectIndex].isCarried = false;
                sceneObjects[carriedObjectIndex].stackLevel = (GROUND_Y - g_autoBuildTargetPos.Y) / OBJECT_SIZE + 1;
                isCarryingObject = false;
                carriedObjectIndex = -1;

                g_autoBuildTargetPos.Y -= OBJECT_SIZE;

                if (g_currentBuildSequence == 1) {
                    if (g_shapeToFind == SHAPE_SQUARE) g_shapeToFind = SHAPE_TRIANGLE;
                    else if (g_shapeToFind == SHAPE_TRIANGLE) g_shapeToFind = SHAPE_CIRCLE;
                    else g_autoBuildState = STATE_DONE;
                }
                else if (g_currentBuildSequence == 2) {
                    if (g_shapeToFind == SHAPE_CIRCLE) g_shapeToFind = SHAPE_SQUARE;
                    else if (g_shapeToFind == SHAPE_SQUARE) g_shapeToFind = SHAPE_TRIANGLE;
                    else g_autoBuildState = STATE_DONE;
                }


                if (g_autoBuildState != STATE_DONE) {
                    g_autoBuildState = STATE_FINDING_BLOCK;
                }
                break;

            case STATE_DONE:
                KillTimer(hwnd, IDT_AUTOBUIILD_TIMER);
                ResetProgram(hwnd);
                break;
            }

            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        Bitmap backBuffer(rc.right, rc.bottom);
        Graphics graphicsFromImage(&backBuffer);
        graphicsFromImage.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);

        graphicsFromImage.Clear(Color(240, 240, 255));

        SolidBrush groundBrush(Color(200, 220, 200));
        graphicsFromImage.FillRectangle(&groundBrush, 0, GROUND_Y + OBJECT_SIZE, GAME_AREA_WIDTH, WINDOW_HEIGHT - (GROUND_Y + OBJECT_SIZE));

        Pen dividerPen(Color(50, 50, 50), 2);
        graphicsFromImage.DrawLine(&dividerPen, GAME_AREA_WIDTH, 0, GAME_AREA_WIDTH, WINDOW_HEIGHT);

        SolidBrush yellowBrush(Color(255, 255, 210, 40));
        SolidBrush darkGrayBrush(Color(255, 80, 80, 80));
        graphicsFromImage.FillRectangle(&darkGrayBrush, 200, GROUND_Y + OBJECT_SIZE - 20, 80, 20);
        graphicsFromImage.FillRectangle(&darkGrayBrush, 210, GROUND_Y + OBJECT_SIZE - 40, 60, 20);
        Point towerPoints[] = { Point(225, GROUND_Y + OBJECT_SIZE - 40), Point(255, GROUND_Y + OBJECT_SIZE - 40), Point(245, 50), Point(235, 50) };
        graphicsFromImage.FillPolygon(&yellowBrush, towerPoints, 4);
        Point jibPoints[] = { Point(240, 50), Point(240, 65), Point(GAME_AREA_WIDTH - 50, 65), Point(GAME_AREA_WIDTH - 50, 58) };
        graphicsFromImage.FillPolygon(&yellowBrush, jibPoints, 4);
        Point counterJibPoints[] = { Point(240, 50), Point(240, 65), Point(120, 65), Point(120, 58) };
        graphicsFromImage.FillPolygon(&yellowBrush, counterJibPoints, 4);
        graphicsFromImage.FillRectangle(&darkGrayBrush, 80, 45, 40, 40);

        for (const auto& obj : sceneObjects) {
            if (!obj.isCarried) DrawShape(graphicsFromImage, obj);
        }

        Pen blackPen(Color(255, 0, 0, 0), 2.0f);
        graphicsFromImage.FillRectangle(&yellowBrush, trolleyX - 10, 65, 20, 15);
        graphicsFromImage.DrawRectangle(&blackPen, trolleyX - 10, 65, 20, 15);

        if (isCarryingObject && carriedObjectIndex != -1) {
            const auto& carriedObj = sceneObjects[carriedObjectIndex];
            graphicsFromImage.DrawLine(&blackPen, trolleyX, 80, trolleyX, hookY);
            CraneObject drawObject = carriedObj;
            drawObject.x = trolleyX - drawObject.size / 2;
            drawObject.y = hookY;
            DrawShape(graphicsFromImage, drawObject);
        }
        else {
            graphicsFromImage.DrawLine(&blackPen, trolleyX, 80, trolleyX, hookY);
            graphicsFromImage.DrawLine(&blackPen, trolleyX - 5, hookY, trolleyX + 5, hookY);
        }

        if (g_currentError != ERROR_NONE) {
            switch (g_currentError) {
            case ERROR_PICKUP_WRONG_SHAPE: g_errorMessage = L"Możesz podnosić tylko klocki pasujące do obecnego trybu gry."; break;
            case ERROR_PICKUP_NOT_TOPMOST: g_errorMessage = L"Możesz podnosić tylko klocki, które są na samej górze."; break;
            case ERROR_PICKUP_MISS: g_errorMessage = L"Hak musi być bezpośrednio nad klockiem, aby go podnieść."; break;
            case ERROR_PICKUP_TOO_HEAVY: g_errorMessage = L"Klocek jest za ciężki dla dźwigu!"; break;
            case ERROR_DROP_WRONG_SHAPE: g_errorMessage = L"Możesz stawiać klocek tylko na klocku tego samego kształtu."; break;
            case ERROR_DROP_TOO_HIGH: g_errorMessage = L"Wieża nie może być wyższa niż 3 klocki."; break;
            case ERROR_DROP_MISALIGNED_OR_OVERLAP: g_errorMessage = L"Klocki muszą być układane idealnie jeden na drugim i nie mogą się nachodzić."; break;
            default: g_errorMessage = L""; break;
            }

            FontFamily fontFamily(L"Arial");
            Font font(&fontFamily, 14, FontStyleBold, UnitPixel);
            SolidBrush textBrush(Color(255, 200, 0, 0));
            StringFormat stringFormat;
            stringFormat.SetAlignment(StringAlignmentCenter);
            stringFormat.SetLineAlignment(StringAlignmentCenter);
            RectF textRect(GAME_AREA_WIDTH + 10.0f, 310.0f, PANEL_WIDTH - 20.0f, 120.0f);
            graphicsFromImage.DrawString(g_errorMessage, -1, &font, textRect, &stringFormat, &textBrush);
        }

        Graphics windowGraphics(hdc);
        windowGraphics.DrawImage(&backBuffer, 0, 0);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (g_autoBuildState != STATE_IDLE) return 0;

        if (g_currentError != ERROR_NONE) {
            g_currentError = ERROR_NONE;
            InvalidateRect(hwnd, NULL, FALSE);
        }

        switch (wParam)
        {
        case VK_LEFT:  if (trolleyX > 240) trolleyX -= 5; break;
        case VK_RIGHT: if (trolleyX < GAME_AREA_WIDTH - 60) trolleyX += 5; break;
        case VK_UP:    if (hookY > 100) hookY -= 10; break;
        case VK_DOWN:  if (hookY < GROUND_Y) hookY += 10; break;
        case VK_SPACE:
        {
            if (!isCarryingObject) {
                int targetIdx = -1;
                for (size_t i = 0; i < sceneObjects.size(); i++) {
                    if (sceneObjects[i].isCarried) continue;
                    bool collision = (trolleyX >= sceneObjects[i].x && trolleyX <= sceneObjects[i].x + sceneObjects[i].size) && (hookY >= sceneObjects[i].y && hookY <= sceneObjects[i].y + sceneObjects[i].size);
                    if (collision) {
                        targetIdx = i;
                        break;
                    }
                }

                if (targetIdx == -1) {
                    g_currentError = ERROR_PICKUP_MISS;
                }
                else {
                    ShapeType requiredShape;
                    switch (g_currentMode) {
                    case MODE_SQUARES:   requiredShape = SHAPE_SQUARE; break;
                    case MODE_TRIANGLES: requiredShape = SHAPE_TRIANGLE; break;
                    case MODE_CIRCLES:   requiredShape = SHAPE_CIRCLE; break;
                    }
                    bool shapeMatch = sceneObjects[targetIdx].type == requiredShape;

                    bool isTopmost = true;
                    for (size_t j = 0; j < sceneObjects.size(); ++j) {
                        if (targetIdx == j || sceneObjects[j].isCarried) continue;
                        const auto& otherObj = sceneObjects[j];
                        if (otherObj.y + otherObj.size == sceneObjects[targetIdx].y && (otherObj.x < sceneObjects[targetIdx].x + sceneObjects[targetIdx].size && sceneObjects[targetIdx].x < otherObj.x + otherObj.size)) {
                            isTopmost = false;
                            break;
                        }
                    }

                    if (!shapeMatch) {
                        g_currentError = ERROR_PICKUP_WRONG_SHAPE;
                    }
                    else if (!isTopmost) {
                        g_currentError = ERROR_PICKUP_NOT_TOPMOST;
                    }
                    else if (sceneObjects[targetIdx].mass > g_maxLiftMass) {
                        g_currentError = ERROR_PICKUP_TOO_HEAVY;
                    }
                    else {
                        trolleyX = sceneObjects[targetIdx].x + sceneObjects[targetIdx].size / 2;
                        isCarryingObject = true;
                        carriedObjectIndex = targetIdx;
                        sceneObjects[targetIdx].isCarried = true;
                    }
                }
                if (g_currentError != ERROR_NONE) MessageBeep(MB_ICONERROR);
            }
            else {
                if (carriedObjectIndex != -1) {
                    auto& droppedObj = sceneObjects[carriedObjectIndex];
                    int potentialX = trolleyX - droppedObj.size / 2;

                    CraneObject* topBlockInColumn = nullptr;
                    for (size_t i = 0; i < sceneObjects.size(); ++i) {
                        if (sceneObjects[i].isCarried) continue;
                        if (abs(sceneObjects[i].x - potentialX) < droppedObj.size / 2) {
                            if (topBlockInColumn == nullptr || sceneObjects[i].y < topBlockInColumn->y) {
                                topBlockInColumn = &sceneObjects[i];
                            }
                        }
                    }

                    int potentialY;
                    int potentialStackLevel;

                    if (topBlockInColumn != nullptr && abs(topBlockInColumn->x - potentialX) > 2) {
                        g_currentError = ERROR_DROP_MISALIGNED_OR_OVERLAP;
                    }
                    else if (topBlockInColumn != nullptr) {
                        potentialY = topBlockInColumn->y - droppedObj.size;
                        potentialStackLevel = topBlockInColumn->stackLevel + 1;

                        if (droppedObj.type != topBlockInColumn->type) {
                            g_currentError = ERROR_DROP_WRONG_SHAPE;
                        }
                        else if (topBlockInColumn->stackLevel >= MAX_TOWER_HEIGHT) {
                            g_currentError = ERROR_DROP_TOO_HIGH;
                        }
                        else if (hookY > potentialY) {
                            g_currentError = ERROR_DROP_MISALIGNED_OR_OVERLAP;
                        }
                    }
                    else {
                        potentialY = GROUND_Y;
                        potentialStackLevel = 1;
                        if (hookY > potentialY) {
                            g_currentError = ERROR_DROP_MISALIGNED_OR_OVERLAP;
                        }
                    }

                    if (g_currentError == ERROR_NONE) {
                        RectF finalRect(potentialX, potentialY, droppedObj.size, droppedObj.size);
                        if (!IsSpaceFree(finalRect, carriedObjectIndex)) {
                            g_currentError = ERROR_DROP_MISALIGNED_OR_OVERLAP;
                        }
                        else {
                            droppedObj.x = potentialX;
                            droppedObj.y = potentialY;
                            droppedObj.stackLevel = potentialStackLevel;
                            droppedObj.isCarried = false;
                            isCarryingObject = false;
                            carriedObjectIndex = -1;
                        }
                    }

                    if (g_currentError != ERROR_NONE) MessageBeep(MB_ICONERROR);
                }
            }
        }
        break;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, IDT_AUTOBUIILD_TIMER);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DrawShape(Graphics& g, const CraneObject& obj) {
    SolidBrush redBrush(Color(255, 200, 50, 50));
    SolidBrush greenBrush(Color(255, 50, 200, 50));
    SolidBrush blueBrush(Color(255, 50, 50, 200));
    Pen blackPen(Color::Black, 1.5f);
    RectF objectRect(static_cast<float>(obj.x), static_cast<float>(obj.y), static_cast<float>(obj.size), static_cast<float>(obj.size));

    switch (obj.type) {
    case SHAPE_SQUARE:
        g.FillRectangle(&redBrush, objectRect);
        g.DrawRectangle(&blackPen, objectRect);
        break;
    case SHAPE_CIRCLE:
        g.FillEllipse(&greenBrush, objectRect);
        g.DrawEllipse(&blackPen, objectRect);
        break;
    case SHAPE_TRIANGLE:
    {
        PointF points[] = {
            PointF(obj.x + obj.size / 2.0f, (float)obj.y),
            PointF((float)obj.x + obj.size, (float)obj.y + obj.size),
            PointF((float)obj.x, (float)obj.y + obj.size)
        };
        g.FillPolygon(&blueBrush, points, 3);
        g.DrawPolygon(&blackPen, points, 3);
        break;
    }
    default: break;
    }

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 12, FontStyleBold, UnitPixel);
    SolidBrush textBrush(Color::White);
    StringFormat stringFormat;
    stringFormat.SetAlignment(StringAlignmentCenter);
    stringFormat.SetLineAlignment(StringAlignmentCenter);

    WCHAR massText[10];
    swprintf(massText, 10, L"%d", obj.mass);

    RectF textRect = objectRect;
    textRect.Y += textRect.Height / 6.0f;

    g.DrawString(massText, -1, &font, textRect, &stringFormat, &textBrush);
}

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_CHAR:
        if (wParam == VK_RETURN)
        {
            SetFocus(GetParent(hwnd));
            return 0;
        }
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, EditSubclassProc, uIdSubclass);
        break;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}
