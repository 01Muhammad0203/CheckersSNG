#pragma once
#include <fstream>// Для работы с файлами (чтение настроек).
#include <nlohmann/json.hpp>// Сторонняя библиотека для парсинга JSON.
using json = nlohmann::json;

#include "../Models/Project_path.h"// Глобальный путь к проекту для доступа к файлам.

// Класс Config отвечает за загрузку и предоставление доступа к настройкам игры из файла settings.json.
class Config
{
public:
    // Конструктор: автоматически вызывает reload() для загрузки настроек при создании объекта.
    Config()
    {
        reload();
    }

    /**
     * @brief Перезагружает настройки из файла settings.json в память.
     */
    void reload()
    {
        // Открытие файла настроек. project_path - это путь к корневой папке проекта.
        std::ifstream fin(project_path + "settings.json");
        // Парсинг содержимого файла в объект JSON с помощью nlohmann/json.
        fin >> config;
        fin.close();
    }

    /**
     * @brief Перегруженный оператор вызова '()' для удобного доступа к настройкам.
     * Позволяет обращаться к настройкам как config("Раздел", "НазваниеНастройки").
     * @param setting_dir Имя раздела (первый уровень в JSON).
     * @param setting_name Имя настройки (второй уровень в JSON).
     * @return auto: Возвращает значение настройки (может быть int, bool, string и т.д.).
     */
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        // Доступ к значению JSON-объекта по ключам.
        return config[setting_dir][setting_name];
    }

private:
    json config;// Приватный член, хранящий все настройки в виде JSON-объекта.
};