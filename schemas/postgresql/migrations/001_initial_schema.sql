-- Миграция 001: Создание базовой таблицы key_value_table
CREATE TABLE IF NOT EXISTS key_value_table (
    key VARCHAR PRIMARY KEY,
    value VARCHAR
); 