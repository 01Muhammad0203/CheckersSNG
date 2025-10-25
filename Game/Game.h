#pragma once
#include <chrono>
#include <thread>
#include <fstream> // Добавлен, т.к. используется ofstream

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        // Очистка файла журнала (log.txt) при старте новой игры.
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    /**
     * @brief Главный цикл игры. Управляет сменой ходов, логикой конца игры и перезапуском.
     * @return int: 0 - выход/ничья, 1 - победа черных, 2 - победа белых.
     */
    int play()
    {
        auto start = chrono::steady_clock::now();// Запоминаем время начала игры.

        // Логика перезапуска/первого запуска.
        if (is_replay)
        {
            logic = Logic(&board, &config);// Пересоздаем Logic для сброса состояния игры.
            config.reload();// Перезагружаем настройки.
            board.redraw();// Перерисовываем доску с новым состоянием.
        }
        else
        {
            board.start_draw();// Первый запуск: инициализация отрисовки.
        }
        is_replay = false;// Сбрасываем флаг перезапуска.

        int turn_num = -1;
        bool is_quit = false;
        const int Max_turns = config("Game", "MaxNumTurns");// Получаем лимит ходов из настроек.
        while (++turn_num < Max_turns) // Главный игровой цикл.
        {
            beat_series = 0;// Сброс счетчика серии взятий в начале хода.
            logic.find_turns(turn_num % 2);// Поиск всех возможных ходов для текущего игрока (0/1).

            if (logic.turns.empty())// Условие конца игры: если возможных ходов нет.
                break;

            // Установка глубины поиска для бота на основе настроек.
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));

            // Проверка, является ли текущий игрок человеком.
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                auto resp = player_turn(turn_num % 2);// Ход человека: ожидание и обработка ввода.
                if (resp == Response::QUIT)// Обработка команды QUIT.
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)// Обработка команды REPLAY.
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)// Обработка команды BACK (откат).
                {
                    // Откат хода бота (если предыдущий был ботом, и не было серии взятий).
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();// Откатываем ход бота.
                        --turn_num;// Уменьшаем счетчик, чтобы следующим ходил бот.
                    }
                    // Дополнительное уменьшение счетчика, если не было серии взятий (для отката хода человека).
                    if (!beat_series)
                        --turn_num;

                    board.rollback();// Откатываем ход текущего игрока.
                    --turn_num;// Уменьшаем счетчик для повторного хода текущего игрока.
                    beat_series = 0;// Сбрасываем серию взятий.
                }
            }
            else
                bot_turn(turn_num % 2);// Ход бота: вычисление и выполнение хода.
        }

        // Логика завершения игры.
        auto end = chrono::steady_clock::now();
        // Запись времени игры в лог-файл.
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        if (is_replay)// Рекурсивный вызов play() для перезапуска.
            return play();
        if (is_quit)
            return 0;// Выход из программы.

        int res = 2;// 2 - победа белых (по умолчанию).
        if (turn_num == Max_turns)// Ничья по лимиту ходов.
        {
            res = 0;
        }
        else if (turn_num % 2)// Победа черных (если прервано на ходе белых - 0, т.е. 0 % 2 == 0).
        {
            res = 1;
        }
        board.show_final(res);// Отображение финального экрана.
        auto resp = hand.wait();// Ожидание команды REPLAY/QUIT после конца игры.

        if (resp == Response::REPLAY)// Обработка команды REPLAY.
        {
            is_replay = true;
            return play();// Рекурсивный вызов.
        }
        return res;// Возврат финального результата.
    }

private:
    /**
     * @brief Выполняет ход бота: ищет лучший ход, применяет задержку и совершает серию ходов.
     * @param color Цвет игрока (0 - белые, 1 - черные).
     */
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now();// Запоминаем время начала хода.

        auto delay_ms = config("Bot", "BotDelayMS");// Получаем минимальную задержку из настроек.
        // Запускаем отдельный поток для задержки, чтобы обеспечить минимальное время хода,
        // даже если поиск хода завершился быстро.
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color);// Запускаем поиск лучшего хода (может быть серией).
        th.join();// Ожидаем завершения задержки (минимум delay_ms).

        bool is_first = true;
        // making moves
        // Выполнение серии ходов (если есть взятие, то это несколько шагов).
        for (auto turn : turns)
        {
            if (!is_first)// Добавляем задержку между отдельными шагами серии взятий.
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1);// Увеличиваем счетчик серии, если было взятие.
            board.move_piece(turn, beat_series);// Выполняем шаг хода/взятия.
        }

        auto end = chrono::steady_clock::now();
        // Запись времени хода бота в лог-файл.
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // return 1 if quit
        vector<pair<POS_T, POS_T>> cells;
        // Собираем координаты всех шашек, которыми можно начать ход.
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);// Подсвечиваем доступные для хода шашки.
        move_pos pos = { -1, -1, -1, -1 };// Переменная для хранения сделанного хода.
        POS_T x = -1, y = -1;// Координаты выбранной шашки (начальная позиция).
        // trying to make first move
        while (true) // Цикл выбора начальной и конечной клетки.
        {
            auto resp = hand.get_cell();// Ожидаем клика или команды.
            if (get<0>(resp) != Response::CELL)// Если получена команда (QUIT, BACK, REPLAY), возвращаем ее.
                return get<0>(resp);
            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };// Координаты клика.

            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                // 1. Проверяем, кликнул ли игрок на шашку, доступную для начала хода.
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                // 2. Проверяем, кликнул ли игрок на конечную клетку (завершение хода).
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;// Ход определен.
                    break;
                }
            }

            if (pos.x != -1)// Если ход полностью определен, выходим из цикла.
                break;

            if (!is_correct)// Если клик не является корректной шашкой для начала хода.
            {
                if (x != -1)// Отменяем предыдущий выбор.
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;// Сброс выбранной шашки.
                y = -1;
                continue;
            }

            // Шашка выбрана (первый клик).
            x = cell.first;
            y = cell.second;
            board.clear_highlight();// Убираем подсветку всех начальных шашек.
            board.set_active(x, y);// Выделяем выбранную шашку.

            // Собираем и подсвечиваем все возможные конечные клетки для выбранной шашки.
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }

        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);// Выполняем первый шаг хода.

        if (pos.xb == -1)// Если не было взятия, ход завершен.
            return Response::OK;

        // continue beating while can
        // Логика для серии взятий (обязательное продолжение битья).
        beat_series = 1;
        while (true)
        {
            // Ищем возможные ходы (взятия) только для шашки, которая только что била.
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)// Если дальнейшее взятие невозможно, серия завершена.
                break;

            // Подсветка всех возможных конечных клеток для продолжения взятия.
            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);// Подсветка активной шашки.
            // trying to make move
            while (true) // Цикл ожидания клика для продолжения серии взятий.
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)// Обработка команд (QUIT/BACK/REPLAY).
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    // Проверка, является ли клик корректным продолжением взятия.
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;// Запоминаем новый шаг взятия.
                        break;
                    }
                }
                if (!is_correct)
                    continue;

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;// Увеличиваем счетчик серии взятий.
                board.move_piece(pos, beat_series);// Выполняем следующий шаг взятия.
                break;// Выход для проверки, можно ли бить дальше.
            }
        }

        return Response::OK;
    }

private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};