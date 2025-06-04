#define NOMINMAX
#include <windows.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cctype>

const int CELL_SIZE = 80;
const int BOARD_SIZE = 8;
char board[8][8]; // 'w', 'b', 'W', 'B', '.'
bool selected = false;
int selectedX = -1, selectedY = -1;

struct Move {
    int fromX, fromY;
    int toX, toY;
};

void initBoard() {
    for (int y = 0; y < BOARD_SIZE; ++y)
        for (int x = 0; x < BOARD_SIZE; ++x)
            board[y][x] = ((x + y) % 2 == 1) ? ((y < 3) ? 'b' : (y > 4) ? 'w' : '.') : '.';
}

void DrawBoard(HDC hdc) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            RECT rect = { x * CELL_SIZE, y * CELL_SIZE, (x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE };
            HBRUSH brush = CreateSolidBrush((x + y) % 2 == 0 ? RGB(222, 184, 135) : RGB(139, 69, 19));
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);

            if (selected && x == selectedX && y == selectedY) {
                HPEN pen = CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
                HGDIOBJ oldPen = SelectObject(hdc, pen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
            }

            char piece = board[y][x];
            if (piece != '.') {
                COLORREF color = isupper(piece) ? (tolower(piece) == 'w' ? RGB(255, 255, 255) : RGB(0, 0, 0))
                    : (piece == 'w' ? RGB(255, 255, 255) : RGB(0, 0, 0));
                HBRUSH pieceBrush = CreateSolidBrush(color);
                SelectObject(hdc, pieceBrush);
                Ellipse(hdc, rect.left + 10, rect.top + 10, rect.right - 10, rect.bottom - 10);
                DeleteObject(pieceBrush);

                if (isupper(piece)) {
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, RGB(255, 215, 0));
                    HFONT hFont = CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                        DEFAULT_PITCH | FF_SWISS, L"Arial");
                    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
                    DrawText(hdc, L"K", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(hdc, oldFont);
                    DeleteObject(hFont);
                }
            }
        }
    }
}


bool canCaptureAgain(int x, int y, char player) {
    int dirs[4][2] = { {-1, -1}, {-1, 1}, {1, -1}, {1, 1} };
    if (tolower(board[y][x]) == board[y][x]) { // обычная шашка
        int dir = (player == 'b') ? 1 : -1;
        for (int dx = -1; dx <= 1; dx += 2) {
            int nx = x + dx * 2;
            int ny = y + dir * 2;
            int midX = x + dx;
            int midY = y + dir;
            if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE &&
                board[ny][nx] == '.' &&
                board[midY][midX] != '.' && tolower(board[midY][midX]) != player) {
                return true;
            }
        }
    }
    else { // дамка
        for (auto& d : dirs) {
            int dx = d[0], dy = d[1];
            int cx = x + dx, cy = y + dy;
            bool captured = false;
            while (cx + dx >= 0 && cx + dx < BOARD_SIZE && cy + dy >= 0 && cy + dy < BOARD_SIZE) {
                if (!captured && board[cy][cx] != '.' && tolower(board[cy][cx]) != player) {
                    captured = true;
                    cx += dx; cy += dy;
                }
                else if (captured && board[cy][cx] == '.') {
                    return true;
                }
                else break;
                cx += dx; cy += dy;
            }
        }
    }
    return false;
}
void makeMove(const Move& m) {
    char piece = board[m.fromY][m.fromX];
    board[m.toY][m.toX] = piece;
    board[m.fromY][m.fromX] = '.';

    // Превращение в дамку
    if (piece == 'w' && m.toY == 0) board[m.toY][m.toX] = 'W';
    if (piece == 'b' && m.toY == 7) board[m.toY][m.toX] = 'B';

    // Взятие
    if (abs(m.toX - m.fromX) > 1 && abs(m.toY - m.fromY) > 1) {
        int dx = (m.toX - m.fromX) / abs(m.toX - m.fromX);
        int dy = (m.toY - m.fromY) / abs(m.toY - m.fromY);
        int cx = m.fromX + dx;
        int cy = m.fromY + dy;

        while (cx != m.toX && cy != m.toY) {
            if (board[cy][cx] != '.' && tolower(board[cy][cx]) != tolower(piece)) {
                board[cy][cx] = '.';
                break; // удаляем только одну побитую фигуру
            }
            cx += dx;
            cy += dy;
        }
    }
}

std::vector<Move> getAllPossibleMoves(char player) {
    std::vector<Move> moves;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            char piece = board[y][x];
            if (piece != player && piece != toupper(player)) continue;
            bool isQueen = isupper(piece);
            int directions[4][2] = { {-1,-1}, {1,-1}, {-1,1}, {1,1} };
            for (auto& d : directions) {
                int dx = d[0], dy = d[1];
                if (isQueen) {
                    int i = 1;
                    while (true) {
                        int nx = x + dx * i;
                        int ny = y + dy * i;
                        if (nx < 0 || ny < 0 || nx >= BOARD_SIZE || ny >= BOARD_SIZE)
                            break;
                        if (board[ny][nx] == '.') {
                            moves.push_back({ x, y, nx, ny });
                        }
                        else {
                            int j = 1;
                            while (true) {
                                int cx = x + dx * j;
                                int cy = y + dy * j;
                                if (cx < 0 || cy < 0 || cx >= BOARD_SIZE || cy >= BOARD_SIZE)
                                    break;
                                if (board[cy][cx] == '.') {
                                    j++;
                                    continue;
                                }
                                if (tolower(board[cy][cx]) != player && board[cy + dy][cx + dx] == '.' &&
                                    cx + dx >= 0 && cx + dx < BOARD_SIZE && cy + dy >= 0 && cy + dy < BOARD_SIZE) {
                                    moves.push_back({ x, y, cx + dx, cy + dy });
                                }
                                break;
                            }
                            break;
                        }
                        i++;
                    }
                }
                else {
                    int nx = x + dx;
                    int ny = y + ((player == 'w') ? -1 : 1);
                    if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && board[ny][nx] == '.') {
                        moves.push_back({ x, y, nx, ny });
                    }
                    int jx = x + dx * 2;
                    int jy = y + ((player == 'w') ? -2 : 2);
                    int mx = x + dx;
                    int my = y + ((player == 'w') ? -1 : 1);
                    if (jx >= 0 && jx < BOARD_SIZE && jy >= 0 && jy < BOARD_SIZE &&
                        board[jy][jx] == '.' && board[my][mx] != '.' && tolower(board[my][mx]) != player) {
                        moves.push_back({ x, y, jx, jy });
                    }
                }
            }
        }
    }
    return moves;
}
void undoMove(const Move& m, char captured) {
    board[m.fromY][m.fromX] = board[m.toY][m.toX];
    board[m.toY][m.toX] = '.';
    // Отменить превращение в дамку
    if ((m.toY == 0 && board[m.fromY][m.fromX] == 'W') || (m.toY == 7 && board[m.fromY][m.fromX] == 'B')) {
        board[m.fromY][m.fromX] = tolower(board[m.fromY][m.fromX]);

    }

    if (captured != '.') {
        int stepX = (m.toX - m.fromX) / abs(m.toX - m.fromX);
        int stepY = (m.toY - m.fromY) / abs(m.toY - m.fromY);
        int cx = m.fromX + stepX;
        int cy = m.fromY + stepY;
        while (cx != m.toX && cy != m.toY) {
            if (board[cy][cx] == '.') {
                board[cy][cx] = captured;
                break;
            }
            cx += stepX;
            cy += stepY;
        }
    }
}
int evaluateBoard() {
    int score = 0;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            char piece = board[y][x];
            if (piece == 'b') score += 3;
            else if (piece == 'B') score += 5;
            else if (piece == 'w') score -= 3;
            else if (piece == 'W') score -= 5;
        }
    }
    return score;
}
int minimax(int depth, bool maximizingPlayer) {
    char currentPlayer = maximizingPlayer ? 'b' : 'w';
    std::vector<Move> moves = getAllPossibleMoves(currentPlayer);
    std::vector<Move> kingMoves = getAllPossibleMoves(toupper(currentPlayer));
    moves.insert(moves.end(), kingMoves.begin(), kingMoves.end());
    if (depth == 0 == moves.empty())
        return evaluateBoard();
    int bestScore = maximizingPlayer ? -10000 : 10000;
    for (const Move& m : moves) {
        char captured = '.';
        if (abs(m.toX - m.fromX) > 1) {
            int stepX = (m.toX - m.fromX) / abs(m.toX - m.fromX);
            int stepY = (m.toY - m.fromY) / abs(m.toY - m.fromY);
            int cx = m.fromX + stepX;
            int cy = m.fromY + stepY;
            while (cx != m.toX && cy != m.toY) {
                if (board[cy][cx] != '.' && tolower(board[cy][cx]) != currentPlayer) {
                    captured = board[cy][cx];
                    break;
                }
                cx += stepX;
                cy += stepY;
            }
        }
        makeMove(m);
        int score = minimax(depth - 1, !maximizingPlayer);
        undoMove(m, captured);
        if (maximizingPlayer)
            bestScore = std::max(bestScore, score);
        else
            bestScore = std::min(bestScore, score);
    }
    return bestScore;
}
Move getBestMoveForAI() {
    std::vector<Move> moves = getAllPossibleMoves('b');
    std::vector<Move> kingMoves = getAllPossibleMoves('B');
    moves.insert(moves.end(), kingMoves.begin(), kingMoves.end());
    if (moves.empty()) return { 0, 0, 0, 0 };
    int bestScore = -10000;
    Move bestMove = moves[0];
    for (const Move& m : moves) {
        char captured = '.';
        if (abs(m.toX - m.fromX) > 1) {
            int stepX = (m.toX - m.fromX) / abs(m.toX - m.fromX);
            int stepY = (m.toY - m.fromY) / abs(m.toY - m.fromY);
            int cx = m.fromX + stepX;
            int cy = m.fromY + stepY;
            while (cx != m.toX && cy != m.toY) {
                if (board[cy][cx] != '.' && tolower(board[cy][cx]) == 'w') {
                    captured = board[cy][cx];
                    break;
                }
                cx += stepX;
                cy += stepY;
            }
        }
        makeMove(m);
        int score = minimax(3, false);
        undoMove(m, captured);
        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
    }
    return bestMove;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawBoard(hdc);
        EndPaint(hwnd, &ps);
    } break;
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam) / CELL_SIZE;
        int y = HIWORD(lParam) / CELL_SIZE;
        if (selected) {
            int dx = x - selectedX;
            int dy = y - selectedY;
            Move m = { selectedX, selectedY, x, y };
            // Проверка допустимости хода
            std::vector<Move> possibleMoves = getAllPossibleMoves('w');
            bool valid = false;
            for (const auto& move : possibleMoves) {
                if (move.fromX == m.fromX && move.fromY == m.fromY &&
                    move.toX == m.toX && move.toY == m.toY) {
                    valid = true;
                    break;
                }
            }
            if (valid) {
                makeMove(m);
                // Превращение в дамку
                if (m.toY == 0 && board[m.toY][m.toX] == 'w')
                    board[m.toY][m.toX] = 'W';
                // Проверка на мультивзятие
                if (abs(m.toX - m.fromX) == 2 && canCaptureAgain(m.toX, m.toY, 'w')) {
                    selected = true;
                    selectedX = m.toX;
                    selectedY = m.toY;
                }
                else {
                    selected = false;
                    // Ход AI
                    Move aiMove = getBestMoveForAI();
                    makeMove(aiMove);
                    if (aiMove.toY == 7 && board[aiMove.toY][aiMove.toX] == 'b')
                        board[aiMove.toY][aiMove.toX] = 'B';
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
            else {
                selected = false;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        else if (board[y][x] == 'w' || board[y][x] == 'W') {
            selected = true;
            selectedX = x;
            selectedY = y;
            InvalidateRect(hwnd, NULL, TRUE);
        }
    } break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    initBoard();
    const wchar_t CLASS_NAME[] = L"CheckersWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Шашки",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CELL_SIZE * BOARD_SIZE + 16, CELL_SIZE * BOARD_SIZE + 39,
        NULL, NULL, hInstance, NULL
    );
    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
