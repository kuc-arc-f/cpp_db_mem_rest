CREATE TABLE temp (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL
);

--system_cache
CREATE TABLE system_cache (
    id TEXT PRIMARY KEY,
    sql TEXT NOT NULL
);
