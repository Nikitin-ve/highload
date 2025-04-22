# /// [psql prepare]
import pytest
import os
import glob
import re
from pathlib import Path

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.postgresql']


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    # Создаем функцию для применения миграций
    def apply_migrations(cursor):
        # Создаем таблицу миграций, если её нет
        cursor.execute(
            'CREATE TABLE IF NOT EXISTS schema_migrations ('
            '    version VARCHAR PRIMARY KEY,'
            '    applied_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()'
            ')'
        )
        
        # Находим все файлы миграций
        migrations_dir = service_source_dir.joinpath('schemas/postgresql/migrations')
        migration_files = []
        
        for file_path in glob.glob(os.path.join(migrations_dir, '*.sql')):
            file_name = os.path.basename(file_path)
            match = re.match(r'^(\d+)_.*\.sql$', file_name)
            if match:
                version = match.group(1)
                migration_files.append((version, file_path))
        
        # Сортируем миграции по версии
        migration_files.sort(key=lambda x: x[0])
        
        # Применяем миграции
        for version, file_path in migration_files:
            cursor.execute(
                'SELECT 1 FROM schema_migrations WHERE version = %s',
                (version,)
            )
            if not cursor.fetchall():
                with open(file_path, 'r') as f:
                    sql = f.read()
                cursor.execute(sql)
                cursor.execute(
                    'INSERT INTO schema_migrations (version) VALUES (%s)',
                    (version,)
                )
    
    # Создаем и возвращаем инициализатор базы данных с миграциями
    return pgsql_local_create([{'name': 'admin', 'init_hook': apply_migrations}])
    # /// [psql prepare]