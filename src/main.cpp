#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <clocale>
#include <windows.h>
#include <conio.h>

// Установка позиции курсора в консоли
void setCursorPosition(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 1. Функция полной очистки экрана
void clearScreen() {
    system("cls");
}

// 2. Функция инициализации консоли
void initConsole() {
    setlocale(LC_ALL, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8); // Дополнительно фиксируем UTF-8 для вывода в Windows

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    clearScreen();
}

// 3. Функция корректного выхода
void exitApp() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    clearScreen();
}

// Форматирование скорости в строку (например, "1.0x")
std::string formatSpeed(int delayMs) {
    double multiplier = 50.0 / delayMs;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << multiplier << "x";
    return oss.str();
}

// Сборка строки прогресс-бара (теперь только шкала, проценты и статус паузы)
std::string buildBarString(int progress, int barWidth, bool isPaused) {
    int filled = (barWidth * progress) / 100;
    std::string bar = "[";
    bar.append(filled, '#');
    bar.append(barWidth - filled, '-');
    bar += "] " + std::to_string(progress) + "%";
    if (isPaused) {
        bar += " [ПАУЗА]";
    }
    return bar;
}

// Отрисовка верхней панели управления со скоростью возле кнопок [+/-]
void drawHeader(int screenWidth, int delayMs) {
    setCursorPosition(0, 0);
    std::cout << std::string(screenWidth, ' ');
    setCursorPosition(1, 0);
    std::cout << "[Space] Пауза | [+/-] Скорость (" << formatSpeed(delayMs) << ") | [R] Сброс | [Q] Выход";
}

// 4. Функция отрисовки прогресс-бара строго по центру
void drawProgressBar(int progress, int barWidth, int delayMs, bool isPaused,
                     int& prevX, int& prevY, int& prevWidth, int& prevHeight, int& prevDelay) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int screenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    std::string bar = buildBarString(progress, barWidth, isPaused);

    // Вычисляем центр экрана
    int startX = (screenWidth - (int)bar.length()) / 2;
    int startY = screenHeight / 2;
    if (startX < 0) startX = 0;

    // Если размер окна изменился — стираем старый бар и перерисовываем шапку
    if (screenWidth != prevWidth || screenHeight != prevHeight) {
        if (prevY != -1) {
            setCursorPosition(0, prevY);
            std::cout << std::string(prevWidth, ' ');
        }
        drawHeader(screenWidth, delayMs);

        prevWidth = screenWidth;
        prevHeight = screenHeight;
        prevDelay = delayMs;
    }
    // Если окно не менялось, но изменилась скорость — обновляем только верхнюю строку
    else if (delayMs != prevDelay) {
        drawHeader(screenWidth, delayMs);
        prevDelay = delayMs;
    }

    // Если строка бара стала короче (например, сняли паузу или сбросили 100% -> 0%) — стираем хвосты
    if (prevX != -1 && prevX < startX && screenHeight == prevHeight) {
        setCursorPosition(prevX, prevY);
        std::cout << std::string(prevWidth - prevX, ' ');
    }

    // Отрисовываем прогресс-бар в центре
    setCursorPosition(startX, startY);
    std::cout << bar;

    prevX = startX;
    prevY = startY;
}

// Обработка нажатий клавиш
void handleInput(int& progress, int& delayMs, bool& isPaused, bool& isRunning) {
    if (_kbhit()) {
        char key = _getch();
        if (key == ' ') isPaused = !isPaused;
        if (key == '+' || key == '=') delayMs -= 10;
        if (key == '-' || key == '_') delayMs += 10;
        if (key == 'r' || key == 'R') progress = 0;
        if (key == 'q' || key == 'Q') isRunning = false;

        if (delayMs < 10) delayMs = 10;
        if (delayMs > 250) delayMs = 250;
    }
}

// Обновление значения прогресса
void updateProgress(int& progress, bool isPaused) {
    if (!isPaused) {
        progress++;
        if (progress > 100) {
            progress = 0;
        }
    }
}

int main() {
    const int barWidth = 40;
    int progress = 0;
    int delayMs = 50;
    bool isPaused = false;
    bool isRunning = true;

    int prevX = -1, prevY = -1;
    int prevWidth = -1, prevHeight = -1;
    int prevDelay = -1;

    initConsole();

    while (isRunning) {
        handleInput(progress, delayMs, isPaused, isRunning);
        drawProgressBar(progress, barWidth, delayMs, isPaused, prevX, prevY, prevWidth, prevHeight, prevDelay);
        updateProgress(progress, isPaused);
        Sleep(delayMs);
    }

    exitApp();
    return 0;
}
