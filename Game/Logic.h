#pragma once
#include <random>
#include <vector>
#include <algorithm> // �������� ��� std::max/min

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

// ���������, �������������� �������������, ������������ ��� ������ ����������/����������� �������.
const int INF = 1e9;

class Logic
{
public:
    /**
     * @brief ������ ���� ��������� ��������� ����� ��� �������� ������/�����.
     */
    vector<move_pos> turns;
    /**
     * @brief ����, �����������, ���� �� ����� ��������� ����� ������������ ������.
     */
    bool have_beats;
    /**
     * @brief ������������ ������� ������������ ������ (Minimax/Alpha-Beta) ��� ����.
     * ��������������� � Game::play() �� ������ ��������.
     */
    int Max_depth;

    /**
         * @brief ����������� ������ Logic.
         * @param board ��������� �� ������ Board (������� ��������� �����).
         * @param config ��������� �� ������ Config (��������� ����).
         */
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        // ������������� ���������� ��������� �����.
        // ���� "NoRandom" �� ����������, ������������ ������� ����� ��� seed.
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        // ��������� ������ ������ ������� (��������, "NumberAndPotential").
        scoring_mode = (*config)("Bot", "BotScoringType");
        // ��������� ������ ����������� (��������, "O0" - ��� �����������, "AB" - Alpha-Beta).
        optimization = (*config)("Bot", "Optimization");
    }

    // --- ������������� ������� find_turns() ---

    /**
     * @brief ���� ��� ��������� ���� ��� ������ ��������� ����� �� ������� ������� �����.
     * (��������� ��������� ��� Game::play() - ���� �� Board::mtx)
     * @param color ���� ������ (0 - �����, 1 - ������).
     */
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }

    /**
     * @brief ���� ��� ��������� ���� (������� ����������� ����� ������) ��� ���������� ����� �� ������� �����.
     * (��������� ��������� ��� Game::play() - ���� �� Board::mtx)
     * @param x ���������� X (������) �����.
     * @param y ���������� Y (�������) �����.
     */
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

    // --- �������� ������ ������ ����� ---

private:
    /**
     * @brief ���� ��� ��������� ���� ��� ������ ��������� ����� �� �������� ������� �����.
     * ��������� �������� �������. ���� ������� ���� �� ���� ������, ������� ���� ������������.
     * @param color ���� ������ (0 - �����, 1 - ������).
     * @param mtx ������� �����, �� ������� ������ ���.
     */
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        // ������� ���� ������ �����
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                // ���� �� ������ ����� ����� ������� �����
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx); // ���� ���� ��� ���� �����

                    // ���� ������ ������� ������, � ������ �� ����:
                    // ������� ����� ��������� ���� (�.�. ������ �����������)
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }

                    // ��������� ��������� ����:
                    // - ���� ���� ��� ���� ������ (have_beats_before) � ������� ����� ���� ����� ���� (have_beats)
                    // - ���� ���� ���� �� ������� ������ (����� ����������� ������� ����)
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        // ���������� �����������: ������������ ������� ����� (��� ����).
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    /**
     * @brief ���� ��� ��������� ���� ��� ���������� ����� �� �������� ������� �����.
     * ������� ���� ������, ���� ��� ����, ������� ���� �� ������.
     * @param x ���������� X (������) �����.
     * @param y ���������� Y (�������) �����.
     * @param mtx ������� �����, �� ������� ������ ���.
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
            // check pieces (�����)
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    // �������� �� ����������� ������:
                    // 1. �������� ������ (i, j) ����� (mtx[i][j] == 0)
                    // 2. ����� ������ (xb, yb) �������� �����
                    // 3. ����� ����� ������� �����
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens (�����) - ����� ������ �� ����������
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1; // ���������� ������������ ����� �����
                    // �������� �� ���������
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2]) // ������� �����
                        {
                            // ���� ����� ������ ����� ��� ��� ���� ���� ���� �����
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break; // ��������� �������������
                            }
                            xb = i2; // ���������� ����� �����
                            yb = j2;
                        }
                        // ���� ���� ����� ����� (xb != -1) � ������� ������ �� �������� ����� (xb != i2),
                        // ������, ������� ������ (i2, j2) � ��� ������ ������, ���� ����� ������������ ����� ������.
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }

        // check other turns (����� ������� �����)
        if (!turns.empty())
        {
            have_beats = true; // ������� ������, ������� ���� �� �����.
            return;
        }

        switch (type)
        {
        case 1:
        case 2:
            // check pieces (�����)
        {
            // ���������� ����������� ����: ����� (1) ���� ���� (+1), ������ (2) ���� ����� (-1).
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                // �������� �� ����� �� ������� � ��������� �������� ������.
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                    continue;
                turns.emplace_back(x, y, i, j);
            }
            break;
        }
        default:
            // check queens (�����) - ������� ��� �� ����������
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    // �������� �� ��������� �� ����� ��� �� ������ �����.
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break; // ����� �������������
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

    // --- ������� ��� ������ ���� ---

    /**
     * @brief ��������� ��� (���) �� �������� ������� �����.
     * @param mtx ������� ������� �����.
     * @param turn �������� ���� (���������/��������/����� �������).
     * @return vector<vector<POS_T>> ����� ��������� ����� ����� ����.
     */
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1) // ���� ���� ����� �����, ������� ��.
            mtx[turn.xb][turn.yb] = 0;

        // �������� �� ����������� � �����
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2; // 1 -> 3 (����� �����), 2 -> 4 (������ �����)

        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // ���������� ����� �� ����� �������.
        mtx[turn.x][turn.y] = 0; // ������� ������ �������.
        return mtx;
    }

    /**
     * @brief ��������� ������� ������� �� ����� ��� Minimax ���������.
     * @param mtx ������� ����� ��� ������.
     * @param first_bot_color ���� ������, ��� �������� ��� ���� �������� (Max-�����).
     * @return double ������ �������. ������� �������� ����� ��� Max-������.
     */
    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0; // �������� ��� ����� (w), ����� ����� (wq), ������ (b), ������ ����� (bq).
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);

                // �������������� ������� �� ��������� (����������� �����)
                if (scoring_mode == "NumberAndPotential")
                {
                    // ����� (1) ���� � 0-� ������ (7-i). ��� ������ i, ��� ����� � �����.
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    // ������ (2) ���� � 7-� ������ (i). ��� ������ i, ��� ����� � �����.
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }

        // ������������: Max-����� ������ "������" (b, bq) ��� �������� ��������.
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }

        // ������� ������/���������
        if (w + wq == 0) // Min-����� ��������
            return INF;
        if (b + bq == 0) // Max-����� ��������
            return 0;

        int q_coef = 4; // ����������� �������� �����
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        // ���������� ��������� ���� Max-������ � ���� Min-������.
        // ������ > 1.0 � ������ Max-������, < 1.0 � ������ Min-������.
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

public:
    /**
     * @brief ��������� ����� ������ ����� ����� ��� ����.
     * ���������� find_first_best_turn ��� ��������� ��������� ����� ������.
     * @param color ���� ���� (Max-�����).
     * @return vector<move_pos> ������ �����, ������������ ������ ��� (����� ������).
     */
    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear();
        next_move.clear();

        // ��������� ����������� ����� � ��������� ���������� 0.
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        // �������������� ������� ���� �� ������������ ���� (next_best_state).
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
     * @brief ����������� ������� ��� ���������� ������� ������� ���� ��� ����� ������.
     * ��� ������� ������������ ������������ ����� ������, ��� ������� ������ �� ��������,
     * � ����� ��������� � find_best_turns_rec.
     * @param mtx ������� ������� �����.
     * @param color ���� �������� ������.
     * @param x X-���������� �����, ������� ���� (��� -1 ��� �������� ����).
     * @param y Y-���������� �����, ������� ���� (��� -1 ��� �������� ����).
     * @param state ������ �������� ��������� � �������� next_move/next_best_state.
     * @param alpha ������ ����, ��������� �� ���������� ������� (��� ���������).
     * @return double ������ ������ ��� Max-������ (����).
     */
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {
        // ���������� �������� ��������� � ������� ��� ������������ ����.
        next_best_state.push_back(-1);
        next_move.emplace_back(-1, -1, -1, -1);
        double best_score = -1;

        // 1. ����� ��������� �����
        if (state != 0) // ���� ��� ����������� ����� ������ (state != 0), ���� ������ ��� ����� �����.
            find_turns(x, y, mtx);
        else // ���� ��� ������ ��� ����, ���� ��� ���� ����� ������.
            find_turns(color, mtx);

        auto turns_now = turns;
        bool have_beats_now = have_beats;

        // 2. ������� ��������� ����� ������
        if (!have_beats_now && state != 0)
        {
            // ���� ����� ��������� ����, �������� ���������� find_best_turns_rec, 
            // ������� ������ ������ ������� ��� Minimax-������.
            return find_best_turns_rec(mtx, 1 - color, 1, alpha, INF + 1);
        }

        if (turns_now.empty()) // ������� - �������� (��� �����)
            return 0; // ������ ���������� ��� Max-������.

        // 3. ������� � ������ �����
        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;

            if (have_beats_now) // ���� ��� ����������� ����� ������
            {
                // ����������� ����� find_first_best_turn: ������� Minimax �� ��������.
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else // ���� ��� ������� ������ ��� (�� ������)
            {
                // ������� � find_best_turns_rec: ������� Minimax �������� (depth = 1).
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 1, best_score, INF + 1);
            }

            // 4. ���������� ������� ����� � ���������� ����
            if (score > best_score)
            {
                best_score = score;
                // ���������, ���� ����� ������ ���: � ���������� ���� ����� (next_state) ��� � ����� ����� (-1).
                next_best_state[state] = (have_beats_now ? int(next_state) : -1);
                next_move[state] = turn;
                // Alpha-Beta ��������� ��� ������� ������
                if (optimization != "O0" && best_score >= INF) // ���� ������� ������, ����� ���������� �����.
                    return INF;
            }
        }
        return best_score;
    }

    /**
     * @brief ����������� ����� � �������������� Minimax � Alpha-Beta ���������.
     * @param mtx ������� ������� �����.
     * @param color ���� ������, ��� ��� �����������.
     * @param depth ������� ������� ������ (���������� � 1 ����� ������� ����).
     * @param alpha ������ (����������) ����, ������� Max-����� ����� �������������.
     * @param beta ������ (����������) ����, ������� Min-����� ����� �������������.
     * @param x X-���������� �����, ������� ���� (��� ����������� �����).
     * @param y Y-���������� �����, ������� ���� (��� ����������� �����).
     * @return double ������ �������.
     */
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1)
    {
        // 1. ������� ������: ���������� ������������ �������.
        if (depth >= Max_depth)
        {
            // ��������� �������. first_bot_color ������ Max-�����.
            return calc_score(mtx, (Max_depth % 2 != color));
        }

        // 2. ����� ��������� �����
        if (x != -1) // ���� ��� ����������� ����� ������ (����� ����������� ����).
        {
            find_turns(x, y, mtx);
        }
        else // ������� ���: ���� ��� ���� ����� ������.
            find_turns(color, mtx);

        auto turns_now = turns;
        bool have_beats_now = have_beats;

        // 3. ��������� ������������ ����� ������.
        if (!have_beats_now && x != -1)
        {
            // ���� ����� ��������� ����, �� �� ���� � ����� (x!=-1), 
            // �������� ��� ������� ������ � ����������� �������.
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        // 4. ������� ������: ��� ����� (��������).
        if (turns_now.empty())
            // ���� ��� �����: Max-����� (depth % 2 == 1) ����������� (������ 0). 
            // Min-����� (depth % 2 == 0) ����������� (������ INF, �.�. Min-����� ����� ��������������).
            return (depth % 2 ? 0 : INF);

        double min_score = INF + 1; // ������������ ��� Min-������ (Minimax).
        double max_score = -1; // ������������ ��� Max-������ (Minimax).

        // 5. ����������� ������� ���� ��������� �����.
        for (auto turn : turns_now)
        {
            double score = 0.0;

            if (!have_beats_now && x == -1) // ������� ��� (�� ������ � �� ����������� �����).
            {
                // �������� ���� ������� ������ � ���������� �������.
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else // ������ ��� ����������� ����� ������.
            {
                // ��� �������� ���� �� ������ (color) � ������� �� ��������.
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            // ���������� Minimax �����
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            // 6. Alpha-Beta ���������
            if (depth % 2) // Max-������� (�������� �������, ���� ��������).
                alpha = max(alpha, max_score);
            else // Min-������� (������ �������, ���� �������).
                beta = min(beta, min_score);

            if (optimization != "O0" && alpha >= beta)
            {
                // ���������: ���� alpha >= beta, �� ����� ���, ������� Min-����� ������� �� �������� 
                // (��� Max-����� ������� �� ��������), � ����� ����� ������.
                return (depth % 2 ? max_score : min_score); // ���������� ������� ������/������ ����.
            }
        }

        // ���������� ��������� Minimax: �������� �� Max-������, ������� �� Min-������.
        return (depth % 2 ? max_score : min_score);
    }

private:
    // --- ��������� ���� ������ ---
    /**
     * @brief ��������� ��������� �����, ������������ ��� ������������� �����.
     */
    default_random_engine rand_eng;
    /**
     * @brief ����� ������ �������, �������� � ������������ (��������, "NumberAndPotential").
     */
    string scoring_mode;
    /**
     * @brief ����� ����������� ������, �������� � ������������ (��������, "O0", "AB").
     */
    string optimization;
    /**
     * @brief ������ ��� �������� ������� ���� (������� ���� � �����) �� ������� ��������� (����) � ������ ������.
     */
    vector<move_pos> next_move;
    /**
     * @brief ������ ��� �������� ������� ���������� ��������� � ����� ������.
     * ������������ ������ � next_move ��� �������������� ������� ���� (�����).
     */
    vector<int> next_best_state;
    /**
     * @brief ��������� �� ������� ������� �����.
     */
    Board* board;
    /**
     * @brief ��������� �� ��������� ����.
     */
    Config* config;
};