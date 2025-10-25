#pragma once
#include <stdlib.h>

// ќпределение типа дл€ координат на доске. »спользуетс€ int8_t дл€ экономии пам€ти, так как координаты 0-7.
typedef int8_t POS_T;

// —труктура дл€ хранени€ данных одного хода.
struct move_pos
{
    POS_T x, y;// »сходные координаты шашки (откуда ходим).
    POS_T x2, y2;//  онечные координаты шашки (куда ходим).
    //  оординаты побитой шашки. '-1' означает, что вз€ти€ не было.
    POS_T xb = -1, yb = -1;

    //  онструктор дл€ обычного хода (без вз€ти€).
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    //  онструктор дл€ хода со вз€тием шашки.
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // ѕерегрузка оператора '==' дл€ сравнени€ двух ходов. —равнивает только начальные и конечные клетки.
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    // ѕерегрузка оператора '!='.
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};