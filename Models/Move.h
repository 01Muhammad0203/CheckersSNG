#pragma once
#include <stdlib.h>

// Определение типа для координат на доске. Используется int8_t для экономии памяти, так как координаты 0-7.
typedef int8_t POS_T;

// Структура для хранения данных одного хода.
struct move_pos
{
    POS_T x, y;// Исходные координаты шашки (откуда ходим).
    POS_T x2, y2;// Конечные координаты шашки (куда ходим).
    // Координаты побитой шашки. '-1' означает, что взятия не было.
    POS_T xb = -1, yb = -1;

    // Конструктор для обычного хода (без взятия).
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    // Конструктор для хода со взятием шашки.
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Перегрузка оператора '==' для сравнения двух ходов. Сравнивает только начальные и конечные клетки.
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    // Перегрузка оператора '!='.
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};
