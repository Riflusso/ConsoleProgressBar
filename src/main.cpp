#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <chrono>
#include <thread>
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
    SetConsoleOutputCP(CP_UTF8);

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

// Форматирование скорости
std::string formatSpeed(double speed) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << speed << "x";
    return oss.str();
}

// Сборка строки прогресс-бара
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

// Отрисовка верхней панели управления
void drawHeader(int screenWidth, double speed) {
    setCursorPosition(0, 0);
    std::cout << std::string(screenWidth, ' ');
    setCursorPosition(1, 0);
    std::cout << "[Space] Пауза | [+/-] Скорость (" << formatSpeed(speed) << ") | [R] Сброс | [Q] Выход";
}

// 4. Функция отрисовки прогресс-бара строго по центру
void drawProgressBar(int progress, int barWidth, double speed, bool isPaused,
                     int& prevX, int& prevY, int& prevWidth, int& prevHeight, double& prevSpeed) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int screenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    std::string bar = buildBarString(progress, barWidth, isPaused);

    int startX = (screenWidth - (int)bar.length()) / 2;
    int startY = screenHeight / 2;
    if (startX < 0) startX = 0;

    // Если размер окна изменился — стираем старый бар и перерисовываем шапку
    if (screenWidth != prevWidth || screenHeight != prevHeight) {
        if (prevY != -1) {
            setCursorPosition(0, prevY);
            std::cout << std::string(prevWidth, ' ');
        }
        drawHeader(screenWidth, speed);

        prevWidth = screenWidth;
        prevHeight = screenHeight;
        prevSpeed = speed;
    }
    // Если изменилась скорость — обновляем верхнюю строку
    else if (std::abs(speed - prevSpeed) > 1e-6) {
        drawHeader(screenWidth, speed);
        prevSpeed = speed;
    }

    // Если строка бара стала короче — стираем хвосты
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
void handleInput(double& progress, double& speed, bool& isPaused, bool& isRunning) {
    if (_kbhit()) {
        char key = _getch();
        if (key == ' ') isPaused = !isPaused;

        // Регулировка скорости прогресс-бара (шаг 0.25, минимум 0.25, максимума нет)
        if (key == '+' || key == '=') speed += 0.25;
        if (key == '-' || key == '_') {
            speed -= 0.25;
            if (speed < 0.25) speed = 0.25;
        }

        if (key == 'r' || key == 'R') progress = 0.0;
        if (key == 'q' || key == 'Q') isRunning = false;
    }
}

// Обновление прогресса по реальному времени
void updateProgress(double& progress, double speed, bool isPaused, double deltaSeconds) {
    if (!isPaused) {
        const double basePercentPerSecond = 20.0;
        progress += basePercentPerSecond * speed * deltaSeconds;
        progress = std::fmod(progress, 100.0);
    }
}

// Ожидание до конца текущего кадра исходя из частоты кадров
void waitForNextFrame(std::chrono::steady_clock::time_point frameStart, int fps) {
    auto targetDuration = std::chrono::duration<double>(1.0 / fps);
    auto elapsed = std::chrono::steady_clock::now() - frameStart;

    if (elapsed < targetDuration) {
        std::this_thread::sleep_for(targetDuration - elapsed);
    }
}

int main() {
    const int barWidth = 40;
    const int targetFps = 60; // Частота кадров задана в коде

    double progress = 0.0;
    double speed = 1.0;
    bool isPaused = false;
    bool isRunning = true;

    int prevX = -1, prevY = -1;
    int prevWidth = -1, prevHeight = -1;
    double prevSpeed = -1.0;

    initConsole();

    auto lastTime = std::chrono::steady_clock::now();

    while (isRunning) {
        auto frameStart = std::chrono::steady_clock::now();
        std::chrono::duration<double> deltaTime = frameStart - lastTime;
        lastTime = frameStart;

        handleInput(progress, speed, isPaused, isRunning);
        updateProgress(progress, speed, isPaused, deltaTime.count());
        drawProgressBar(static_cast<int>(progress), barWidth, speed, isPaused,
                        prevX, prevY, prevWidth, prevHeight, prevSpeed);

        waitForNextFrame(frameStart, targetFps);
    }

    exitApp();
    return 0;
}
