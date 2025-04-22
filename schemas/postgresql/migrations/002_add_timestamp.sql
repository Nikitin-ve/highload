-- Миграция 002: Добавление поля timestamp к таблице key_value_table
ALTER TABLE IF EXISTS key_value_table 
ADD COLUMN IF NOT EXISTS created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(); 