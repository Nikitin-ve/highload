INSERT INTO key_value_table (key, value, created_at)
VALUES ($1, $2, NOW())
ON CONFLICT DO NOTHING