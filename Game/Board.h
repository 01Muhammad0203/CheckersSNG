#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm> // Добавлен для std::min/max

#include "../Models/Move.h"
#include "../Models/Project_path.h"

// Условная компиляция для подключения SDL2:
// macOS использует иную структуру каталогов для заголовков SDL2.
#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif

using namespace std;

class Board
{
public:
    Board() = default;
    /**
     * @brief Конструктор Board с заданными размерами окна.
     * @param W Ширина окна.
     * @param H Высота окна.
     */
    Board(const unsigned int W, const unsigned int H) : W(W), H(H)
    {
    }

    // draws start board
    /**
     * @brief Инициализирует графическую подсистему (SDL), создает окно/рендерер
     * и загружает все текстуры.
     * @return int: 0 в случае успеха, 1 в случае ошибки инициализации/загрузки.
     */
    int start_draw()
    {
        // Инициализация всех подсистем SDL.
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        {
            print_exception("SDL_Init can't init SDL2 lib");
            return 1;
        }
        // Если размеры окна не заданы (W=0 или H=0), используем размер экрана.
        if (W == 0 || H == 0)
        {
            SDL_DisplayMode dm;
            if (SDL_GetDesktopDisplayMode(0, &dm))
            {
                print_exception("SDL_GetDesktopDisplayMode can't get desctop display mode");
                return 1;
            }
            // Установка квадратного окна, основанного на меньшем измерении экрана, с небольшим отступом.
            W = min(dm.w, dm.h);
            W -= W / 15;
            H = W;
        }
        // Создание окна.
        win = SDL_CreateWindow("Checkers", 0, H / 30, W, H, SDL_WINDOW_RESIZABLE);
        if (win == nullptr)
        {
            print_exception("SDL_CreateWindow can't create window");
            return 1;
        }
        // Создание рендерера с аппаратным ускорением и вертикальной синхронизацией.
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ren == nullptr)
        {
            print_exception("SDL_CreateRenderer can't create renderer");
            return 1;
        }
        // Загрузка всех необходимых текстур (доска, шашки, дамки, кнопки).
        board = IMG_LoadTexture(ren, board_path.c_str());
        w_piece = IMG_LoadTexture(ren, piece_white_path.c_str());
        b_piece = IMG_LoadTexture(ren, piece_black_path.c_str());
        w_queen = IMG_LoadTexture(ren, queen_white_path.c_str());
        b_queen = IMG_LoadTexture(ren, queen_black_path.c_str());
        back = IMG_LoadTexture(ren, back_path.c_str());
        replay = IMG_LoadTexture(ren, replay_path.c_str());
        if (!board || !w_piece || !b_piece || !w_queen || !b_queen || !back || !replay)
        {
            print_exception("IMG_LoadTexture can't load main textures from " + textures_path);
            return 1;
        }
        // Перечитываем актуальные размеры после создания рендерера.
        SDL_GetRendererOutputSize(ren, &W, &H);
        // Инициализация игрового поля и его отрисовка.
        make_start_mtx();
        rerender();
        return 0;
    }

    /**
     * @brief Сбрасывает состояние доски для начала новой игры (перезапуск).
     */
    void redraw()
    {
        game_results = -1; // Сброс флага результата игры.
        history_mtx.clear(); // Очистка истории состояний доски.
        history_beat_series.clear(); // Очистка истории серий взятий.
        make_start_mtx(); // Установка начальной расстановки шашек.
        clear_active(); // Сброс активной (выбранной) шашки.
        clear_highlight(); // Сброс подсвеченных клеток.
    }

    // --- Функции для выполнения хода ---

    /**
     * @brief Перемещает шашку, обрабатывая взятие (если есть).
     * @param turn Структура хода, включающая координаты битой шашки (xb, yb).
     * @param beat_series Количество взятий в текущей серии (0, если это не серия).
     */
    void move_piece(move_pos turn, const int beat_series = 0)
    {
        if (turn.xb != -1) // Если есть битая шашка (координаты != -1)
        {
            mtx[turn.xb][turn.yb] = 0; // Удаляем битую шашку с доски.
        }
        move_piece(turn.x, turn.y, turn.x2, turn.y2, beat_series); // Вызов основного перемещения.
    }

    /**
     * @brief Основная функция для перемещения шашки и проверки превращения в дамку.
     * @param i Начальная X-координата.
     * @param j Начальная Y-координата.
     * @param i2 Конечная X-координата.
     * @param j2 Конечная Y-координата.
     * @param beat_series Количество взятий в текущей серии.
     */
    void move_piece(const POS_T i, const POS_T j, const POS_T i2, const POS_T j2, const int beat_series = 0)
    {
        // Проверка на ошибки: конечная позиция не должна быть занята.
        if (mtx[i2][j2])
        {
            throw runtime_error("final position is not empty, can't move");
        }
        // Проверка на ошибки: начальная позиция не должна быть пуста.
        if (!mtx[i][j])
        {
            throw runtime_error("begin position is empty, can't move");
        }
        // Проверка и выполнение превращения в дамку:
        // Белая шашка (1) достигает 0-й строки ИЛИ Черная шашка (2) достигает 7-й строки.
        if ((mtx[i][j] == 1 && i2 == 0) || (mtx[i][j] == 2 && i2 == 7))
            mtx[i][j] += 2; // Тип шашки меняется (1->3, 2->4).

        mtx[i2][j2] = mtx[i][j]; // Перемещаем шашку на конечную позицию.
        drop_piece(i, j); // Очищаем начальную позицию и перерисовываем.
        add_history(beat_series); // Сохраняем состояние доски в историю.
    }

    /**
     * @brief Удаляет шашку с заданной позиции и перерисовывает доску.
     * @param i X-координата.
     * @param j Y-координата.
     */
    void drop_piece(const POS_T i, const POS_T j)
    {
        mtx[i][j] = 0;
        rerender();
    }

    /**
     * @brief Принудительно превращает шашку в дамку (для отладки/специальных режимов).
     * @param i X-координата.
     * @param j Y-координата.
     */
    void turn_into_queen(const POS_T i, const POS_T j)
    {
        if (mtx[i][j] == 0 || mtx[i][j] > 2)
        {
            throw runtime_error("can't turn into queen in this position");
        }
        mtx[i][j] += 2; // Увеличиваем тип шашки: 1->3, 2->4.
        rerender();
    }

    /**
     * @brief Возвращает текущее состояние доски.
     * @return vector<vector<POS_T>>: Копия матрицы доски.
     */
    vector<vector<POS_T>> get_board() const
    {
        return mtx;
    }

    // --- Функции управления отрисовкой ---

    /**
     * @brief Подсвечивает клетки, доступные для хода/взятия.
     * @param cells Вектор пар (X, Y) координат для подсветки.
     */
    void highlight_cells(vector<pair<POS_T, POS_T>> cells)
    {
        for (auto pos : cells)
        {
            POS_T x = pos.first, y = pos.second;
            is_highlighted_[x][y] = 1; // Устанавливаем флаг подсветки.
        }
        rerender();
    }

    /**
     * @brief Снимает подсветку со всех клеток.
     */
    void clear_highlight()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            is_highlighted_[i].assign(8, 0);
        }
        rerender();
    }

    /**
     * @brief Устанавливает выбранную (активную) шашку для хода.
     * @param x X-координата.
     * @param y Y-координата.
     */
    void set_active(const POS_T x, const POS_T y)
    {
        active_x = x;
        active_y = y;
        rerender();
    }

    /**
     * @brief Сбрасывает активную шашку.
     */
    void clear_active()
    {
        active_x = -1;
        active_y = -1;
        rerender();
    }

    /**
     * @brief Проверяет, подсвечена ли данная клетка.
     * @param x X-координата.
     * @param y Y-координата.
     * @return bool: true, если клетка подсвечена.
     */
    bool is_highlighted(const POS_T x, const POS_T y)
    {
        return is_highlighted_[x][y];
    }

    // --- Функции истории и управления игрой ---

    /**
     * @brief Откатывает ход на одно полное действие, включая всю серию взятий, если таковая была.
     * Откатывает до состояния, предшествующего последнему ходу.
     */
    void rollback()
    {
        // Определяем, сколько шагов нужно откатить.
        // max(1, ...) гарантирует, что даже обычный ход (beat_series=0) откатится на 1 шаг.
        auto beat_series = max(1, *(history_beat_series.rbegin()));

        // Откатываем все шаги серии, пока не вернемся к предыдущему полному ходу.
        while (beat_series-- && history_mtx.size() > 1)
        {
            history_mtx.pop_back();
            history_beat_series.pop_back();
        }
        // Восстанавливаем состояние доски из истории (последнее сохраненное).
        mtx = *(history_mtx.rbegin());
        clear_highlight();
        clear_active();
    }

    /**
     * @brief Устанавливает результат игры для отображения финального экрана.
     * @param res Результат игры (-1: игра продолжается, 0: ничья, 1: победа белых, 2: победа черных).
     */
    void show_final(const int res)
    {
        game_results = res;
        rerender();
    }

    // use if window size changed
    /**
     * @brief Обновляет размеры окна и перерисовывает доску после изменения размера окна.
     */
    void reset_window_size()
    {
        SDL_GetRendererOutputSize(ren, &W, &H);
        rerender();
    }

    /**
     * @brief Освобождает все ресурсы SDL (текстуры, рендерер, окно) и завершает работу SDL.
     */
    void quit()
    {
        SDL_DestroyTexture(board);
        SDL_DestroyTexture(w_piece);
        SDL_DestroyTexture(b_piece);
        SDL_DestroyTexture(w_queen);
        SDL_DestroyTexture(b_queen);
        SDL_DestroyTexture(back);
        SDL_DestroyTexture(replay);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }

    /**
     * @brief Деструктор: вызывает quit() для корректного освобождения ресурсов SDL.
     */
    ~Board()
    {
        if (win)
            quit();
    }

private:
    /**
     * @brief Добавляет текущее состояние доски и информацию о серии взятий в историю.
     * @param beat_series Количество взятий в текущей серии.
     */
    void add_history(const int beat_series = 0)
    {
        history_mtx.push_back(mtx); // Сохранение матрицы доски.
        history_beat_series.push_back(beat_series); // Сохранение статуса серии взятий.
    }
    // function to make start matrix
    /**
     * @brief Инициализирует матрицу доски mtx в начальное положение.
     * Шашки ставятся на черные клетки (i+j нечетно) в первых трех и последних трех рядах.
     */
    void make_start_mtx()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                mtx[i][j] = 0;
                // Черные шашки (2) в рядах 0-2 на черных клетках.
                if (i < 3 && (i + j) % 2 == 1)
                    mtx[i][j] = 2;
                // Белые шашки (1) в рядах 5-7 на черных клетках.
                if (i > 4 && (i + j) % 2 == 1)
                    mtx[i][j] = 1;
            }
        }
        add_history(); // Сохраняем начальное состояние.
    }

    // function that re-draw all the textures
    /**
     * @brief Главная функция отрисовки. Полностью перерисовывает все элементы на экране.
     */
    void rerender()
    {
        // 1. draw board
        SDL_RenderClear(ren); // Очистка рендерера.
        SDL_RenderCopy(ren, board, NULL, NULL); // Отрисовка текстуры доски на весь экран.

        // 2. draw pieces
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!mtx[i][j])
                    continue;
                // Вычисление координат для отрисовки шашки.
                int wpos = W * (j + 1) / 10 + W / 120; // x-координата на экране.
                int hpos = H * (i + 1) / 10 + H / 120; // y-координата на экране.
                SDL_Rect rect{ wpos, hpos, W / 12, H / 12 }; // Целевой прямоугольник.

                SDL_Texture* piece_texture;
                // Выбор текстуры в зависимости от типа шашки (1-белая, 2-черная, 3-белая дамка, 4-черная дамка).
                if (mtx[i][j] == 1)
                    piece_texture = w_piece;
                else if (mtx[i][j] == 2)
                    piece_texture = b_piece;
                else if (mtx[i][j] == 3)
                    piece_texture = w_queen;
                else
                    piece_texture = b_queen;

                SDL_RenderCopy(ren, piece_texture, NULL, &rect); // Отрисовка шашки.
            }
        }

        // 3. draw hilight
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 0); // Установка зеленого цвета для подсветки.
        const double scale = 2.5; // Коэффициент для уменьшения толщины рамки подсветки.
        SDL_RenderSetScale(ren, scale, scale); // Установка масштаба для отрисовки тонких линий.
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!is_highlighted_[i][j])
                    continue;
                // Вычисление прямоугольника для подсвеченной клетки (с учетом масштаба).
                SDL_Rect cell{ int(W * (j + 1) / 10 / scale), int(H * (i + 1) / 10 / scale), int(W / 10 / scale),
                              int(H / 10 / scale) };
                SDL_RenderDrawRect(ren, &cell); // Отрисовка рамки.
            }
        }

        // 4. draw active
        if (active_x != -1) // Если есть активная шашка.
        {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 0); // Установка красного цвета для активной шашки.
            // Вычисление прямоугольника для активной клетки.
            SDL_Rect active_cell{ int(W * (active_y + 1) / 10 / scale), int(H * (active_x + 1) / 10 / scale),
                                 int(W / 10 / scale), int(H / 10 / scale) };
            SDL_RenderDrawRect(ren, &active_cell); // Отрисовка рамки.
        }
        SDL_RenderSetScale(ren, 1, 1); // Сброс масштаба.

        // 5. draw arrows (кнопки "Отменить" и "Перезапуск")
        SDL_Rect rect_left{ W / 40, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, back, NULL, &rect_left);
        SDL_Rect replay_rect{ W * 109 / 120, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, replay, NULL, &replay_rect);

        // 6. draw result (финальный экран)
        if (game_results != -1) // Если игра завершена.
        {
            string result_path = draw_path;
            if (game_results == 1) // 1: победа белых.
                result_path = white_path;
            else if (game_results == 2) // 2: победа черных.
                result_path = black_path;
            // Загрузка текстуры с результатом (победа/ничья).
            SDL_Texture* result_texture = IMG_LoadTexture(ren, result_path.c_str());
            if (result_texture == nullptr)
            {
                print_exception("IMG_LoadTexture can't load game result picture from " + result_path);
                return;
            }
            SDL_Rect res_rect{ W / 5, H * 3 / 10, W * 3 / 5, H * 2 / 5 };
            SDL_RenderCopy(ren, result_texture, NULL, &res_rect); // Отрисовка финального экрана.
            SDL_DestroyTexture(result_texture); // Освобождение временной текстуры.
        }

        SDL_RenderPresent(ren); // Вывод отрисованного кадра на экран.
        // next rows for mac os
        // Небольшая задержка и обработка событий для корректного отображения на macOS.
        SDL_Delay(10);
        SDL_Event windowEvent;
        SDL_PollEvent(&windowEvent);
    }

    /**
     * @brief Записывает сообщение об ошибке в файл журнала (log.txt).
     * @param text Сообщение об ошибке.
     */
    void print_exception(const string& text) {
        ofstream fout(project_path + "log.txt", ios_base::app); // Открытие файла в режиме добавления.
        fout << "Error: " << text << ". " << SDL_GetError() << endl; // Запись ошибки SDL.
        fout.close();
    }

public:
    /**
     * @brief Ширина окна (публичное поле).
     */
    int W = 0;
    /**
     * @brief Высота окна (публичное поле).
     */
    int H = 0;
    // history of boards
    /**
     * @brief История состояний доски. Вектор векторов матриц.
     */
    vector<vector<vector<POS_T>>> history_mtx;

private:
    /**
     * @brief Указатель на окно SDL.
     */
    SDL_Window* win = nullptr;
    /**
     * @brief Указатель на рендерер SDL.
     */
    SDL_Renderer* ren = nullptr;
    // textures (Указатели на загруженные текстуры SDL)
    SDL_Texture* board = nullptr;
    SDL_Texture* w_piece = nullptr;
    SDL_Texture* b_piece = nullptr;
    SDL_Texture* w_queen = nullptr;
    SDL_Texture* b_queen = nullptr;
    SDL_Texture* back = nullptr;
    SDL_Texture* replay = nullptr;
    // texture files names (Пути к файлам текстур)
    const string textures_path = project_path + "Textures/";
    const string board_path = textures_path + "board.png";
    const string piece_white_path = textures_path + "piece_white.png";
    const string piece_black_path = textures_path + "piece_black.png";
    const string queen_white_path = textures_path + "queen_white.png";
    const string queen_black_path = textures_path + "queen_black.png";
    const string white_path = textures_path + "white_wins.png";
    const string black_path = textures_path + "black_wins.png";
    const string draw_path = textures_path + "draw.png";
    const string back_path = textures_path + "back.png";
    const string replay_path = textures_path + "replay.png";
    // coordinates of chosen cell
    /**
     * @brief Координаты (X, Y) активной (выбранной) шашки. -1, если нет активной шашки.
     */
    int active_x = -1, active_y = -1;
    // game result if exist
    /**
     * @brief Код результата игры: -1 (в игре), 0 (ничья), 1 (белые выиграли), 2 (черные выиграли).
     */
    int game_results = -1;
    // matrix of possible moves
    /**
     * @brief Матрица флагов, указывающая, должна ли клетка быть подсвечена (доступный ход).
     */
    vector<vector<bool>> is_highlighted_ = vector<vector<bool>>(8, vector<bool>(8, 0));
    // matrix of possible moves
    // 1 - white, 2 - black, 3 - white queen, 4 - black queen
    /**
     * @brief Матрица текущего состояния доски: 0 - пусто, 1-4 - типы шашек.
     */
    vector<vector<POS_T>> mtx = vector<vector<POS_T>>(8, vector<POS_T>(8, 0));
    // series of beats for each move
    /**
     * @brief История серий взятий: количество взятий, выполненных в последнем шаге.
     * Используется для корректного отката хода.
     */
    vector<int> history_beat_series;
};
