#include <iostream>
#include <windows.h> // Для управления курсором и задержки (Sleep)
#include <conio.h>   // Для считывания клавиш без Enter (_kbhit, _getch)

// Простая функция для перемещения курсора в нужную точку (X, Y)
void setCursorPosition(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Функция для скрытия мигающего курсора (чтобы не мешал)
void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

int main() {
    setlocale(LC_ALL, ".UTF-8");
    hideCursor();
    system("cls"); // Очищаем экран ОДИН раз перед началом

    // 1. Узнаем размер окна консоли, чтобы найти центр
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int screenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // 2. Настройки прогресс-бара
    const int barWidth = 40; // Длина самого бара (40 символов)
    int progress = 0;        // Текущий процент (0..100)
    int delayMs = 50;        // Скорость (задержка в миллисекундах)
    bool isPaused = false;

    // Вычисляем координаты центра (учитываем длину бара + скобки и текст процентов)
    int startX = (screenWidth - (barWidth + 7)) / 2;
    int startY = screenHeight / 2;

    // Выводим подсказку в самом верху экрана один раз
    setCursorPosition(2, 1);
    std::cout << "[Space] Пауза | [+/-] Скорость | [R] Сброс | [Q] Выход";

    // 3. Главный цикл программы
    while (true) {
        // Проверяем, нажал ли пользователь какую-нибудь клавишу
        if (_kbhit()) {
            char key = _getch(); // Считываем символ без ожидания Enter

            if (key == ' ') isPaused = !isPaused;            // Пауза
            if (key == '+' || key == '=') delayMs -= 10;     // Быстрее
            if (key == '-' || key == '_') delayMs += 10;     // Медленнее
            if (key == 'r' || key == 'R') progress = 0;      // Сброс
            if (key == 'q' || key == 'Q') break;             // Выход из цикла

            // Ограничиваем скорость, чтобы задержка не ушла в минус
            if (delayMs < 5) delayMs = 5;
            if (delayMs > 500) delayMs = 500;
        }

        // Ставим курсор в центр экрана (перерисовываем ТОЛЬКО эту строку)
        setCursorPosition(startX, startY);

        // Рисуем сам прогресс-бар
        std::cout << "[";
        int filled = (barWidth * progress) / 100; // Сколько символов закрасить
        for (int i = 0; i < barWidth; i++) {
            if (i < filled) std::cout << "#";
            else std::cout << "-";
        }
        std::cout << "] " << progress << "%   "; // Пробелы в конце затирают старые цифры (например, при переходе с 100% на 0%)

        // Если не на паузе — двигаем прогресс вперед
        if (!isPaused) {
            progress++;
            if (progress > 100) {
                progress = 0;
            }
        }

        // Ждем перед следующим кадром
        Sleep(delayMs);
    }

    // Очищаем экран перед выходом
    system("cls");
    return 0;
}
