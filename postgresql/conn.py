import psycopg2

# Подключение к вашей базе
conn = psycopg2.connect(
    host="highload-sikalovaes.db-msk0.amvera.tech",       # например: 'db.amvera.io'
    port="5432",               # порт PostgreSQL по умолчанию
    database="postgres",   # имя вашей базы
    user="postgres",          # имя пользователя
    password="highload1"   # пароль
)

# Создаём курсор
cur = conn.cursor()

# SQL-команды
sql = """
DROP SCHEMA IF EXISTS hello_schema CASCADE;
CREATE SCHEMA IF NOT EXISTS hello_schema;

CREATE TABLE IF NOT EXISTS hello_schema.users (
    name TEXT PRIMARY KEY,
    count INTEGER DEFAULT 1
);
"""

try:
    cur.execute(sql)
    conn.commit()
    print("Schema and table created successfully.")
except Exception as e:
    print("Error while executing SQL:", e)
    conn.rollback()
