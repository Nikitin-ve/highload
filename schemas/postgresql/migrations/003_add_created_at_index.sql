-- Добавление индекса на поле created_at для ускорения запросов с фильтрацией по дате
CREATE INDEX IF NOT EXISTS idx_key_value_created_at ON key_value_table(created_at); 