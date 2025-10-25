#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс Hand отвечает за обработку ввода пользователя (кликов мыши) и системных событий SDL.
class Hand
{
public:
    // Конструктор: сохраняет указатель на объект Board для доступа к размерам окна и истории ходов.
    Hand(Board* board) : board(board)
    {
    }
    /**
     * @brief Ожидает и обрабатывает одно событие, которое может быть кликом по клетке или командой.
     * @return tuple<Response, POS_T, POS_T>: тип ответа (CELL, QUIT и т.д.) и координаты клетки (xc, yc).
     */
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;// Координаты клика в пикселях.
        int xc = -1, yc = -1;// Координаты клетки (0-7), или -1 для системных зон.
        while (true) // Цикл ожидания события.
        {
            if (SDL_PollEvent(&windowEvent)) // Проверяем наличие событий.
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT;// Команда выхода из окна.
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    // Преобразование пиксельных координат в координаты клетки (0-7).
                    xc = int(y / (board->H / 10) - 1);// Координата строки (0-7).
                    yc = int(x / (board->W / 10) - 1);// Координата столбца (0-7).

                    // Условие для кнопки "Отменить ход" (BACK): зона (-1, -1) и наличие истории.
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;
                    }
                    // Условие для кнопки "Перезапуск" (REPLAY): зона (-1, 8).
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;
                    }
                    // Условие для клика по игровой клетке (0-7).
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL;
                    }
                    else
                    {
                        // Клик вне активных зон игнорируется.
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    // Обработка изменения размера окна (перерисовка).
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();
                        break;
                    }
                }
                // Выходим, если получена команда.
                if (resp != Response::OK)
                    break;
            }
        }
        // Возвращаем тип ответа и координаты.
        return { resp, xc, yc };
    }

    /**
     * @brief Ожидает команду QUIT или REPLAY (обычно после конца игры).
     * @return Response: QUIT или REPLAY.
     */
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true) // Цикл ожидания события.
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    // Преобразование клика в координаты (проверка системных зон).
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    // Проверка на клик по кнопке "Перезапуск" (REPLAY) в зоне (-1, 8).
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                                        break;
                }
                // Выходим, если получена команда.
                if (resp != Response::OK)
                    break;
            }
        }
        return resp; // Возвращаем команду.
    }

private:
    Board* board;// Указатель на Board для взаимодействия с доской и получения ее размеров.
};