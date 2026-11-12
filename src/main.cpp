#include <iostream>
#include <string>
#include <windows.h>
#include <conio.h>

void setCursorPosition(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

int main() {
    setlocale(LC_ALL, ".UTF-8");
    hideCursor();
    system("cls"); // Полная очистка только ОДИН раз при запуске

    const int barWidth = 40;
    int progress = 0;
    int delayMs = 50;
    bool isPaused = false;

    // Запоминаем прошлые координаты и ширину окна, чтобы убирать "хвосты" при ресайзе
    int prevX = -1, prevY = -1;
    int prevWidth = -1, prevHeight = -1;

    while (true) {
        // 1. Обработка клавиш
        if (_kbhit()) {
            char key = _getch();
            if (key == ' ') isPaused = !isPaused;
            if (key == '+' || key == '=') delayMs -= 10;
            if (key == '-' || key == '_') delayMs += 10;
            if (key == 'r' || key == 'R') progress = 0;
            if (key == 'q' || key == 'Q') break;

            if (delayMs < 5) delayMs = 5;
            if (delayMs > 500) delayMs = 500;
        }

        // 2. Узнаем актуальный размер окна прямо сейчас
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        int screenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        // 3. Собираем строку прогресс-бара заранее, чтобы знать её точную длину
        int filled = (barWidth * progress) / 100;
        std::string bar = "[";
        bar.append(filled, '#');
        bar.append(barWidth - filled, '-');
        bar += "] " + std::to_string(progress) + "%";
        if (isPaused) bar += " [PAUSED]";

        // 4. Считаем идеальный центр (половина ширины экрана минус половина длины нашей строки)
        int startX = (screenWidth - (int)bar.length()) / 2;
        int startY = screenHeight / 2;

        // Защита от выхода за левый край, если окно сделали совсем крошечным
        if (startX < 0) startX = 0;

        // 5. Если размер окна изменился — стираем старую строку пробелами
        if (screenWidth != prevWidth || screenHeight != prevHeight) {
            if (prevY != -1) {
                setCursorPosition(0, prevY);
                std::cout << std::string(prevWidth, ' '); // Затираем всю старую строку
            }
            // Обновляем подсказку сверху
            setCursorPosition(0, 0);
            std::cout << std::string(screenWidth, ' ');
            setCursorPosition(1, 0);
            std::cout << "[Space] Пауза | [+/-] Скорость | [R] Сброс | [Q] Выход";

            prevWidth = screenWidth;
            prevHeight = screenHeight;
        }
        // Если окно не менялось, но длина строки уменьшилась (например, с 100% на 0% или сняли паузу)
        else if (prevX != -1 && prevX < startX) {
            setCursorPosition(prevX, prevY);
            std::cout << std::string(prevWidth - prevX, ' '); // Стираем хвосты по бокам
        }

        // 6. Рисуем строку ровно посередине
        setCursorPosition(startX, startY);
        std::cout << bar;

        prevX = startX;
        prevY = startY;

        // 7. Обновляем прогресс
        if (!isPaused) {
            progress++;
            if (progress > 100) progress = 0;
        }

        Sleep(delayMs);
    }

    system("cls");
    return 0;
}
