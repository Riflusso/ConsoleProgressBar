#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <chrono>
#include <thread>
#include <clocale>
#include <windows.h>

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
    else if (std::abs(speed - prevSpeed) > 1e-6) {
        drawHeader(screenWidth, speed);
        prevSpeed = speed;
    }

    if (prevX != -1 && prevX < startX && screenHeight == prevHeight) {
        setCursorPosition(prevX, prevY);
        std::cout << std::string(prevWidth - prevX, ' ');
    }

    setCursorPosition(startX, startY);
    std::cout << bar;

    prevX = startX;
    prevY = startY;
}

// Обработка нажатий клавиш через скан-коды (PS/2 Set 1) и Unicode-символы
void handleInput(double& progress, double& speed, bool& isPaused, bool& isRunning) {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD eventsCount = 0;
    GetNumberOfConsoleInputEvents(hIn, &eventsCount);

    while (eventsCount > 0) {
        INPUT_RECORD record;
        DWORD eventsRead = 0;
        // Используем именно ReadConsoleInputW (Unicode), чтобы не терять кириллицу
        ReadConsoleInputW(hIn, &record, 1, &eventsRead);
        eventsCount--;

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
            WORD vk = record.Event.KeyEvent.wVirtualKeyCode;
            WORD scan = record.Event.KeyEvent.wVirtualScanCode;
            wchar_t ch = record.Event.KeyEvent.uChar.UnicodeChar;

            // Пробел (VK 0x20, Scan 0x39)
            if (vk == VK_SPACE || scan == 0x39 || ch == L' ') {
                isPaused = !isPaused;
            }
            // Клавиша '+' / '=' (Scan 0x0D — кнопка '=' рядом с Backspace, Scan 0x4E — '+' на NumPad)
            else if (vk == VK_OEM_PLUS || vk == VK_ADD || scan == 0x0D || scan == 0x4E || ch == L'+' || ch == L'=') {
                speed += 0.25;
            }
            // Клавиша '-' / '_' (Scan 0x0C — кнопка '-' рядом с 0, Scan 0x4A — '-' на NumPad)
            else if (vk == VK_OEM_MINUS || vk == VK_SUBTRACT || scan == 0x0C || scan == 0x4A || ch == L'-' || ch == L'_') {
                speed -= 0.25;
                if (speed < 0.25) speed = 0.25;
            }
            // Клавиша 'R' / 'К' (Scan 0x13 — физическая кнопка R)
            else if (vk == 'R' || scan == 0x13 || ch == L'r' || ch == L'R' || ch == L'к' || ch == L'К') {
                progress = 0.0;
            }
            // Клавиша 'Q' / 'Й' (Scan 0x10 — физическая кнопка Q)
            else if (vk == 'Q' || scan == 0x10 || ch == L'q' || ch == L'Q' || ch == L'й' || ch == L'Й') {
                isRunning = false;
            }
        }
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
    const int targetFps = 60;

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
