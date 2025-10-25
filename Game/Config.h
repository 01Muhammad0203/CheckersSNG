#pragma once
#include <fstream>// ��� ������ � ������� (������ ��������).
#include <nlohmann/json.hpp>// ��������� ���������� ��� �������� JSON.
using json = nlohmann::json;

#include "../Models/Project_path.h"// ���������� ���� � ������� ��� ������� � ������.

// ����� Config �������� �� �������� � �������������� ������� � ���������� ���� �� ����� settings.json.
class Config
{
public:
    // �����������: ������������� �������� reload() ��� �������� �������� ��� �������� �������.
    Config()
    {
        reload();
    }

    /**
     * @brief ������������� ��������� �� ����� settings.json � ������.
     */
    void reload()
    {
        // �������� ����� ��������. project_path - ��� ���� � �������� ����� �������.
        std::ifstream fin(project_path + "settings.json");
        // ������� ����������� ����� � ������ JSON � ������� nlohmann/json.
        fin >> config;
        fin.close();
    }

    /**
     * @brief ������������� �������� ������ '()' ��� �������� ������� � ����������.
     * ��������� ���������� � ���������� ��� config("������", "�����������������").
     * @param setting_dir ��� ������� (������ ������� � JSON).
     * @param setting_name ��� ��������� (������ ������� � JSON).
     * @return auto: ���������� �������� ��������� (����� ���� int, bool, string � �.�.).
     */
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        // ������ � �������� JSON-������� �� ������.
        return config[setting_dir][setting_name];
    }

private:
    json config;// ��������� ����, �������� ��� ��������� � ���� JSON-�������.
};