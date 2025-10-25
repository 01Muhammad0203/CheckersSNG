#pragma once
#include <random>
#include <vector>
#include <algorithm> // Добавлен для std::max/min

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

// Константа, представляющая бесконечность, используется для оценки выигрышных/проигрышных позиций.
const int INF = 1e9;

class Logic
{
public:
    /**
     * @brief Список всех найденных возможных ходов для текущего игрока/шашки.
     */
    vector<move_pos> turns;
    /**
     * @brief Флаг, указывающий, есть ли среди найденных ходов обязательные взятия.
     */
    bool have_beats;
    /**
     * @brief Максимальная глубина рекурсивного поиска (Minimax/Alpha-Beta) для бота.
     * Устанавливается в Game::play() на основе настроек.
     */
    int Max_depth;

    /**
         * @brief Конструктор класса Logic.
         * @param board Указатель на объект Board (текущее состояние доски).
         * @param config Указатель на объект Config (настройки игры).
         */
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        // Инициализация генератора случайных чисел.
        // Если "NoRandom" не установлен, используется текущее время для seed.
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        // Получение режима оценки позиции (например, "NumberAndPotential").
        scoring_mode = (*config)("Bot", "BotScoringType");
        // Получение режима оптимизации (например, "O0" - без оптимизации, "AB" - Alpha-Beta).
        optimization = (*config)("Bot", "Optimization");
    }

    // --- Перегруженные функции find_turns() ---

    /**
     * @brief Ищет все возможные ходы для игрока заданного цвета на текущей игровой доске.
     * (Публичный интерфейс для Game::play() - ищет на Board::mtx)
     * @param color Цвет игрока (0 - белые, 1 - черные).
     */
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    /**
     * @brief Ищет все возможные ходы (включая продолжение серии взятий) для конкретной шашки на текущей доске.
     * (Публичный интерфейс для Game::play() - ищет на Board::mtx)
     * @param x Координата X (строка) шашки.
     * @param y Координата Y (столбец) шашки.
     */
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

    // --- Основная логика поиска ходов ---

private:
    /**
     * @brief Ищет все возможные ходы для игрока заданного цвета на заданной матрице доски.
     * Приоритет отдается взятиям. Если найдено хотя бы одно взятие, обычные ходы игнорируются.
     * @param color Цвет игрока (0 - белые, 1 - черные).
     * @param mtx Матрица доски, на которой ищется ход.
     */
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        // Перебор всех клеток доски
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                // Если на клетке стоит шашка нужного цвета
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx); // Ищем ходы для этой шашки

                    // Если сейчас найдено взятие, а раньше не было:
                    // очищаем ранее найденные ходы (т.к. взятие обязательно)
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }

                    // Добавляем найденные ходы:
                    // - либо если уже есть взятия (have_beats_before) и текущая шашка тоже может бить (have_beats)
                    // - либо если пока не найдено взятий (тогда добавляются обычные ходы)
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        // Добавление случайности: перемешиваем порядок ходов (для бота).
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    /**
     * @brief Ищет все возможные ходы для конкретной шашки на заданной матрице доски.
     * Сначала ищет взятия, если они есть, обычные ходы не ищутся.
     * @param x Координата X (строка) шашки.
     * @param y Координата Y (столбец) шашки.
     * @param mtx Матрица доски, на которой ищется ход.
     */
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // check beats
        switch (type)
        {
        case 1:
        case 2:
            // check pieces (шашки)
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    // Проверка на возможность взятия:
                    // 1. Конечная клетка (i, j) пуста (mtx[i][j] == 0)
                    // 2. Битая клетка (xb, yb) содержит шашку
                    // 3. Битая шашка другого цвета
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens (дамки) - поиск взятий по диагоналям
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1; // Координаты потенциально битой шашки
                    // Движение по диагонали
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2]) // Найдена шашка
                        {
                            // Если шашка своего цвета ИЛИ уже была бита одна шашка
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break; // Диагональ заблокирована
                            }
                            xb = i2; // Запоминаем битую шашку
                            yb = j2;
                        }
                        // Если есть битая шашка (xb != -1) И текущая клетка не является битой (xb != i2),
                        // значит, текущая клетка (i2, j2) — это пустая клетка, куда можно приземлиться после взятия.
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }

        // check other turns (поиск обычных ходов)
        if (!turns.empty())
        {
            have_beats = true; // Найдено взятие, обычные ходы не нужны.
            return;
        }

        switch (type)
        {
        case 1:
        case 2:
            // check pieces (шашки)
        {
            // Определяем направление хода: белые (1) идут вниз (+1), черные (2) идут вверх (-1).
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                // Проверка на выход за границы и занятость конечной клетки.
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                turns.emplace_back(x, y, i, j);
            }
            break;
        }
        default:
            // check queens (дамки) - обычный ход по диагоналям
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    // Движение по диагонали до конца или до первой шашки.
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break; // Доска заблокирована
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

    // --- Функции для логики бота ---

    /**
     * @brief Выполняет ход (шаг) на заданной матрице доски.
     * @param mtx Текущая матрица доски.
     * @param turn Описание хода (начальная/конечная/битая позиции).
     * @return vector<vector<POS_T>> Новое состояние доски после хода.
     */
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1) // Если есть битая шашка, удаляем ее.
            mtx[turn.xb][turn.yb] = 0;

        // Проверка на превращение в дамку
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2; // 1 -> 3 (белая дамка), 2 -> 4 (черная дамка)

        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // Перемещаем шашку на новую позицию.
        mtx[turn.x][turn.y] = 0; // Очищаем старую позицию.
        return mtx;
    }

    /**
     * @brief Оценивает текущую позицию на доске для Minimax алгоритма.
     * @param mtx Матрица доски для оценки.
     * @param first_bot_color Цвет игрока, для которого бот ищет максимум (Max-игрок).
     * @return double Оценка позиции. Большее значение лучше для Max-игрока.
     */
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0; // Счетчики для белых (w), белых дамок (wq), черных (b), черных дамок (bq).
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);

                // Дополнительный скоринг за потенциал (продвижение шашек)
                if (scoring_mode == "NumberAndPotential")
                {
                    // Белые (1) идут к 0-й строке (7-i). Чем меньше i, тем ближе к дамке.
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    // Черные (2) идут к 7-й строке (i). Чем больше i, тем ближе к дамке.
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }

        // Нормализация: Max-игрок всегда "черные" (b, bq) для простоты расчетов.
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }

        // Условие победы/поражения
        if (w + wq == 0) // Min-игрок проиграл
            return INF;
        if (b + bq == 0) // Max-игрок проиграл
            return 0;

        int q_coef = 4; // Коэффициент ценности дамки
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        // Возвращаем отношение силы Max-игрока к силе Min-игрока.
        // Оценка > 1.0 в пользу Max-игрока, < 1.0 в пользу Min-игрока.
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

public:
    /**
     * @brief Запускает поиск лучшей серии ходов для бота.
     * Использует find_first_best_turn для обработки начальной серии взятий.
     * @param color Цвет бота (Max-игрок).
     * @return vector<move_pos> Вектор шагов, составляющих лучший ход (серию взятий).
     */
    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear();
        next_move.clear();

        // Запускаем рекурсивный поиск с начальным состоянием 0.
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        // Восстановление лучшего хода по сохраненному пути (next_best_state).
        int cur_state = 0;
        vector<move_pos> res;
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];
        } while (cur_state != -1 && next_move[cur_state].x != -1);
        return res;
    }

private:
    /**
     * @brief Рекурсивная функция для нахождения лучшего первого хода или серии взятий.
     * Эта функция обрабатывает обязательные серии взятий, где глубина поиска не меняется,
     * а затем переходит к find_best_turns_rec.
     * @param mtx Текущая матрица доски.
     * @param color Цвет текущего игрока.
     * @param x X-координата шашки, которая бьет (или -1 для обычного хода).
     * @param y Y-координата шашки, которая бьет (или -1 для обычного хода).
     * @param state Индекс текущего состояния в массивах next_move/next_best_state.
     * @param alpha Лучший счет, найденный на предыдущих уровнях (для отсечения).
     * @return double Лучшая оценка для Max-игрока (бота).
     */
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {
        // Добавление текущего состояния в массивы для отслеживания пути.
        next_best_state.push_back(-1);
        next_move.emplace_back(-1, -1, -1, -1);
        double best_score = -1;

        // 1. Поиск возможных ходов
        if (state != 0) // Если это продолжение серии взятий (state != 0), ищем только для одной шашки.
            find_turns(x, y, mtx);
        else // Если это первый шаг хода, ищем для всех шашек игрока.
            find_turns(color, mtx);

        auto turns_now = turns;
        bool have_beats_now = have_beats;

        // 2. Условие окончания серии взятий
        if (!have_beats_now && state != 0)
        {
            // Если шашка перестала бить, передаем управление find_best_turns_rec, 
            // которая начнет отсчет глубины для Minimax-поиска.
            return find_best_turns_rec(mtx, 1 - color, 1, alpha, INF + 1);
        }

        if (turns_now.empty()) // Позиция - проигрыш (нет ходов)
            return 0; // Оценка минимальна для Max-игрока.

        // 3. Перебор и оценка ходов
        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;

            if (have_beats_now) // Если это продолжение серии взятий
            {
                // Рекурсивный вызов find_first_best_turn: глубина Minimax НЕ меняется.
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else // Если это обычный первый ход (не взятие)
            {
                // Переход к find_best_turns_rec: глубина Minimax меняется (depth = 1).
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 1, best_score, INF + 1);
            }

            // 4. Обновление лучшего счета и сохранение пути
            if (score > best_score)
            {
                best_score = score;
                // Сохраняем, куда ведет лучший ход: к следующему шагу серии (next_state) или к концу серии (-1).
                next_best_state[state] = (have_beats_now ? int(next_state) : -1);
                next_move[state] = turn;
                // Alpha-Beta отсечение для первого уровня
                if (optimization != "O0" && best_score >= INF) // Если найдена победа, можно прекратить поиск.
                    return INF;
            }
        }
        return best_score;
    }

    /**
     * @brief Рекурсивный поиск с использованием Minimax и Alpha-Beta отсечения.
     * @param mtx Текущая матрица доски.
     * @param color Цвет игрока, чей ход оценивается.
     * @param depth Текущая глубина поиска (начинается с 1 после первого хода).
     * @param alpha Лучший (наибольший) счет, который Max-игрок может гарантировать.
     * @param beta Худший (наименьший) счет, который Min-игрок может гарантировать.
     * @param x X-координата шашки, которая бьет (для продолжения серии).
     * @param y Y-координата шашки, которая бьет (для продолжения серии).
     * @return double Оценка позиции.
     */
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        // 1. Базовый случай: Достигнута максимальная глубина.
        if (depth >= Max_depth)
        {
            // Оцениваем позицию. first_bot_color всегда Max-игрок.
            return calc_score(mtx, (Max_depth % 2 != color));
        }

        // 2. Поиск возможных ходов
        if (x != -1) // Если это продолжение серии взятий (после предыдущего хода).
        {
            find_turns(x, y, mtx);
        }
        else // Обычный ход: ищем для всех шашек игрока.
            find_turns(color, mtx);

        auto turns_now = turns;
        bool have_beats_now = have_beats;

        // 3. Обработка обязательной серии взятий.
        if (!have_beats_now && x != -1)
        {
            // Если шашка перестала бить, но мы были в серии (x!=-1), 
            // передаем ход другому игроку и увеличиваем глубину.
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        // 4. Базовый случай: нет ходов (проигрыш).
        if (turns_now.empty())
            // Если нет ходов: Max-игрок (depth % 2 == 1) проигрывает (оценка 0). 
            // Min-игрок (depth % 2 == 0) проигрывает (оценка INF, т.к. Min-игрок хочет минимизировать).
            return (depth % 2 ? 0 : INF);

        double min_score = INF + 1; // Используется для Min-игрока (Minimax).
        double max_score = -1; // Используется для Max-игрока (Minimax).

        // 5. Рекурсивный перебор всех возможных ходов.
        for (auto turn : turns_now)
        {
            double score = 0.0;

            if (!have_beats_now && x == -1) // Обычный ход (не взятие и не продолжение серии).
            {
                // Передача хода другому игроку и увеличение глубины.
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else // Взятие или продолжение серии взятий.
            {
                // Ход остается тому же игроку (color) и глубина НЕ меняется.
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            // Обновление Minimax счета
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // 6. Alpha-Beta отсечение
            if (depth % 2) // Max-уровень (нечетная глубина, ищем максимум).
                alpha = max(alpha, max_score);
            else // Min-уровень (четная глубина, ищем минимум).
                beta = min(beta, min_score);

            if (optimization != "O0" && alpha >= beta)
            {
                // Отсечение: если alpha >= beta, мы нашли ход, который Min-игрок никогда не допустит 
                // (или Max-игрок никогда не допустит), и ветвь можно отсечь.
                return (depth % 2 ? max_score : min_score); // Возвращаем текущий лучший/худший счет.
            }
        }

        // Возвращаем результат Minimax: максимум на Max-уровне, минимум на Min-уровне.
        return (depth % 2 ? max_score : min_score);
    }

private:
    // --- Приватные поля класса ---
    /**
     * @brief Генератор случайных чисел, используется для перемешивания ходов.
     */
    default_random_engine rand_eng;
    /**
     * @brief Режим оценки позиции, заданный в конфигурации (например, "NumberAndPotential").
     */
    string scoring_mode;
    /**
     * @brief Режим оптимизации поиска, заданный в конфигурации (например, "O0", "AB").
     */
    string optimization;
    /**
     * @brief Вектор для хранения лучшего хода (первого шага в серии) из каждого состояния (узла) в дереве поиска.
     */
    vector<move_pos> next_move;
    /**
     * @brief Вектор для хранения индекса следующего состояния в серии взятий.
     * Используется вместе с next_move для восстановления лучшего пути (серии).
     */
    vector<int> next_best_state;
    /**
     * @brief Указатель на текущую игровую доску.
     */
    Board* board;
    /**
     * @brief Указатель на настройки игры.
     */
    Config* config;
};